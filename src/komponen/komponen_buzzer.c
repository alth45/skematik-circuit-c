#include "schematic.h"

/* ===================== Simbol Buzzer (komponen_buzzer.c) ===================
   Simbol semi-sirkular khas buzzer piezo dengan garis basis.
   Dua pin: pin0 = kiri (input/+), pin1 = kanan (gnd/-).
   Nilai v = frekuensi bunyi (Hz, default 1000).
   Saat RUN dan buzzer aktif (berbunyi): warna hijau + gelombang suara. */

static void draw_buzzer(HDC hdc, int x, int y, BOOL sel, int on) {
    COLORREF col = (sim_running && on) ? RGB(30,160,60)
                 : sel ? RGB(50,128,255) : RGB(20,20,20);
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* kawat lead kiri & kanan */
    MoveToEx(hdc, x - 40, y, NULL); LineTo(hdc, x - 14, y);
    MoveToEx(hdc, x + 14, y, NULL); LineTo(hdc, x + 40, y);

    /* mangkuk semi-sirkular (dome) menghadap kanan + garis basis vertikal */
    Arc(hdc, x - 14, y - 20, x + 26, y + 20, x - 14, y - 20, x - 14, y + 20);
    MoveToEx(hdc, x - 14, y - 20, NULL); LineTo(hdc, x - 14, y + 20);

    /* indikator bunyi: tiga busur kecil di sisi kanan */
    if (sim_running && on) {
        Arc(hdc, x + 18, y - 10, x + 30, y + 10, x + 24, y - 6, x + 24, y + 6);
        Arc(hdc, x + 22, y - 15, x + 38, y + 15, x + 30, y - 9, x + 30, y + 9);
        Arc(hdc, x + 26, y - 20, x + 46, y + 20, x + 36, y - 12, x + 36, y + 12);
    } else {
        Arc(hdc, x + 18, y - 8,  x + 26, y + 8,  x + 22, y - 5, x + 22, y + 5);
        Arc(hdc, x + 22, y - 12, x + 34, y + 12, x + 28, y - 7, x + 28, y + 7);
    }

    SelectObject(hdc, oldb);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

/* Wrapper status-run (butuh param tambahan on) via variabel statis. */
static int buz_at_on = 0;
static void buz_at(HDC hdc, int x, int y, BOOL sel) {
    draw_buzzer(hdc, x, y, sel, buz_at_on);
}

void draw_k_buzzer(HDC hdc, Component *c) {
    buz_at_on = sim_buzzer_on(c);
    with_rotation(hdc, c, buz_at);
}
