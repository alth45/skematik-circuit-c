#include "schematic.h"

/* ===================== Simbol LED (komponen_led.c) ===================== */

static void draw_led(HDC hdc, int x, int y, BOOL sel, int lit) {
    COLORREF col = (sim_running && lit) ? RGB(255,140,0)
                 : sel ? RGB(50,128,255) : RGB(200,30,30);
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    /* glow saat menyala */
    if (sim_running && lit) {
        HBRUSH glow = CreateSolidBrush(RGB(255,220,120));
        HBRUSH og = (HBRUSH)SelectObject(hdc, glow);
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(255,180,60));
        HPEN og2 = (HPEN)SelectObject(hdc, gp);
        Ellipse(hdc, x-22, y-25, x+22, y+25);
        SelectObject(hdc, og2);
        SelectObject(hdc, og);
        DeleteObject(gp);
        DeleteObject(glow);
        SelectObject(hdc, pen);
    }
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-12, y);
    MoveToEx(hdc, x+12, y, NULL); LineTo(hdc, x+40, y);

    // segitiga dioda
    POINT tri[3] = { {x-12, y-15}, {x-12, y+15}, {x+12, y} };
    Polygon(hdc, tri, 3);

    // garis katoda
    MoveToEx(hdc, x+12, y-15, NULL); LineTo(hdc, x+12, y+15);

    // panah cahaya
    MoveToEx(hdc, x-5, y-18, NULL); LineTo(hdc, x-12, y-30);
    MoveToEx(hdc, x+3, y-18, NULL); LineTo(hdc, x-4, y-30);

    SelectObject(hdc, oldb);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

/* Wrapper status-run (butuh param tambahan lit) via variabel statis. */
static int led_at_lit = 0;
static void led_at(HDC hdc, int x, int y, BOOL sel) {
    draw_led(hdc, x, y, sel, led_at_lit);
}

void draw_k_led(HDC hdc, Component *c) {
    led_at_lit = sim_comp_lit(c);
    with_rotation(hdc, c, led_at);
}
