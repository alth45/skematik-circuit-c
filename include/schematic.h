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
#define MAX_JUNCTIONS 128   /* junction = titik sambung wire-to-wire (T-junction) */
#define GRID     20
#define MENUBAR_H   30   /* baris menu File/Edit/View/Simulasi/Bantu (paling atas) */
#define TOOLBAR_H   94   /* total area UI atas = menubar + toolbar komponen (64 px) */
#define PALETTE_H   64   /* tinggi strip tombol komponen di bawah menubar */
#define TOOLBAR_BTN_W 46
#define TOOLBAR_BTN_H 46
#define TOOLBAR_BTN_GAP 3
#define PIN_R    7
#define STATUS_H 26
#define WIRE_HIT 8

/* ---- Nilai (value) komponen & model perhitungan listrik ---- */
#define VF_LED_DEFAULT   2.0
#define VF_LED_MAX       20e-3
#define VF_DIODE         0.7
#define I_LEAK           1e-9
#define V_EXPLODE        50.0
#define VALUE_STR_CAP    48
#define LDR_DARK_DEFAULT 1e6
#define LDR_LIGHT_RATIO  0.01
#define BUZZER_FREQ_DEFAULT 1000.0
#define IND_DEFAULT_H    100e-6
#define VZENER_DEFAULT   5.1
#define MOSFET_VTH_DEFAULT  2.0
#define POT_R_DEFAULT       10000.0
#define POT_WIPER_DEFAULT   50

/* Ukuran ruang node union-find di sim.c (pin komponen + junction). */
#define NODE_SPACE        (MAX_COMP * 3 + MAX_JUNCTIONS)

/* ===================== Struct dasar ===================== */

typedef struct {
    int x, y;
    int used;
} Junction;

typedef enum {
    COMP_NONE = 0,
    COMP_RESISTOR,
    COMP_SWITCH,
    COMP_BATTERY,
    COMP_LED,
    COMP_GROUND,
    COMP_CAPACITOR,
    COMP_TRANSISTOR, /* NPN: pin0=C(kanan-atas), pin1=E(kanan-bawah), pin2=B(kiri) */
    COMP_DIODE,      /* pin0=A(kiri), pin1=K(kanan) */
    COMP_CAPOLAR,    /* pin0=(+) kiri, pin1=(-) kanan */
    COMP_ACSOURCE,   /* pin0=Live, pin1=Netral; v=VAC RMS; wave=SINE/SQUARE/TRIANGLE */
    COMP_LDR,        /* pin0=kiri pin1=kanan; v=R_gelap (Ohm); on=1 berarti TERANG */
    COMP_BUZZER,     /* pin0=(+), pin1=gnd; v=frekuensi (Hz) */
    COMP_VOLTMETER,  /* pin0=(+), pin1=(-); ideal (impedansi tak hingga) */
    COMP_AMMETER,    /* pin0=masuk, pin1=keluar; ideal (hambatan ~0) */
    COMP_OSCPROBE,   /* pin0=sinyal, pin1=referensi; ideal (impedansi tak hingga) */
    COMP_INDUCTOR,   /* pin0=kiri pin1=kanan; DC: short, AC: XL=2πfL; v=Henry */
    COMP_ZENER,      /* pin0=A(kiri) pin1=K(kanan); forward Vf≈0.7, reverse breakdown @ v (Vz) */
    COMP_MOSFET,     /* N-channel enhancement: pin0=D(kanan-atas), pin1=S(kanan-bawah), pin2=G(kiri); v=Vth */
    COMP_POT,        /* potensiometer 3-pin: pin0=A(kiri), pin1=B(kanan), pin2=W(atas);
                        R_A = v·on%, R_B = v·(100−on)%; on = posisi wiper 0..100 */
    COMP_AND,        /* pin0=A, pin1=B, pin2=OUT */
    COMP_OR,
    COMP_NOT,        /* pin0=IN, pin1=OUT */
    COMP_NAND,
    COMP_NOR,
    COMP_XOR
} CompType;

