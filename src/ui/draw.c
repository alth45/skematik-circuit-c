#include "schematic.h"

/* ===================== draw.c: Dispatcher Gambar & Utilitas Bersama ===================== */

/* Font di-cache sekali per proses (bukan per paint / per komponen).
   Cache boleh NULL bila CreateFontW gagal -> retry pada pemanggilan
   berikutnya, tidak menyimpan kegagalan permanen. Tidak ada DeleteObject
   karena umur font = umur proses (GDI bersihkan otomatis saat exit). */
static HFONT font_value_label(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    return h;
}
static HFONT font_sel_label(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    return h;
}

static int comp_in_selection(int id);
static void draw_selection_boxes(HDC hdc);
static void draw_marquee(HDC hdc);

void with_rotation(HDC hdc, Component *c,
                   void (*fn)(HDC, int, int, BOOL)) {
    int r = ((c->rot % 4) + 4) % 4;
    if (r == 0) { fn(hdc, c->x, c->y, c->id == selected_id); return; }
    int saved = SaveDC(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xf;
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
    int show = (current_tool == TOOL_WIRE || wiring_active ||
                c->id == selected_id || comp_in_selection(c->id));
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

static void draw_value_label(HDC hdc, Component *c) {
    if (!comp_has_value(c->type)) return;

    const wchar_t *unit = comp_value_unit(c->type);
    wchar_t vs[VALUE_STR_CAP];
    format_value(c->v, unit, vs, VALUE_STR_CAP);

    HFONT hf = font_value_label();
    HFONT of = (HFONT)SelectObject(hdc, hf);
    SetBkMode(hdc, TRANSPARENT);

    SIZE sz; GetTextExtentPoint32W(hdc, vs, (int)wcslen(vs), &sz);
    int tw = sz.cx, th = sz.cy;

    int tx = c->x - tw / 2;
    int ty = (c->rot % 2 == 0) ? c->y - 34 : c->y - th / 2;
    if (c->rot % 2 != 0) {
        if ((c->rot % 4) == 1) tx = c->x - tw - 52;
        else                   tx = c->x + 52;
    }

    HPEN bp = CreatePen(PS_SOLID, 1, RGB(220,220,220));
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    HBRUSH bb = CreateSolidBrush(RGB(255,255,255));
    HBRUSH obb = (HBRUSH)SelectObject(hdc, bb);
    Rectangle(hdc, tx - 4, ty - 2, tx + tw + 4, ty + th + 2);
    SelectObject(hdc, obb); SelectObject(hdc, obp);
    DeleteObject(bb); DeleteObject(bp);

    COLORREF col = RGB(70, 70, 70);
    if (sim_running && c->type == COMP_LED && sim_comp_blown(c))
        col = RGB(200, 30, 30);
    SetTextColor(hdc, col);
    TextOutW(hdc, tx, ty, vs, (int)wcslen(vs));

    SelectObject(hdc, of);
    /* font di-cache: JANGAN DeleteObject(hf) */
}

void draw_component(HDC hdc, Component *c) {
    BOOL sel = (c->id == selected_id);
    switch (c->type) {
        case COMP_RESISTOR:    draw_k_resistor(hdc, c);    break;
        case COMP_SWITCH:      draw_k_switch(hdc, c);      break;
        case COMP_BATTERY:     draw_k_battery(hdc, c);     break;
        case COMP_ACSOURCE:    draw_k_acsource(hdc, c);    break;
        case COMP_LED:         draw_k_led(hdc, c);         break;
        case COMP_GROUND:      draw_k_ground(hdc, c);      break;
        case COMP_CAPACITOR:   draw_k_capacitor(hdc, c);   break;
        case COMP_TRANSISTOR:  draw_k_transistor(hdc, c);  break;
        case COMP_DIODE:       draw_k_diode(hdc, c);       break;
        case COMP_CAPOLAR:     draw_k_capolar(hdc, c);     break;
        case COMP_LDR:         draw_k_ldr(hdc, c);         break;
        case COMP_BUZZER:      draw_k_buzzer(hdc, c);      break;
        case COMP_VOLTMETER:   draw_k_voltmeter(hdc, c);   break;
        case COMP_AMMETER:     draw_k_ammeter(hdc, c);     break;
        default: break;
    }
    draw_value_label(hdc, c);
    if (sel) {
        HPEN pen = CreatePen(PS_DOT, 1, RGB(50,128,255));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        int hw = (c->rot % 2 == 0) ? 48 : 38;
        int hh = (c->rot % 2 == 0) ? 38 : 48;
        if (c->type == COMP_TRANSISTOR) { hw = 52; hh = 42; }
        Rectangle(hdc, c->x - hw, c->y - hh, c->x + hw, c->y + hh);
        SelectObject(hdc, oldb);
        SelectObject(hdc, old);
        DeleteObject(pen);
        wchar_t buf[16];
        swprintf(buf, 16, L"%d deg", (c->rot % 4) * 90);
        SetTextColor(hdc, RGB(50,128,255));
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, c->x - 24, c->y - hh - 18, buf, (int)wcslen(buf));
    }
    draw_pins(hdc, c);
}

void draw_selection_overlay(HDC hdc, HDC hdc_screen) {
    draw_selection_boxes(hdc);
    if (box_selecting) draw_marquee(hdc_screen);
}

static int comp_in_selection(int id) {
    for (int i = 0; i < sel_count; i++)
        if (selection[i] == id) return 1;
    return 0;
}

static void draw_selection_boxes(HDC hdc) {
    if (sel_count <= 1) return;
    int minx = 1 << 30, miny = 1 << 30, maxx = -(1 << 30), maxy = -(1 << 30);
    HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 160, 90));
    HPEN oldp = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    for (int i = 0; i < sel_count; i++) {
        Component *c = find_comp_by_id(selection[i]);
        if (!c) continue;
        int hw, hh;
        comp_bounding_half(c, &hw, &hh);
        Rectangle(hdc, c->x - hw, c->y - hh, c->x + hw, c->y + hh);
        if (c->x - hw < minx) minx = c->x - hw;
        if (c->y - hh < miny) miny = c->y - hh;
        if (c->x + hw > maxx) maxx = c->x + hw;
        if (c->y + hh > maxy) maxy = c->y + hh;
    }
    SelectObject(hdc, ob);
    SelectObject(hdc, oldp);
    DeleteObject(pen);
    if (minx <= maxx) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(0, 160, 90));
        HPEN ogp = (HPEN)SelectObject(hdc, gp);
        HBRUSH gb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, minx - 6, miny - 6, maxx + 6, maxy + 6);
        SelectObject(hdc, gb);
        SelectObject(hdc, ogp);
        DeleteObject(gp);
        wchar_t buf[32];
        swprintf(buf, 32, L"%d komponen (blok)", sel_count);
        HFONT hf = font_sel_label();
        HFONT ofh = (HFONT)SelectObject(hdc, hf);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 130, 75));
        TextOutW(hdc, minx - 4, miny - 24, buf, (int)wcslen(buf));
        SelectObject(hdc, ofh);
        /* font di-cache: JANGAN DeleteObject(hf) */
    }
}

