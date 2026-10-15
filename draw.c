#include "schematic.h"

/* ===================== Gambar Simbol dengan GDI ===================== */

/* Bungkus gambar simbol dengan rotasi dunia (GDI World Transform).
   Semua fungsi draw_* lokal menggambar asumsi rot=0 (horizontal). */
static void with_rotation(HDC hdc, Component *c,
                          void (*fn)(HDC, int, int, BOOL)) {
    int r = ((c->rot % 4) + 4) % 4;
    if (r == 0) { fn(hdc, c->x, c->y, c->id == selected_id); return; }
    int saved = SaveDC(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xf;
    /* peta: local (lx,ly) -> world (cx + rx, cy + ry), dgn (rx,ry) spt comp_local_to_world.
       r=1: (x,y)->(-y,x): eM11=0,eM12=1,eM21=-1,eM22=0
       r=2: (x,y)->(-x,-y): eM11=-1,eM12=0,eM21=0,eM22=-1
       r=3: (x,y)->(y,-x): eM11=0,eM12=-1,eM21=1,eM22=0
       world = M*local + (cx,cy); jadi gambar di (0,0) = pusat. */
    if (r == 1)      { xf.eM11 = 0;  xf.eM12 = 1;  xf.eM21 = -1; xf.eM22 = 0; }
    else if (r == 2) { xf.eM11 = -1; xf.eM12 = 0;  xf.eM21 = 0;  xf.eM22 = -1; }
    else             { xf.eM11 = 0;  xf.eM12 = -1; xf.eM21 = 1;  xf.eM22 = 0; }
    xf.eDx = (FLOAT)c->x;
    xf.eDy = (FLOAT)c->y;
    SetWorldTransform(hdc, &xf);
    fn(hdc, 0, 0, c->id == selected_id);
    RestoreDC(hdc, saved);
}

/* Wrapper status-run untuk switch & LED (butuh param tambahan).
   Bodinya di bawah setelah draw_switch/draw_led didefinisikan. */
static int sw_at_on = 0;
static int led_at_lit = 0;
static int tr_at_closed = 0;
static void with_rotation_sw(HDC hdc, Component *c);
static void with_rotation_led(HDC hdc, Component *c);
static void with_rotation_tr(HDC hdc, Component *c);


/* Semua draw_* menerima state run: sw_on (saklar), lit (LED menyala). */
static void draw_resistor(HDC hdc, int x, int y, BOOL sel) {
    HPEN pen = CreatePen(PS_SOLID, 2, sel ? RGB(50,128,255) : RGB(20,20,20));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HPEN oldBrush = (HPEN)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    // lead kiri & kanan
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-20, y);
    MoveToEx(hdc, x+20, y, NULL); LineTo(hdc, x+40, y);
    // kotak
    Rectangle(hdc, x-20, y-10, x+20, y+10);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, old);
    DeleteObject(pen);

    SetTextColor(hdc, RGB(80,80,80));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-6, y-22, L"R", 1);
}

static void draw_switch(HDC hdc, int x, int y, BOOL sel, int sw_on) {
    COLORREF col = sim_running
        ? (sw_on ? RGB(20,150,60) : RGB(200,40,40))
        : (sel ? RGB(50,128,255) : RGB(20,20,20));
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH br = CreateSolidBrush(col);
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, br);

    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-15, y);
    MoveToEx(hdc, x+15, y, NULL); LineTo(hdc, x+40, y);

    // titik kontak
    Ellipse(hdc, x-18, y-3, x-12, y+3);
    Ellipse(hdc, x+12, y-3, x+18, y+3);

    SelectObject(hdc, oldb);
    DeleteObject(br);

    // tuas: tertutup = horizontal, terbuka = miring ke atas
    if (sw_on) { MoveToEx(hdc, x-15, y, NULL); LineTo(hdc, x+15, y); }
    else       { MoveToEx(hdc, x-15, y, NULL); LineTo(hdc, x+12, y-15); }

    SelectObject(hdc, old);
    DeleteObject(pen);

    SetTextColor(hdc, RGB(80,80,80));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-10, y-28, L"SW", 2);
    if (sim_running) {
        const wchar_t *st = sw_on ? L"ON" : L"OFF";
        SetTextColor(hdc, sw_on ? RGB(20,150,60) : RGB(200,40,40));
        TextOutW(hdc, x-10, y+14, st, (int)wcslen(st));
        if (sel) {
            SetTextColor(hdc, RGB(50,128,255));
            TextOutW(hdc, x-24, y-44, L"klik: ON/OFF", 12);
        }
    }
}

