#include "schematic.h"

/* ===================== Simbol Buzzer (komponen_buzzer.c) =====================
   Setengah lingkaran (mangkuk) menghadap kiri dengan garis datar di kanannya,
   plus dua gelombang suara. pin0 = (+) kiri, pin1 = (-) kanan.
   Saat RUN dan berbunyi: warna oranye + gelombang suara menebal. */

static void draw_buzzer(HDC hdc, int x, int y, BOOL sel, int sound) {
    COLORREF col = (sim_running && sound) ? RGB(255,140,0)
                 : sel ? RGB(50,128,255) : RGB(150,60,160); /* ungu */
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* kawat terminal */
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-14, y);
    MoveToEx(hdc, x+14, y, NULL); LineTo(hdc, x+40, y);

    /* mangkuk setengah lingkaran: busur dari atas-kanan ke bawah-kanan */
    Arc(hdc, x-14, y-20, x+14, y+20, x+14, y-20, x+14, y+20);
    /* plat datar di sisi kanan */
    MoveToEx(hdc, x+14, y-20, NULL); LineTo(hdc, x+14, y+20);

    /* tanda "+" dekat pin0 dan "-" dekat pin1 */
    SelectObject(hdc, old);
    HPEN tp = CreatePen(PS_SOLID, 1, col);
    HPEN otp = (HPEN)SelectObject(hdc, tp);
    MoveToEx(hdc, x-24, y-10, NULL); LineTo(hdc, x-16, y-10);
    MoveToEx(hdc, x-20, y-14, NULL); LineTo(hdc, x-20, y-6);
    MoveToEx(hdc, x+18, y+10, NULL); LineTo(hdc, x+26, y+10);
    SelectObject(hdc, otp);
    DeleteObject(tp);
    SelectObject(hdc, pen);

    /* gelombang suara (dua arc kecil di kanan); tebal saat berbunyi */
    {
        HPEN wp = CreatePen(PS_SOLID, sound ? 3 : 1, col);
        HPEN owp = (HPEN)SelectObject(hdc, wp);
        Arc(hdc, x+18, y-10, x+28, y+10, x+23, y-10, x+23, y+10);
        Arc(hdc, x+24, y-14, x+38, y+14, x+31, y-14, x+31, y+14);
        SelectObject(hdc, owp);
        DeleteObject(wp);
    }

    SelectObject(hdc, oldb);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

static int buz_at_sound = 0;
static void buz_at(HDC hdc, int x, int y, BOOL sel) {
    draw_buzzer(hdc, x, y, sel, buz_at_sound);
}

void draw_k_buzzer(HDC hdc, Component *c) {
    buz_at_sound = sim_comp_sound(c);
    with_rotation(hdc, c, buz_at);
}