static void draw_marquee(HDC hdc) {
    if (!box_selecting) return;
    int lx = (box_x1 < box_x2) ? box_x1 : box_x2;
    int rx = (box_x1 < box_x2) ? box_x2 : box_x1;
    int ty = (box_y1 < box_y2) ? box_y1 : box_y2;
    int by = (box_y1 < box_y2) ? box_y2 : box_y1;

    if (rx > lx && by > ty) {
        HDC mdc = CreateCompatibleDC(hdc);
        HBITMAP mono = CreateBitmap(rx - lx, by - ty, 1, 1, NULL);
        HBITMAP omono = (HBITMAP)SelectObject(mdc, mono);
        PatBlt(mdc, 0, 0, rx - lx, by - ty, BLACKNESS);
        BitBlt(hdc, lx, ty, rx - lx, by - ty, mdc, 0, 0, MERGEPAINT);
        SelectObject(mdc, omono);
        DeleteObject(mono);
        DeleteDC(mdc);
    }

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 128, 255));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, lx, ty, rx, by);
    SelectObject(hdc, ob);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

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

void draw_canvas(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, W, H);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    HBRUSH bg = CreateSolidBrush(RGB(255,255,255));
    FillRect(memDC, &rc, bg);
    DeleteObject(bg);

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

    for (int i = 0; i < comp_count; i++) {
        draw_component(memDC, &comps[i]);
    }

    draw_selection_boxes(memDC);
    if (box_selecting) draw_marquee(memDC);

    BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}