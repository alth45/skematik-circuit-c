#include "schematic.h"

/* ===================== Simbol TRANSISTOR NPN (transistor.c) =====================
   Transistor NPN: lingkaran + bar basis vertikal.
   Titik pin (lihat comp_pin_pos di model.c) tepat di ujung kaki:
   pin0 = Kolektor -> ujung kaki kiri-atas  (-40,-30)
   pin1 = Emitor   -> ujung kaki kanan-bawah (+40,+30), panah keluar
   pin2 = Basis    -> ujung kawat basis kiri (-40,  0) */

static void draw_transistor(HDC hdc, int x, int y, BOOL sel, int tr_closed)
{
    // Warna simbol mengikuti status simulasi
    COLORREF col = sel
        ? RGB(50, 128, 255)
        : RGB(20, 20, 20);

    if (sim_running && tr_closed)
        col = RGB(20, 150, 60);

    HPEN pen = CreatePen(PS_SOLID, 2, col);
    if (!pen) return;

    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(
        hdc, GetStockObject(NULL_BRUSH));

    // Simpan kondisi GDI agar fungsi tidak merusak state pemanggil
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldTextColor = GetTextColor(hdc);

    // =====================================================
    // 1. Lingkaran badan transistor
    // =====================================================
    Ellipse(hdc, x - 21, y - 21, x + 21, y + 21);

    // =====================================================
    // 2. Basis: kawat horizontal dari pin (-40,0) sampai bar
    // =====================================================
    MoveToEx(hdc, x - 40, y, NULL);
    LineTo(hdc, x - 6, y);

    // Bar basis vertikal (batang tengah simbol)
    MoveToEx(hdc, x - 6, y - 12, NULL);
    LineTo(hdc, x - 6, y + 12);

    // =====================================================
    // 3. Kolektor: kaki diagonal dari bar basis ke pin kiri
    //    (baris x=-14 s.d. -40), tembus tepi lingkaran.
    // =====================================================
    MoveToEx(hdc, x - 2, y - 7, NULL);
    LineTo(hdc, x - 14, y - 15);
    LineTo(hdc, x - 40, y - 30);

    // =====================================================
    // 4. Emitor: kaki diagonal dari bar basis ke pin kanan
    //    (baris x=+14 s.d. +40), tembus tepi lingkaran.
    // =====================================================
    MoveToEx(hdc, x - 2, y + 7, NULL);
    LineTo(hdc, x + 14, y + 15);
    LineTo(hdc, x + 40, y + 30);

    // =====================================================
    // 5. Panah NPN: mengarah keluar pada jalur emitor.
    //    Kepala panah dihitung SEJAJAR garis kaki emitor
    //    (arah vektor (26,15)) sehingga menempel presisi
    //    di tengah-tengah kaki, tidak lagi miring/terlepas.
    // =====================================================
    const double dxk = 26.0, dyk = 15.0;          /* arah kaki emitor (14->40, 15->30) */
    double len = sqrt(dxk * dxk + dyk * dyk);     /* panjang vektor arah               */
    double ux = dxk / len, uy = dyk / len;        /* vektor satuan ke ujung (keluar)   */
    double nx = -uy, ny = ux;                     /* normal tegak lurus (lebar panah)  */
    double hx = x + 30, hy = y + 24.15;           /* titik ujung kepala PERSIS di garis kaki (t=0.6154) */
    const double HAL = 9.0, HAW = 3.6;            /* panjang & setengah lebar kepala   */

    POINT arrow[3] = {
        { (LONG)lround(hx - ux * HAL + nx * HAW), (LONG)lround(hy - uy * HAL + ny * HAW) },
        { (LONG)lround(hx - ux * HAL - nx * HAW), (LONG)lround(hy - uy * HAL - ny * HAW) },
        { (LONG)lround(hx),                       (LONG)lround(hy) }
    };

    // Isi kepala panah agar tampak tegas
    HBRUSH arrowBrush = CreateSolidBrush(col);
    if (arrowBrush) {
        HBRUSH previousBrush =
            (HBRUSH)SelectObject(hdc, arrowBrush);

        Polygon(hdc, arrow, 3);

        SelectObject(hdc, previousBrush);
        DeleteObject(arrowBrush);
    }

    // =====================================================
    // 6. Label transistor (di kanan-atas, jauh dari kaki emitor)
    // =====================================================
    SetTextColor(hdc, RGB(80, 80, 80));
    TextOutW(hdc, x + 27, y - 34, L"Q", 1);

    // Status simulasi
    if (sim_running) {
        const wchar_t *st = tr_closed ? L"ON" : L"OFF";

        SetTextColor(
            hdc,
            tr_closed
                ? RGB(20, 150, 60)
                : RGB(200, 40, 40));

        TextOutW(
            hdc,
            x - 10,
            y - 39,
            st,
            (int)wcslen(st));
    }

    // =====================================================
    // 7. Pulihkan seluruh state GDI
    // =====================================================
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    DeleteObject(pen);
}

/* Wrapper status-run (butuh param tambahan tr_closed) via variabel statis. */
static int tr_at_closed = 0;
static void tr_at(HDC hdc, int x, int y, BOOL sel) {
    draw_transistor(hdc, x, y, sel, tr_at_closed);
}

void draw_k_transistor(HDC hdc, Component *c) {
    tr_at_closed = sim_running ? sim_comp_conducting(c) : 0;
    with_rotation(hdc, c, tr_at);
}