static void draw_battery(HDC hdc, int x, int y, BOOL sel) {
    HPEN pen = CreatePen(PS_SOLID, 2, sel ? RGB(50,128,255) : RGB(20,20,20));
    HPEN old = (HPEN)SelectObject(hdc, pen);

    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-10, y);
    MoveToEx(hdc, x+10, y, NULL); LineTo(hdc, x+40, y);

    // plat panjang (+)
    MoveToEx(hdc, x-10, y-15, NULL); LineTo(hdc, x-10, y+15);
    // plat pendek (-)
    MoveToEx(hdc, x+10, y-8, NULL);  LineTo(hdc, x+10, y+8);

    SelectObject(hdc, old);
    DeleteObject(pen);

    SetTextColor(hdc, RGB(80,80,80));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-10, y-30, L"+  -", 4);
    TextOutW(hdc, x-14, y+22, L"BAT", 3);
}

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

static void draw_ground(HDC hdc, int x, int y, BOOL sel) {
    HPEN pen = CreatePen(PS_SOLID, 2, sel ? RGB(50,128,255) : RGB(20,20,20));
    HPEN old = (HPEN)SelectObject(hdc, pen);

    MoveToEx(hdc, x, y-30, NULL); LineTo(hdc, x, y);
    MoveToEx(hdc, x-20, y, NULL); LineTo(hdc, x+20, y);
    MoveToEx(hdc, x-12, y+8, NULL); LineTo(hdc, x+12, y+8);
    MoveToEx(hdc, x-5, y+16, NULL); LineTo(hdc, x+5, y+16);

    SelectObject(hdc, old);
    DeleteObject(pen);
}

/* ===================== Dispatch Komponen ===================== */
static void sw_at(HDC hdc, int x, int y, BOOL sel) {
    draw_switch(hdc, x, y, sel, sw_at_on);
}
static void draw_capacitor(HDC hdc, int x, int y, BOOL sel) {
    HPEN pen = CreatePen(PS_SOLID, 2, sel ? RGB(50,128,255) : RGB(20,20,20));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-6, y);
    MoveToEx(hdc, x+6, y, NULL); LineTo(hdc, x+40, y);
    MoveToEx(hdc, x-6, y-15, NULL); LineTo(hdc, x-6, y+15);
    MoveToEx(hdc, x+6, y-15, NULL); LineTo(hdc, x+6, y+15);
    SelectObject(hdc, ob);
    SelectObject(hdc, old);
    DeleteObject(pen);
    SetTextColor(hdc, RGB(80,80,80));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-4, y-28, L"C", 1);
}

/* Dioda standar: segitiga anoda -> garis katoda.
   Arah sama dgn LED: pin0 = Anoda (kiri), pin1 = Katoda (kanan). */
static void draw_diode(HDC hdc, int x, int y, BOOL sel, int fwd) {
    COLORREF col = sel ? RGB(50,128,255) : RGB(20,20,20);
    if (sim_running && fwd) col = RGB(20,150,60);   /* menghantar = hijau */
    HPEN pen = CreatePen(PS_SOLID, 2, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* kawat lead ke titik pin (-40 / +40) */
    MoveToEx(hdc, x-40, y, NULL); LineTo(hdc, x-12, y);
    MoveToEx(hdc, x+12, y, NULL); LineTo(hdc, x+40, y);

    /* segitiga anoda */
    POINT tri[3] = { {x-12, y-15}, {x-12, y+15}, {x+12, y} };
    Polygon(hdc, tri, 3);

    /* garis katoda (menempel persis di ujung kanan segitiga) */
    MoveToEx(hdc, x+12, y-15, NULL); LineTo(hdc, x+12, y+15);

    SelectObject(hdc, ob);
    SelectObject(hdc, old);
    DeleteObject(pen);

    SetTextColor(hdc, RGB(80,80,80));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x-4, y-30, L"D", 1);

    /* saat RUN & forward: panah arah alir A->K di bawah simbol */
    if (sim_running && fwd) {
        HPEN ap = CreatePen(PS_SOLID, 2, RGB(20,150,60));
        HPEN oap = (HPEN)SelectObject(hdc, ap);
        MoveToEx(hdc, x-14, y+22, NULL); LineTo(hdc, x+10, y+22);
        MoveToEx(hdc, x+10, y+22, NULL); LineTo(hdc, x+4,  y+18);
        MoveToEx(hdc, x+10, y+22, NULL); LineTo(hdc, x+4,  y+26);
        SelectObject(hdc, oap);
        DeleteObject(ap);
    }
}

