#include "schematic.h"

/* ===================== draw.c: Dispatcher Gambar & Utilitas Bersama =====================
   Simbol tiap komponen kini dipisah ke file sendiri-sendiri:
     komponen_resistor.c  -> draw_k_resistor
     komponen_switch.c    -> draw_k_switch
     komponen_battery.c   -> draw_k_battery
     komponen_led.c       -> draw_k_led
     komponen_ground.c    -> draw_k_ground
     komponen_capacitor.c -> draw_k_capacitor
     transistor.c         -> draw_k_transistor
     komponen_diode.c     -> draw_k_diode
     komponen_capolar.c   -> draw_k_capolar
   File ini hanya berisi: utilitas rotasi dunia (with_rotation), dispatcher
   draw_component, label nilai, titik pin, kabel, dan canvas. */

/* Bungkus gambar simbol dengan rotasi dunia (GDI World Transform).
   Semua fungsi draw_* di file komponen menggambar asumsi rot=0 (horizontal). */
void with_rotation(HDC hdc, Component *c,
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

/* Label NILAI komponen (R/BAT/LED/C/CP/D) — digambar TANPA rotasi agar
   teks selalu tegak dan mudah dibaca. Dipanggil dari draw_component. */
static void draw_value_label(HDC hdc, Component *c) {
    if (!comp_has_value(c->type)) return;

    const wchar_t *unit = comp_value_unit(c->type);
    wchar_t vs[VALUE_STR_CAP];
    format_value(c->v, unit, vs, VALUE_STR_CAP);

    HFONT hf = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT of = (HFONT)SelectObject(hdc, hf);
    SetBkMode(hdc, TRANSPARENT);

    SIZE sz; GetTextExtentPoint32W(hdc, vs, (int)wcslen(vs), &sz);
    int tw = sz.cx, th = sz.cy;

    /* posisi default: horizontal -> atas; vertikal -> samping kiri/kanan */
    int tx = c->x - tw / 2;
    int ty = (c->rot % 2 == 0) ? c->y - 34 : c->y - th / 2;
    if (c->rot % 2 != 0) {
        /* vertikal: taruh di sisi yang kosong (kiri untuk rot 1, kanan rot 3) */
        if ((c->rot % 4) == 1) tx = c->x - tw - 52;
        else                   tx = c->x + 52;
    }

    /* kotak putih tipis agar terbaca di atas grid/kabel */
    HPEN bp = CreatePen(PS_SOLID, 1, RGB(220,220,220));
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    HBRUSH bb = CreateSolidBrush(RGB(255,255,255));
    HBRUSH obb = (HBRUSH)SelectObject(hdc, bb);
    Rectangle(hdc, tx - 4, ty - 2, tx + tw + 4, ty + th + 2);
    SelectObject(hdc, obb); SelectObject(hdc, obp);
    DeleteObject(bb); DeleteObject(bp);

    COLORREF col = RGB(70, 70, 70);
    if (sim_running && c->type == COMP_LED && sim_comp_blown(c))
        col = RGB(200, 30, 30);                 /* LED meledak -> merah */
    SetTextColor(hdc, col);
    TextOutW(hdc, tx, ty, vs, (int)wcslen(vs));

    SelectObject(hdc, of);
    DeleteObject(hf);
}

/* ===================== Dispatch Komponen (ke file simbol masing-masing) ===================== */
void draw_component(HDC hdc, Component *c) {
    BOOL sel = (c->id == selected_id);
    switch (c->type) {
        case COMP_RESISTOR:    draw_k_resistor(hdc, c);    break;
        case COMP_SWITCH:      draw_k_switch(hdc, c);      break;
        case COMP_BATTERY:     draw_k_battery(hdc, c);     break;
        case COMP_LED:         draw_k_led(hdc, c);         break;
        case COMP_GROUND:      draw_k_ground(hdc, c);      break;
        case COMP_CAPACITOR:   draw_k_capacitor(hdc, c);   break;
        case COMP_TRANSISTOR:  draw_k_transistor(hdc, c);  break;
        case COMP_DIODE:       draw_k_diode(hdc, c);       break;
        case COMP_CAPOLAR:     draw_k_capolar(hdc, c);     break;
        default: break;
    }
    draw_value_label(hdc, c);   /* teks nilai selalu tegak (tanpa rotasi) */
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
