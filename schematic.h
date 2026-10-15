#ifndef SCHEMATIC_H
#define SCHEMATIC_H

#define _USE_MATH_DEFINES
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* ===================== Konstanta & Tipe ===================== */
#define MAX_COMP 256
#define MAX_WIRES 512
#define GRID     20
#define TOOLBAR_H 64
#define TOOLBAR_BTN_W 52
#define TOOLBAR_BTN_H 46
#define PIN_R    7
#define STATUS_H 26
#define WIRE_HIT 8

typedef enum {
    COMP_NONE = 0,
    COMP_RESISTOR,
    COMP_SWITCH,
    COMP_BATTERY,
    COMP_LED,
    COMP_GROUND,
    COMP_CAPACITOR,
    COMP_TRANSISTOR /* NPN: pin0=C(kiri) pin1=E(kanan) pin2=B(bawah) */
} CompType;

/* Tool yang dipilih user di palette */
typedef enum {
    TOOL_SELECT = 0,   /* geser/select komponen, default */
    TOOL_WIRE,         /* tarik kabel antar pin */
    TOOL_RESISTOR,
    TOOL_SWITCH,
    TOOL_BATTERY,
    TOOL_LED,
    TOOL_GROUND,
    TOOL_CAPACITOR,
    TOOL_TRANSISTOR,
    TOOL_DELETE
} ToolKind;

typedef struct {
    CompType type;
    int x, y;          // posisi pusat (pixel, koordinat canvas)
    int id;
    int rot;           // 0,1,2,3 = 0,90,180,270 derajat (CW)
    int on;            // saklar: 0 = terbuka/OFF, 1 = tertutup/ON
} Component;

/* Pin = terminal komponen. Ground hanya 1 pin (atas). */
typedef struct {
    int comp_id;   // id komponen
    int pin_idx;   // 0 = kiri/atas, 1 = kanan (ground hanya 0)
} PinRef;

typedef struct {
    PinRef a, b;
} Wire;

/* Jumlah tombol palette */
#define BTN_COUNT 10

/* ===================== State Global (model.c) ===================== */
extern Component comps[MAX_COMP];
extern int  comp_count;
extern int  next_id;
extern ToolKind current_tool;
extern int  selected_id;
extern int  dragging_id;
extern int  drag_dx, drag_dy;

/* Wires */
extern Wire wires[MAX_WIRES];
extern int  wire_count;

/* Wiring sementara saat drag kabel */
extern int wiring_active;
extern PinRef wiring_from;
extern int wiring_mx, wiring_my;

/* Tombol toolbar (atas) */
extern const ToolKind btnTools[BTN_COUNT];
extern const wchar_t *btnLabels[BTN_COUNT]; /* nama tool = tooltip/status */
#define CLEAR_BTN_INDEX BTN_COUNT       /* tombol aksi Clear */
#define SAVE_BTN_INDEX  (BTN_COUNT + 1) /* tombol aksi Save JSON */
#define OPEN_BTN_INDEX  (BTN_COUNT + 2) /* tombol aksi Open JSON */
#define RUN_BTN_INDEX   (BTN_COUNT + 3) /* tombol aksi Run/Stop simulasi */

/* Toolbar: hit-test & gambar (toolbar.c).
   toolbar_hit: -1 = tidak kena; 0..BTN_COUNT-1 = indeks tool;
   CLEAR/SAVE/OPEN_BTN_INDEX = tombol aksi. */
int toolbar_hit(int x, int y);
int toolbar_tool_at(int x, int y, int *is_clear);
void draw_toolbar(HDC hdc, int W);

/* ===================== Utilitas (model.c) ===================== */
int snap(int v);
Component *find_comp_by_id(int id);
Component *find_comp_at(int x, int y);
int comp_pin_count(const Component *c);
int comp_pin_valid(const Component *c, int pin_idx);
void comp_pin_pos(const Component *c, int pin_idx, int *px, int *py);
void comp_local_to_world(const Component *c, int lx, int ly, int *wx, int *wy);
int find_pin_at(int x, int y, PinRef *out, int *px, int *py);
void model_add_component(CompType type, int x, int y);
void model_remove_at(int x, int y);
void model_clear_all(void);
void model_rotate_selected(void);
int model_add_wire(PinRef a, PinRef b);
int model_remove_wire_at(int x, int y);

/* ===================== Gambar (draw.c) ===================== */
void draw_component(HDC hdc, Component *c);
void draw_canvas(HWND hwnd);
void draw_wires(HDC hdc);

/* ===================== Event Handler (events.c) ===================== */
void on_lbutton_down(HWND hwnd, int x, int y);
void on_lbutton_up(HWND hwnd, int x, int y);
void on_rbutton_down(HWND hwnd, int x, int y);
void on_mouse_move(HWND hwnd, int x, int y);
void on_key_down(HWND hwnd, WPARAM key);

/* ===================== File / JSON (fileio.c) ===================== */
int model_save_json(const wchar_t *path, wchar_t *err, int err_cap);
int model_load_json(const wchar_t *path, wchar_t *err, int err_cap);
void model_save_dialog(HWND hwnd);
void model_load_dialog(HWND hwnd);
void set_status(const wchar_t *fmt, ...);
extern wchar_t g_status[256];

/* ===================== Simulasi Run (sim.c) ===================== */
/* Aturan Ideal: baterai pin0=(+), pin1=(-); ground = (-);
   resistor & saklar-ON menghantar; kapasitor = blokir DC (terbuka);
   transistor NPN menghantar C-E jika basis kena (+) dan emitor kena (-);
   LED nyala jika anoda(pin0) kena (+) dan katoda(pin1) kena (-)
   lewat jalur menghantar. */
extern int sim_running;      /* 1 = mode Run (simulasi jalan) */
extern int sim_led_count;    /* jumlah LED menyala (hasil terakhir) */
extern int sim_short;        /* 1 = (+) tersambung langsung ke (-) */
extern int sim_has_battery;  /* 1 = ada baterai di canvas */
void sim_recompute(void);    /* hitung ulang hasil simulasi */
void sim_set_running(int on);/* masuk/keluar mode Run */
int sim_wire_hot(int idx);   /* 1 = kabel idx bertegangan (+) */
int sim_comp_lit(const Component *c); /* 1 = LED ini menyala */
int sim_comp_conducting(const Component *c); /* 1 = transistor ini ON (C-E tembus) */
const wchar_t *sim_message(void);     /* pesan status hasil simulasi */

/* ===================== Window Procedure (main.c) ===================== */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif /* SCHEMATIC_H */