/* Wrapper status-run untuk dioda (param tambahan: fwd) */
static int diode_at_fwd = 0;
static void diode_at(HDC hdc, int x, int y, BOOL sel) {
    draw_diode(hdc, x, y, sel, diode_at_fwd);
}
static void with_rotation_diode(HDC hdc, Component *c) {
    diode_at_fwd = sim_running ? sim_diode_fwd(c) : 0;
    with_rotation(hdc, c, diode_at);
}

/* Kapasitor polar (elektrolit): plat lengkung kutub (-) + tanda (+) kutub.
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

/* Transistor NPN: lingkaran + bar basis vertikal.
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


static void led_at(HDC hdc, int x, int y, BOOL sel) {
    draw_led(hdc, x, y, sel, led_at_lit);
}
static void tr_at(HDC hdc, int x, int y, BOOL sel) {
    draw_transistor(hdc, x, y, sel, tr_at_closed);
}
static void with_rotation_sw(HDC hdc, Component *c) {
    sw_at_on = c->on;
    with_rotation(hdc, c, sw_at);
}
static void with_rotation_led(HDC hdc, Component *c) {
    led_at_lit = sim_comp_lit(c);
    with_rotation(hdc, c, led_at);
}
static void with_rotation_tr(HDC hdc, Component *c) {
    tr_at_closed = sim_running ? sim_comp_conducting(c) : 0;
    with_rotation(hdc, c, tr_at);
}

static void draw_pins(HDC hdc, Component *c) {
    int n = comp_pin_count(c);
    int show = (current_tool == TOOL_WIRE || wiring_active || c->id == selected_id);
    for (int p = 0; p < n; p++) {
        int px, py;
        comp_pin_pos(c, p, &px, &py);
        HBRUSH br = CreateSolidBrush(show ? RGB(220, 60, 60) : RGB(150,150,150));
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, br);
        HPEN pen = CreatePen(PS_SOLID, 1, show ? RGB(160,20,20) : RGB(120,120,120));
        HPEN oldp = (HPEN)SelectObject(hdc, pen);
        Ellipse(hdc, px - PIN_R, py - PIN_R, px + PIN_R, py + PIN_R);
        SelectObject(hdc, oldp);
        SelectObject(hdc, oldb);
        DeleteObject(pen);
        DeleteObject(br);
    }
}

void draw_component(HDC hdc, Component *c) {
    BOOL sel = (c->id == selected_id);
    switch (c->type) {
        case COMP_RESISTOR: with_rotation(hdc, c, draw_resistor); break;
        case COMP_SWITCH:   with_rotation_sw(hdc, c); break;
        case COMP_BATTERY:  with_rotation(hdc, c, draw_battery); break;
        case COMP_LED:      with_rotation_led(hdc, c); break;
        case COMP_GROUND:   with_rotation(hdc, c, draw_ground); break;
        case COMP_CAPACITOR: with_rotation(hdc, c, draw_capacitor); break;
        case COMP_TRANSISTOR: with_rotation_tr(hdc, c); break;
        case COMP_DIODE:    with_rotation_diode(hdc, c); break;
        case COMP_CAPOLAR:  with_rotation(hdc, c, draw_capolar); break;
        default: break;
    }
    if (sel) {
        HPEN pen = CreatePen(PS_DOT, 1, RGB(50,128,255));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        int hw = (c->rot % 2 == 0) ? 48 : 38;
        int hh = (c->rot % 2 == 0) ? 38 : 48;
        /* transistor: kaki C/E menjulur sampai y=+-30 & x=+-40,
           jadi kotak seleksinya dibuat lebih besar */
        if (c->type == COMP_TRANSISTOR) { hw = 52; hh = 42; }
        Rectangle(hdc, c->x - hw, c->y - hh, c->x + hw, c->y + hh);
        SelectObject(hdc, oldb);
        SelectObject(hdc, old);
        DeleteObject(pen);
        /* label sudut rotasi */
        wchar_t buf[16];
        swprintf(buf, 16, L"%d deg", (c->rot % 4) * 90);
        SetTextColor(hdc, RGB(50,128,255));
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, c->x - 24, c->y - hh - 18, buf, (int)wcslen(buf));
    }
    draw_pins(hdc, c);
}

