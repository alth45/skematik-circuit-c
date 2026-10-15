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
#define MENUBAR_H   30   /* baris menu File/Edit/View/Simulasi/Bantu (paling atas) */
#define TOOLBAR_H   94   /* total area UI atas = menubar + toolbar komponen (64 px) */
#define PALETTE_H   64   /* tinggi strip tombol komponen di bawah menubar */
#define TOOLBAR_BTN_W 46
#define TOOLBAR_BTN_H 46
#define PIN_R    7
#define STATUS_H 26
#define WIRE_HIT 8

/* ---- Nilai (value) komponen & model perhitungan listrik ---- */
#define VF_LED_DEFAULT   2.0   /* LED default: Vf = 2.0 V, If max = 20 mA */
#define VF_LED_MAX       20e-3
#define VF_DIODE         0.7   /* dioda Si: drop forward 0.7 V */
#define I_LEAK           1e-9  /* arus bocor pA lewat kapasitor (DC steady) */
#define V_EXPLODE        50.0  /* LED "meledak" bila tegangan bolak-balik > 50 V */
#define VALUE_STR_CAP    48

typedef enum {
    COMP_NONE = 0,
    COMP_RESISTOR,
    COMP_SWITCH,
    COMP_BATTERY,
    COMP_LED,
    COMP_GROUND,
    COMP_CAPACITOR,
    COMP_TRANSISTOR, /* NPN: pin0=C(kiri-atas) pin1=E(kanan-bawah) pin2=B(kiri-tengah) */
    COMP_DIODE,      /* pin0=A(anoda kiri) pin1=K(katoda kanan); arah sama dgn LED */
    COMP_CAPOLAR,    /* kapasitor polar/elektrolit: pin0=(+) anoda kiri, pin1=(-) katoda kanan */
    COMP_ACSOURCE    /* sumber AC (tegangan bolak-balik): pin0=Hidup/Live, pin1=Netral.
                        v = tegangan RMS dalam Volt (default 220 V, 50 Hz). */
} CompType;

/* Tool yang dipilih user di palette */
typedef enum {
    TOOL_SELECT = 0,   /* geser/select komponen, default */
    TOOL_WIRE,         /* tarik kabel antar pin */
    TOOL_RESISTOR,
    TOOL_SWITCH,
    TOOL_BATTERY,
    TOOL_ACSOURCE,     /* sumber AC (tegangan bolak-balik) */
    TOOL_LED,
    TOOL_GROUND,
    TOOL_CAPACITOR,
    TOOL_TRANSISTOR,
    TOOL_DIODE,
    TOOL_CAPOLAR,
    TOOL_DELETE
} ToolKind;