typedef enum {
    TOOL_SELECT = 0,
    TOOL_BOX,
    TOOL_WIRE,
    TOOL_RESISTOR,
    TOOL_SWITCH,
    TOOL_BATTERY,
    TOOL_ACSOURCE,
    TOOL_LED,
    TOOL_GROUND,
    TOOL_CAPACITOR,
    TOOL_TRANSISTOR,
    TOOL_DIODE,
    TOOL_CAPOLAR,
    TOOL_LDR,
    TOOL_BUZZER,
    TOOL_VOLTMETER,
    TOOL_AMMETER,
    TOOL_OSCPROBE,
    TOOL_INDUCTOR,
    TOOL_ZENER,
    TOOL_MOSFET,
    TOOL_POT,
    TOOL_AND, TOOL_OR, TOOL_NOT, TOOL_NAND, TOOL_NOR, TOOL_XOR,
    TOOL_DELETE
} ToolKind;

/* Bentuk gelombang sumber AC. */
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE
} WaveKind;

typedef struct {
    CompType type;
    int x, y;
    int id;
    int rot;           /* 0..3 = 0/90/180/270 CW */
    int on;            /* switch 0/1; LDR 0/1 (1=TERANG); POT 0..100 (wiper %) */
    double v;          /* nilai numerik (satuan SI per tipe; lihat comp_value_unit) */
    WaveKind wave;     /* AC source: SINE/SQUARE/TRIANGLE */
    int led_color;     /* COMP_LED: 0=merah, 1=hijau, 2=kuning, 3=biru */

} Component;

typedef struct {
    int comp_id;   /* >=0: id komponen; -1: id junction (pin_idx = junction id) */
    int pin_idx;   /* 0..N untuk pin; junction: id junction 0..MAX_JUNCTIONS-1 */
} PinRef;

typedef struct {
    PinRef a, b;
} Wire;

/* Jumlah tombol palette: 4 fixed (Select/Blok/Wire/Delete) +
   25 komponen scrollable = 29. */
#define BTN_COUNT 29

/* Batas seleksi multi (model.c). */
#define MAX_SEL 256

/* ===================== State Global (model.c) ===================== */
extern Component comps[MAX_COMP];
extern int  comp_count;
extern int  next_id;
extern ToolKind current_tool;
extern int  selected_id;
extern int  dragging_id;
extern int  drag_dx, drag_dy;

extern Wire wires[MAX_WIRES];
extern int  wire_count;

extern Junction junctions[MAX_JUNCTIONS];
extern int      junction_count;

extern int wiring_active;
extern PinRef wiring_from;
extern int wiring_mx, wiring_my;

/* ---- Multi-select / blok rangkaian ---- */
extern int selection[MAX_SEL];
extern int sel_count;
int  add_to_selection(int id);
void toggle_selection(int id);
void clear_selection(void);
int  model_select_rect(int x1, int y1, int x2, int y2, int add);
void model_translate_selection(int dx, int dy);
int  model_delete_selection(void);
int  model_duplicate_selection(int dx, int dy);

extern int box_selecting;
extern int box_x1, box_y1, box_x2, box_y2;
#define BOX_DRAG_MIN 4

Junction *find_junction_by_id(int id);
int  model_add_junction(int x, int y);
void model_remove_junction(int id);

/* ---- Drag multi (grup terseleksi ikut bergerak) ---- */
#define MAX_GROUP_DRAG 256
typedef struct { int idx; int x0, y0; } GroupDragItem;
extern int group_dragging;
extern GroupDragItem group_drag_items[MAX_GROUP_DRAG];
extern int group_drag_n;
extern int group_drag_ox, group_drag_oy;

/* Tombol toolbar (atas) */
extern const ToolKind btnTools[BTN_COUNT];
extern const wchar_t *btnLabels[BTN_COUNT];
#define CLEAR_BTN_INDEX BTN_COUNT
#define SAVE_BTN_INDEX  (BTN_COUNT + 1)
#define OPEN_BTN_INDEX  (BTN_COUNT + 2)
#define RUN_BTN_INDEX   (BTN_COUNT + 3)