/* ===================== Kabel ===================== */
static void stroke_ortho(HDC hdc, int x1, int y1, int x2, int y2) {
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y1);
    LineTo(hdc, x2, y2);
}

void draw_wires(HDC hdc) {
    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        Component *cb = find_comp_by_id(wires[i].b.comp_id);
        if (!ca || !cb) continue;
        int ax, ay, bx, by;
        comp_pin_pos(ca, wires[i].a.pin_idx, &ax, &ay);
        comp_pin_pos(cb, wires[i].b.pin_idx, &bx, &by);
        /* kabel bertegangan (+) = merah saat RUN */
        int hot = sim_running && sim_wire_hot(i);
        HPEN pen = CreatePen(PS_SOLID, hot ? 3 : 2,
                             hot ? RGB(230,50,50) : RGB(20,20,20));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        stroke_ortho(hdc, ax, ay, bx, by);
        HBRUSH dot = CreateSolidBrush(hot ? RGB(230,50,50) : RGB(20,20,20));
        HBRUSH od = (HBRUSH)SelectObject(hdc, dot);
        Ellipse(hdc, ax - 3, ay - 3, ax + 3, ay + 3);
        Ellipse(hdc, bx - 3, by - 3, bx + 3, by + 3);
        SelectObject(hdc, od);
        DeleteObject(dot);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    /* preview kabel saat drag */
    if (wiring_active) {
        Component *ca = find_comp_by_id(wiring_from.comp_id);
        if (ca) {
            int ax, ay;
            comp_pin_pos(ca, wiring_from.pin_idx, &ax, &ay);
            HPEN pv = CreatePen(PS_DOT, 2, RGB(220, 60, 60));
            HPEN opv = (HPEN)SelectObject(hdc, pv);
            int mx = snap(wiring_mx), my = snap(wiring_my);
            stroke_ortho(hdc, ax, ay, mx, my);
            SelectObject(hdc, opv);
            DeleteObject(pv);
            /* highlight pin target */
            PinRef t; int tx, ty;
            if (find_pin_at(wiring_mx, wiring_my, &t, &tx, &ty)) {
                HBRUSH br = CreateSolidBrush(RGB(60, 180, 60));
                HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
                Ellipse(hdc, tx - 10, ty - 10, tx + 10, ty + 10);
                SelectObject(hdc, ob);
                DeleteObject(br);
            }
        }
    }
}

/* ===================== Gambar Seluruh Canvas ===================== */
void draw_canvas(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;

    // Double buffer untuk menghilangkan flicker
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, W, H);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // background putih
    HBRUSH bg = CreateSolidBrush(RGB(255,255,255));
    FillRect(memDC, &rc, bg);
    DeleteObject(bg);

    // grid halus
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(235,235,235));
    HPEN oldp = (HPEN)SelectObject(memDC, gridPen);
    for (int x = 0; x < W; x += GRID) {
        MoveToEx(memDC, x, 0, NULL); LineTo(memDC, x, H);
    }
    for (int y = 0; y < H; y += GRID) {
        MoveToEx(memDC, 0, y, NULL); LineTo(memDC, W, y);
    }
    SelectObject(memDC, oldp);
    DeleteObject(gridPen);

    // gambar semua komponen
    for (int i = 0; i < comp_count; i++) {
        draw_component(memDC, &comps[i]);
    }

    BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}
