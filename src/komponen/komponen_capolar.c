#include "schematic.h"

/* ===================== Simbol KAPASITOR POLAR / ELEKTROLIT (komponen_capolar.c) =====================
   Plat lengkung kutub (-) + tanda (+) kutub.
   pin0 = (+) kiri, pin1 = (-) kanan; blokir DC seperti kapasitor biasa. */

static void draw_capolar(HDC hdc, int x, int y, BOOL sel) {
    COLORREF col = sel ? RGB(50,128,255) : RGB(20,20,20);
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* kawat lead ke titik pin (-40 / +40) */
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-6, y);
    MoveToEx(hdc, x+6, y, NULL); LineTo(hdc, x+40, y);

    /* plat kutub (+) = garis lurus */
    MoveToEx(hdc, x-6, y-15, NULL); LineTo(hdc, x-6, y+15);

    /* plat kutub (-) = lengkung cembung ke kanan (simbol elektrolit) */
    MoveToEx(hdc, x+6, y-15, NULL);
    Arc(hdc, x-24, y-30, x+42, y+30, x+6, y+15, x+6, y-15);

    SelectObject(hdc, ob);
    SelectObject(hdc, old);
    DeleteObject(pen);

    /* tanda polaritas: '+' dekat anoda, '-' dekat katoda */
    SetTextColor(hdc, sel ? RGB(50,128,255) : RGB(20,20,20));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-24, y-14, L"+", 1);
    TextOutW(hdc, x+16, y-14, L"-", 1);

    SetTextColor(hdc, RGB(80,80,80));
    TextOutW(hdc, x-8, y-30, L"CP", 2);
}

void draw_k_capolar(HDC hdc, Component *c) {
    with_rotation(hdc, c, draw_capolar);
}
