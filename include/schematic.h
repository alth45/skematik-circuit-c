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
#define LDR_DARK_DEFAULT 1e6   /* LDR default: 1 MOhm saat gelap */
#define LDR_LIGHT_RATIO  0.01  /* saat TERANG: R ~ 1% dari nilai gelap (~10 kOhm) */
#define BUZZER_FREQ_DEFAULT 1000.0 /* buzzer default: 1 kHz; v = frekuensi Hz */

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
    COMP_ACSOURCE,   /* sumber AC (tegangan bolak-balik): pin0=Hidup/Live, pin1=Netral.
                        v = tegangan RMS dalam Volt (default 220 V, 50 Hz). */
    COMP_LDR,        /* LDR / fotoresistor: resistansi bergantung cahaya.
                        pin0=kiri pin1=kanan; v = resistansi saat GELAP (Ohm, default 1M).
                        Saat terang resistansi ~1% dari nilai gelap. Menghantar seperti resistor. */
    COMP_BUZZER      /* buzzer (piezo/elektromagnetik): pin0=input(+) pin1=gnd(-).
                        v = frekuensi bunyi Hz (default 1000). Berbunyi bila kedua pin
                        terhubung ke kutub berlawanan lewat jalur menghantar. */
} CompType;

/* Tool yang dipilih user di palette */
typedef enum {
    TOOL_SELECT = 0,   /* geser/select komponen (Ctrl+klik = tambah ke seleksi), default */
    TOOL_BOX,          /* blok/rangeleksi: drag kotak untuk memilih banyak komponen */
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
    TOOL_LDR,            /* LDR / fotoresistor (sensor cahaya) */
    TOOL_BUZZER,         /* buzzer (output suara) */
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
                       //   LDR        = R gelap dalam Ohm (default 1e6 / 1 MOhm)
                       //   buzzer     = frekuensi Hz (default 1000)
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

/* Jumlah tombol palette (tambah 1: tool Blok/Multi-Select) */
#define BTN_COUNT 16

/* Batas seleksi multi (model.c) */
#define MAX_SEL 256

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

/* ---- Multi-select / blok rangkaian (fitur "Blok") --------------------
   selection[]  : daftar id komponen yang sedang terseleksi (unik, urut klik)
   sel_count    : jumlah anggota seleksi
   selected_id  : id komponen "primer" — selalu sama dengan anggota seleksi
                  terakhir bila ada >1, atau -1 bila seleksi tunggal/kosong.
   add_to_selection(id)     : tambah id ke seleksi (tolak duplikat/invalid)
   toggle_selection(id)     : Ctrl+klik — masukkan/keluarkan id dari seleksi
   clear_selection()        : kosongkan seleksi multi + selected_id
   model_select_rect(x1,y1,x2,y2,add) : blok semua komponen yang PENUH
                  berada dalam kotak (koordinat sudut bebas); add=1 ->
                  gabungkan dengan seleksi yang ada (Shift+drag).
   model_translate_selection(dx,dy) : geser SEMUA anggota seleksi sebesar
                  (dx,dy) pixel (sudah di-snap ke grid oleh pemanggil).
   model_delete_selection() : hapus seluruh anggota seleksi + kabel ikutannya.
   model_duplicate_selection(dx,dy) : salin blok terpilih sebagai grup baru
                  dengan offset (dx,dy); kabel internal ikut tersalin;
                  seleksi berpindah ke salinan. Return jumlah salinan. */
extern int selection[MAX_SEL];
extern int sel_count;
int  add_to_selection(int id);
void toggle_selection(int id);
void clear_selection(void);
int  model_select_rect(int x1, int y1, int x2, int y2, int add);
void model_translate_selection(int dx, int dy);
int  model_delete_selection(void);
int  model_duplicate_selection(int dx, int dy);

/* ---- State drag kotak seleksi (events.c, digambar di draw.c) ----
   box_selecting = 1 saat user menyeret kotak blok; sudut-sudut dalam
   koordinat canvas. model_select_rect() dipanggil saat mouse-up. */
extern int box_selecting;
extern int box_x1, box_y1, box_x2, box_y2;

/* ---- Drag multi (grup terseleksi ikut bergerak) ----
   group_dragging = 1 saat grup hasil seleksi sedang diseret.
   gd_* menyimpan posisi awal tiap anggota + offset grab (dipakai
   on_mouse_move agar seluruh blok bergerak rigid tanpa akumulasi error). */
#define MAX_GROUP_DRAG 256
typedef struct { int idx; int x0, y0; } GroupDragItem;
extern int group_dragging;
extern GroupDragItem group_drag_items[MAX_GROUP_DRAG];
extern int group_drag_n;
extern int group_drag_ox, group_drag_oy; /* selisih pusat-grab (pixel mentah) */

/* Tombol toolbar (atas) */
extern const ToolKind btnTools[BTN_COUNT];
extern const wchar_t *btnLabels[BTN_COUNT]; /* nama tool = tooltip/status */
#define CLEAR_BTN_INDEX BTN_COUNT       /* tombol aksi Clear */
#define SAVE_BTN_INDEX  (BTN_COUNT + 1) /* tombol aksi Save JSON */
#define OPEN_BTN_INDEX  (BTN_COUNT + 2) /* tombol aksi Open JSON */
#define RUN_BTN_INDEX   (BTN_COUNT + 3) /* tombol aksi Run/Stop simulasi */

/* Toolbar: hit-test & gambar (toolbar_palette.c + toolbar_actions.c).
   toolbar_hit: -1 = tidak kena; 0..BTN_COUNT-1 = indeks tool;
   CLEAR/SAVE/OPEN_BTN_INDEX = tombol aksi. */
int toolbar_hit(int x, int y);
int toolbar_tool_at(int x, int y, int *is_clear);
void draw_toolbar(HDC hdc, int W);

/* ===================== Menu Bar klasik (File/Edit/View/Simulasi/Bantu) ====
   Digambar manual di strip paling atas window (tinggi MENUBAR_H), DI ATAS
   toolbar tombol komponen. Koordinat fungsi ini memakai koordinat client
   window utuh (belum di-offset). — toolbar_menubar.c */
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
#define IDM_EDIT_SELECTALL 0x1115  /* Edit > Pilih Semua (Ctrl+A) */
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
/* Setengah lebar/tinggi kotak pembatas komponen (memperhitungkan rotasi &
   bentuk khusus transistor). Dipakai hit-test badan, blok seleksi, dan
   gambar kotak seleksi grup. */
void comp_bounding_half(const Component *c, int *hw, int *hh);
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

/* Peta nama tipe <-> string JSON (fileio.c). Nama kanonik dipakai model_save_json;
   from_name menerima alias lama ("R", "SW", "BAT", "GND", "NPN", "CP", ...).
   Diakses publik agar round-trip JSON bisa diuji dari unit test. */
const char *comp_type_json_name(CompType t);
int         comp_type_from_json_name(const char *s);

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
void draw_k_ldr(HDC hdc, Component *c);
void draw_k_buzzer(HDC hdc, Component *c);

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
/* NOTE: sim_buzzer_on() & sim_ldr_effective_r() dideklarasikan di blok
   "Test seam" bagian paling bawah header ini (implementasi produksi masih
   ditunda — lihat tests/stubs_production.c). */
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

/* ===================== Test seam ==========================================
   Dua fungsi di bawah dipanggil oleh kode produksi (events.c & komponen_
   buzzer.c) tetapi implementasinya BELUM ada di src/ (sim.c belum menangani
   tipe COMP_LDR / COMP_BUZZER). Blok ini menyediakan:
     - mode PRODUKSI (default): stub kuning + TODO agar aplikasi tetap bisa
       di-link; lihat tests/stubs_production.c.
     - mode TEST (SCHEMATIC_TEST_STUBS=1, dipakai unit test): stub yang bisa
       dioverride lewat function pointer g_*_impl sehingga perilaku tiap
       fungsi bisa disuntik per-test. Lihat tests/unit_stubs.c. */
#ifndef SCHEMATIC_TEST_STUBS

int sim_buzzer_on(const Component *c);    /* 1 = buzzer ini aktif (berbunyi) saat RUN */
double sim_ldr_effective_r(const Component *c); /* R efektif LDR: gelap=v, terang~1% v; -1 bukan LDR */

#else /* SCHEMATIC_TEST_STUBS */

typedef int    (*sim_buzzer_on_fn)(const Component *c);
typedef double (*sim_ldr_effective_r_fn)(const Component *c);
extern sim_buzzer_on_fn        g_sim_buzzer_on_impl;        /* default = stub kuning */
extern sim_ldr_effective_r_fn  g_sim_ldr_effective_r_impl;  /* default = stub kuning */
static inline int sim_buzzer_on(const Component *c) { return g_sim_buzzer_on_impl(c); }
static inline double sim_ldr_effective_r(const Component *c) { return g_sim_ldr_effective_r_impl(c); }

#endif /* SCHEMATIC_TEST_STUBS */

#endif /* SCHEMATIC_H */