typedef struct {
    CompType type;
    int x, y;          // posisi pusat (pixel, koordinat canvas)
    int id;
    int rot;           // 0,1,2,3 = 0,90,180,270 derajat (CW)
    int on;            // saklar: 0 = terbuka/OFF, 1 = tertutup/ON
    double v;          // nilai numerik komponen:
                       //   resistor   = ohm      (default 220)
                       //   battery    = volt     (default 9)
                       //   acsource   = VAC RMS  (default 220, 50 Hz)
                       //   LED        = Vf volt  (default 2.0)
                       //   dioda      = Vf volt  (default 0.7)
                       //   kapasitor & polar = farad (default 100e-9)
                       //   switch/transistor/ground tidak dipakai (0)
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
#define BTN_COUNT 13

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

/* ===================== Menu Bar klasik (File/Edit/View/Simulasi/Bantu) ====
   Digambar manual di strip paling atas window (tinggi MENUBAR_H), DI ATAS
   toolbar tombol komponen. Koordinat fungsi ini memakai koordinat client
   window utuh (belum di-offset). — toolbar.c */
#define IDM_FILE_NEW      0x1101
#define IDM_FILE_OPEN     0x1102
#define IDM_FILE_SAVE     0x1103
#define IDM_FILE_SAVEAS   0x1104
#define IDM_FILE_EXIT     0x1105
#define IDM_EDIT_UNDO     0x1110
#define IDM_EDIT_DUPL     0x1111
#define IDM_EDIT_DELSEL   0x1112
#define IDM_EDIT_ROTATE   0x1113
#define IDM_EDIT_CLEAR    0x1114
#define IDM_VIEW_GRID     0x1120
#define IDM_VIEW_STATUS   0x1121
#define IDM_VIEW_ZOOMIN   0x1122
#define IDM_VIEW_ZOOMOUT  0x1123
#define IDM_VIEW_ZOOMFIT  0x1124
#define IDM_SIM_RUN       0x1130
#define IDM_SIM_STOP      0x1131
#define IDM_SIM_RECOMPUTE 0x1132
#define IDM_SIM_RUN_TOGGLE 0x1133  /* untuk accelerator F5 */
#define IDM_HELP_SHORTCUT 0x1140
#define IDM_HELP_ABOUT    0x1141

/* Hit-test menubar: -1 = tidak kena; >=0 = indeks menu utama (0=File...). */
int menubar_hit(int x, int y);
/* Klik pada menubar: buka popup TrackPopupMenu; jika item dipilih, jalankan
   aksinya. Mengembalikan ID command yang dieksekusi, atau 0 bila batal. */
int menubar_click(HWND hwnd, int x, int y);
/* Gambar strip menubar (baris pertama, tinggi MENUBAR_H). */
void draw_menubar(HDC hdc, int W);
/* Eksekusi command menubar (dipakai juga oleh accelerator WM_COMMAND). */
void menubar_command(HWND hwnd, int cmd);

/* Preferensi tampilan (model.c): grid tampil / status bar tampil. */
extern int g_show_grid;
extern int g_show_status;

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

/* Nilai komponen: default per tipe, parser teks ("2.2k", "100u", "4.7"),
   formatter human-readable, dan dialog edit value (klik-kanan / double-klik). */
double comp_default_value(CompType t);
int    comp_has_value(CompType t);          /* 1 jika tipe punya value yang bisa diedit */
const wchar_t *comp_value_unit(CompType t); /* "Ohm","V","Vf","F" atau NULL */
int    parse_value_text(const wchar_t *s, double *out);   /* 1 = sukses */
int    parse_value_text_a(const char *s, double *out);    /* varian ASCII (JSON) */
void   format_value(double v, const wchar_t *unit, wchar_t *out, int cap);
void   edit_value_dialog(HWND hwnd, Component *c);        /* dialog input nilai */

/* ===================== Gambar Simbol (draw.c + komponen_*.c) =====================
   draw.c = dispatcher & utilitas rotasi; tiap simbol komponen hidup di file
   sendiri: komponen_resistor.c, komponen_switch.c, komponen_battery.c,
   komponen_led.c, komponen_ground.c, komponen_capacitor.c, transistor.c,
   komponen_diode.c, komponen_capolar.c, komponen_acsource.c. */

/* Utilitas bersama (didefinisikan di draw.c):
   gambar simbol dengan rotasi dunia GDI; fn menggambar dgn asumsi rot=0. */
void with_rotation(HDC hdc, Component *c, void (*fn)(HDC, int, int, BOOL));

/* Entry point per-file simbol — signature seragam (HDC, Component*). */
void draw_k_resistor(HDC hdc, Component *c);
void draw_k_switch(HDC hdc, Component *c);
void draw_k_battery(HDC hdc, Component *c);
void draw_k_led(HDC hdc, Component *c);
void draw_k_ground(HDC hdc, Component *c);
void draw_k_capacitor(HDC hdc, Component *c);
void draw_k_transistor(HDC hdc, Component *c);
void draw_k_diode(HDC hdc, Component *c);
void draw_k_capolar(HDC hdc, Component *c);
void draw_k_acsource(HDC hdc, Component *c);

void draw_component(HDC hdc, Component *c);
void draw_canvas(HWND hwnd);
void draw_wires(HDC hdc);

/* ===================== Event Handler (events.c) ===================== */
void on_lbutton_down(HWND hwnd, int x, int y);
void on_lbutton_up(HWND hwnd, int x, int y);
void on_rbutton_down(HWND hwnd, int x, int y); /* klik-kanan = menu konteks */
void on_rbutton_dblclk(HWND hwnd, int x, int y);/* klik-kanan 2x = hapus */
void on_mouse_move(HWND hwnd, int x, int y);
void on_key_down(HWND hwnd, WPARAM key);
void on_lbutton_dblclk(HWND hwnd, int x, int y); /* double-klik = edit value */
void show_context_menu(HWND hwnd, int sx, int sy, Component *c); /* menu klik-kanan */

/* ===================== File / JSON (fileio.c) ===================== */
int model_save_json(const wchar_t *path, wchar_t *err, int err_cap);
int model_load_json(const wchar_t *path, wchar_t *err, int err_cap);
void model_save_dialog(HWND hwnd);
void model_load_dialog(HWND hwnd);
void set_status(const wchar_t *fmt, ...);
extern wchar_t g_status[256];

/* ===================== Simulasi Run (sim.c) ===================== */
/* Aturan Ideal: baterai pin0=(+), pin1=(-); sumber AC pin0=Live, pin1=Netral
   (dipakai sebagai sumber tegangan bolak-balik; untuk analisis DC steady
   dipakai pendekatan nilai RMS bila tak ada baterai DC);
   ground = (-);
   resistor & saklar-ON menghantar; dioda menghantar jika anoda(pin0)
   berada di net (+) (ideal: satu arah A->K);
   kapasitor & kapasitor polar = blokir DC (terbuka), TAPI menghantar AC
   (digabung bila loop hanya disuplai sumber AC);
   transistor NPN menghantar C-E jika basis kena (+) dan emitor kena (-);
   LED nyala jika anoda(pin0) kena (+) dan katoda(pin1) kena (-)
   lewat jalur menghantar. */
extern int sim_running;      /* 1 = mode Run (simulasi jalan) */
extern int sim_led_count;    /* jumlah LED menyala (hasil terakhir) */
extern int sim_short;        /* 1 = (+) tersambung langsung ke (-) */
extern int sim_has_battery;  /* 1 = ada baterai di canvas */
extern int sim_has_ac;       /* 1 = ada sumber AC di canvas */
extern int sim_ac_mode;      /* 1 = loop dihitung dari sumber AC (tanpa baterai DC) */
void sim_recompute(void);    /* hitung ulang hasil simulasi */
void sim_set_running(int on);/* masuk/keluar mode Run */
int sim_wire_hot(int idx);   /* 1 = kabel idx bertegangan (+) */
int sim_comp_lit(const Component *c); /* 1 = LED ini menyala */
int sim_comp_conducting(const Component *c); /* 1 = transistor ini ON (C-E tembus) */
int sim_diode_fwd(const Component *c);    /* 1 = dioda ini menghantar (forward A->K) */
const wchar_t *sim_message(void);     /* pesan status hasil simulasi */

/* ---- Hasil perhitungan kuantitatif (Ohm + hukum tegangan) ---- */
extern double sim_vsrc;       /* total tegangan seri baterai (V) */
extern double sim_rloop;      /* resistansi loop LED paling panas (Ohm) */
extern double sim_iled_max;   /* arus LED terbesar (A) */
extern int    sim_exploded;   /* 1 = ada LED MeLEDAK (kelebihan arus/tegangan) */
extern int    sim_overcur;    /* 1 = ada LED nyala tapi arus > 20 mA (berbahaya) */
int    sim_comp_blown(const Component *c);   /* 1 = LED ini meledak */
double sim_comp_current(const Component *c); /* arus lewat komponen ini (A), <0 = tak terdefinisi */
void   sim_format_si(double v, const wchar_t *unit, wchar_t *out, int cap);

/* ===================== Window Procedure (main.c) ===================== */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif /* SCHEMATIC_H */