/* Toolbar hit-test + draw (toolbar_palette.c + toolbar_actions.c). */
int toolbar_hit(int x, int y, int W);
int toolbar_tool_at(int x, int y, int W, int *is_clear);
void draw_toolbar(HDC hdc, int W);

/* ---- Layout toolbar (fixed + scrollable + fixed) ---- */
#define TOOLBAR_FIXED_LEFT_COUNT  4
#define TOOLBAR_FIXED_RIGHT_COUNT 4
#define TOOLBAR_SCROLL_COUNT      (BTN_COUNT - TOOLBAR_FIXED_LEFT_COUNT)

extern int g_toolbar_scroll_x;

int toolbar_scroll_start_x(void);
int toolbar_scroll_end_x(int W);
int toolbar_max_scroll(int W);
int toolbar_get_scroll_x(int W);
int toolbar_action_x(int action_idx, int W);
int toolbar_scroll_wheel(int delta, int x, int y, int W);
int  toolbar_scrollbar_hit(int x, int y, int W, int *is_thumb);
void toolbar_scrollbar_begin_drag(int x, int y, int W);
int  toolbar_scrollbar_drag(int mx, int W);
void toolbar_scrollbar_end_drag(void);
int  toolbar_scrollbar_dragging(void);
int  toolbar_scrollbar_hover_thumb(int x, int y, int W);

/* ===================== Menu Bar klasik ===================== */
#define IDM_FILE_NEW      0x1101
#define IDM_FILE_OPEN     0x1102
#define IDM_FILE_SAVE     0x1103
#define IDM_FILE_SAVEAS   0x1104
#define IDM_FILE_EXIT     0x1105
#define IDM_FILE_EXPORT_PNG 0x1106
#define IDM_EDIT_UNDO     0x1110
#define IDM_EDIT_REDO     0x1116
#define IDM_EDIT_DUPL     0x1111
#define IDM_EDIT_DELSEL   0x1112
#define IDM_EDIT_ROTATE   0x1113
#define IDM_EDIT_CLEAR    0x1114
#define IDM_EDIT_SELECTALL 0x1115
#define IDM_VIEW_GRID     0x1120
#define IDM_VIEW_STATUS   0x1121
#define IDM_VIEW_ZOOMIN   0x1122
#define IDM_VIEW_ZOOMOUT  0x1123
#define IDM_VIEW_ZOOMFIT  0x1124
#define IDM_VIEW_MINIMAP  0x1125
#define IDM_SIM_RUN       0x1130
#define IDM_SIM_STOP      0x1131
#define IDM_SIM_RECOMPUTE 0x1132
#define IDM_SIM_RUN_TOGGLE 0x1133
#define IDM_HELP_SHORTCUT 0x1140
#define IDM_HELP_ABOUT    0x1141
#define IDM_VIEW_OSC      0x1150

int menubar_hit(int x, int y);
int menubar_click(HWND hwnd, int x, int y);
void draw_menubar(HDC hdc, int W);
void menubar_command(HWND hwnd, int cmd);

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
void comp_bounding_half(const Component *c, int *hw, int *hh);
int find_pin_at(int x, int y, PinRef *out, int *px, int *py);

/* Posisi endpoint PinRef — mendukung dua jenis endpoint:
   - comp_id >= 0 : pin komponen (pin_idx = index pin)
   - comp_id < 0  : junction (pin_idx = id junction)
   Return (0,0) bila endpoint tidak valid.
   Diimplementasi di model.c; dipakai draw.c (draw_wires), events.c,
   dan sim.c (hot-wire detection). */
void pinref_pos(PinRef p, int *x, int *y);

/* Cari wire yang interior-nya terdekat dengan (x,y) — untuk T-junction.
   Endpoint wire (titik ujung) tidak dianggap interior.
   Return 1 sukses (isi *out_wi, *out_jx, *out_jy), 0 bila tidak ada.
   Diimplementasi di model.c; dipakai events.c saat drop wire. */
int  find_wire_segment_at(int x, int y,
                          int *out_wi, int *out_jx, int *out_jy);

