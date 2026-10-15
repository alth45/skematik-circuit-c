#include "schematic.h"

/* ===================== Simbol LDR / Fotoresistor (komponen_ldr.c) =====================
   Badan resistor zig-zag dengan dua panah CAHAYA masuk (menunjuk ke badan),
   dibungkus lingkaran sensitif cahaya. pin0 = kiri, pin1 = kanan.
   Saat RUN dianggap "terang": R efektif = nilai gelap x LDR_LIGHT_RATIO
   (lihat sim_ldr_effective di sim.c). Panah digambar membesar saat RUN
   sebagai indikasi cahaya menyinari. */

static void draw_ldr(HDC hdc, int x, int y, BOOL sel, int run) {
    COLORREF col = sel ? RGB(50,128,255) : RGB(20,140,60); /* hijau = fotosensitif */
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* kawat terminal */
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-22, y);
    MoveToEx(hdc, x+22, y, NULL); LineTo(hdc, x+40, y);

    /* lingkaran badan sensor */
    Ellipse(hdc, x-22, y-16, x+22, y+16);

    /* zig-zag resistansi di dalam lingkaran */
    MoveToEx(hdc, x-22, y, NULL);
    LineTo(hdc, x-15, y-8);
    LineTo(hdc, x-7,  y+8);
    LineTo(hdc, x+1,  y-8);
    LineTo(hdc, x+9,  y+8);
    LineTo(hdc, x+16, y-6);
    LineTo(hdc, x+22, y);

    /* dua panah cahaya MASUK (dari kiri-atas & kiri-bawah menuju badan);
       ujung anak panah menunjuk ke badan -> kebalikan LED */
    int alen = run ? 14 : 10;      /* panah membesar saat RUN (tersinari) */
    /* panah atas */
    MoveToEx(hdc, x-30, y-30, NULL); LineTo(hdc, x-14, y-16);
    MoveToEx(hdc, x-14, y-16, NULL); LineTo(hdc, x-14-alen/3, y-16);
    MoveToEx(hdc, x-14, y-16, NULL); LineTo(hdc, x-14, y-16+alen/3);
    /* panah bawah */
    MoveToEx(hdc, x-30, y+30, NULL); LineTo(hdc, x-14, y+16);
    MoveToEx(hdc, x-14, y+16, NULL); LineTo(hdc, x-14-alen/3, y+16);
    MoveToEx(hdc, x-14, y+16, NULL); LineTo(hdc, x-14, y+16-alen/3);

    SelectObject(hdc, oldb);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

static int ldr_at_run = 0;
static void ldr_at(HDC hdc, int x, int y, BOOL sel) {
    draw_ldr(hdc, x, y, sel, ldr_at_run);
}

void draw_k_ldr(HDC hdc, Component *c) {
    ldr_at_run = sim_running;
    with_rotation(hdc, c, ldr_at);
}