void model_add_component(CompType type, int x, int y);
void model_remove_at(int x, int y);
void model_clear_all(void);
void model_rotate_selected(void);
int model_add_wire(PinRef a, PinRef b);
int model_remove_wire_at(int x, int y);

/* ---- Undo / Redo ---- */
void undo_push(void);
int  undo_undo(void);
int  undo_redo(void);
int  undo_can_undo(void);
int  undo_can_redo(void);
void undo_reset(void);
void undo_begin_batch(void);
void undo_end_batch(void);
void model_set_component_value(int comp_id, double v);

/* Nilai komponen. */
double comp_default_value(CompType t);
int    comp_has_value(CompType t);
const wchar_t *comp_value_unit(CompType t);
int    parse_value_text(const wchar_t *s, double *out);
int    parse_value_text_a(const char *s, double *out);
void   format_value(double v, const wchar_t *unit, wchar_t *out, int cap);
void   edit_value_dialog(HWND hwnd, Component *c);

/* Peta nama tipe <-> string JSON (fileio.c). */
const char *comp_type_json_name(CompType t);
int         comp_type_from_json_name(const char *s);

/* ===================== Gambar Simbol ===================== */
void with_rotation(HDC hdc, Component *c, void (*fn)(HDC, int, int, BOOL));

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
void draw_k_ldr(HDC hdc, Component *c);
void draw_k_buzzer(HDC hdc, Component *c);
void draw_k_voltmeter(HDC hdc, Component *c);
void draw_k_ammeter(HDC hdc, Component *c);
void draw_k_oscprobe(HDC hdc, Component *c);
void draw_k_inductor(HDC hdc, Component *c);
void draw_k_zener(HDC hdc, Component *c);
void draw_k_mosfet(HDC hdc, Component *c);
void draw_k_pot(HDC hdc, Component *c);
void draw_k_and(HDC hdc, Component *c);
void draw_k_or(HDC hdc, Component *c);
void draw_k_not(HDC hdc, Component *c);
void draw_k_nand(HDC hdc, Component *c);
void draw_k_nor(HDC hdc, Component *c);
void draw_k_xor(HDC hdc, Component *c);

void draw_component(HDC hdc, Component *c);
void draw_canvas(HWND hwnd);
void draw_wires(HDC hdc);
void draw_selection_overlay(HDC hdc);
void draw_marquee_overlay(HDC hdc);

/* ===================== Event Handler ===================== */
void on_lbutton_down(HWND hwnd, int x, int y);
void on_lbutton_up(HWND hwnd, int x, int y);
void on_rbutton_down(HWND hwnd, int x, int y);
void on_rbutton_dblclk(HWND hwnd, int x, int y);
void on_mouse_move(HWND hwnd, int x, int y);
void on_key_down(HWND hwnd, WPARAM key);
void on_lbutton_dblclk(HWND hwnd, int x, int y);
void show_context_menu(HWND hwnd, int sx, int sy, Component *c);

/* ===================== File / JSON ===================== */
int model_save_json(const wchar_t *path, wchar_t *err, int err_cap);
int model_load_json(const wchar_t *path, wchar_t *err, int err_cap);
void model_save_dialog(HWND hwnd);
void model_load_dialog(HWND hwnd);
void set_status(const wchar_t *fmt, ...);
extern wchar_t g_status[256];

/* ===================== Simulasi Run (sim.c) ===================== */
extern int sim_running;
extern int sim_led_count;
extern int sim_short;
extern int sim_has_battery;
extern int sim_has_ac;
extern int sim_ac_mode;
void sim_recompute(void);
void sim_set_running(int on);
int sim_wire_hot(int idx);
int sim_comp_lit(const Component *c);
int sim_comp_conducting(const Component *c);
int sim_diode_fwd(const Component *c);
const wchar_t *sim_message(void);

extern double sim_vsrc;
extern double sim_rloop;
extern double sim_iled_max;
extern double sim_vdrop_max;
extern int    sim_exploded;
extern int    sim_overcur;

/* Lokasi visual short (canvas coords). -1 bila tidak ada. */
extern int sim_short_x, sim_short_y;
extern int sim_short_comp_id;
extern int sim_short_wire_idx;

int  short_marker_hit(int sx, int sy);
void short_marker_pan_to(int canvas_w, int canvas_h);
void sim_update_short_position(void);

int    sim_comp_blown(const Component *c);
double sim_comp_current(const Component *c);
void   sim_format_si(double v, const wchar_t *unit, wchar_t *out, int cap);

/* ===================== Zoom & Pan Canvas (model.c) ===================== */
extern double g_zoom;
extern int    g_pan_x;
extern int    g_pan_y;

void canvas_screen_to_canvas(int sx, int sy, int *cx, int *cy);
void canvas_canvas_to_screen(int cx, int cy, int *sx, int *sy);
void canvas_zoom_at(int sx, int sy, double factor);
void canvas_zoom_center(HWND hwnd, double factor);
void canvas_zoom_fit(int view_w, int view_h);
void canvas_zoom_reset(void);

/* ---- Minimap (ui/minimap.c) ---- */
extern int g_show_minimap;
extern int g_minimap_mode;

void minimap_render(HDC hdc, int canvas_w, int canvas_h);
int  minimap_hit(int x, int y, int canvas_w, int canvas_h);
void minimap_click(int x, int y, int canvas_w, int canvas_h);
void minimap_begin_drag(int x, int y, int canvas_w, int canvas_h);
void minimap_drag(int x, int y, int canvas_w, int canvas_h);
void minimap_end_drag(void);
int  minimap_dragging(void);

/* ---- Probe Osiloskop ---- */
double sim_ac_voltage(const Component *src, double t);
void   sim_build_probe_responses(void);
double sim_probe_voltage(const Component *probe, double t);

void osc_open(HWND hwnd_parent);
void osc_refresh(void);
void osc_close(void);
int  osc_is_open(void);

/* ===================== Window Procedure (main.c) ===================== */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void render_canvas_scene(HDC hdc, int W, int H, int include_ui_overlay);

/* ---- PNG Export ---- */
int  png_write_rgb(const wchar_t *path, int width, int height,
                   const unsigned char *rgb);
int  export_canvas_png(HWND hwnd, const wchar_t *path,
                       wchar_t *err, int err_cap);
void model_export_png_dialog(HWND hwnd);

/* ===================== Splash Screen ===================== */
void splash_run(HINSTANCE hInst, HWND hwnd_main);

/* ===================== Pemisah PRODUKSI vs TES ===================== */
#if defined(SCHEMATIC_PRODUCTION) && defined(SCHEMATIC_TEST_STUBS)
#error "Konflik mode build: SCHEMATIC_PRODUCTION dan SCHEMATIC_TEST_STUBS tidak boleh didefinisikan bersamaan."
#endif

/* ===================== Kontrak LDR / Buzzer ===================== */
int    sim_buzzer_on(const Component *c);
double sim_ldr_effective_r(const Component *c);

/* ===================== Buzzer Audio (buzzer_audio.c) =====================
   Memainkan nada sinus pada frekuensi buzzer aktif (v Hz) selama RUN.
   Dipanggil otomatis oleh sim_recompute() setiap perubahan state. */

/* Putar / stop / restart nada sesuai state buzzer aktif.
   Idempoten — aman dipanggil berkali-kali per detik. */
void buzzer_audio_update(void);

/* Hentikan playback (dipakai cleanup). */
void buzzer_audio_stop(void);

/* Bersihkan resource audio (dipanggil saat window destroy). */
void buzzer_audio_cleanup(void);

/* ---- Voltmeter / Ammeter / OSC / Zener / Gate ---- */
double sim_voltmeter_reading(const Component *c);
int    sim_voltmeter_connected(const Component *c);
double sim_ammeter_reading(const Component *c);
int    sim_ammeter_connected(const Component *c);
int    sim_gate_out_high(const Component *c);
int    sim_zener_fwd(const Component *c);
int    sim_zener_breakdown(const Component *c);

#ifdef SCHEMATIC_TEST_STUBS
void test_reset_model(void);
#endif

#endif /* SCHEMATIC_H */