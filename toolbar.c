#include "schematic.h"

static int toolbar_btn_x(int idx) {
    return 8 + idx * (TOOLBAR_BTN_W + 6);
}

int toolbar_hit(int x, int y) {
    return toolbar_tool_at(x, y, NULL);
}

int toolbar_tool_at(int x, int y, int *is_clear) {
    if (is_clear) *is_clear = 0;
    if (y < 8 || y > 8 + TOOLBAR_BTN_H) return -1;
    for (int i = 0; i < BTN_COUNT; i++) {
        int bx = toolbar_btn_x(i);
        if (x >= bx && x <= bx + TOOLBAR_BTN_W) return i;
    }
    {
        int bx = toolbar_btn_x(BTN_COUNT);
        if (x >= bx && x <= bx + TOOLBAR_BTN_W) {
            if (is_clear) *is_clear = 1;
            return CLEAR_BTN_INDEX;
        }
    }
    {
        int bx = toolbar_btn_x(BTN_COUNT + 1);
        if (x >= bx && x <= bx + TOOLBAR_BTN_W) return SAVE_BTN_INDEX;
    }
    {
        int bx = toolbar_btn_x(BTN_COUNT + 2);
        if (x >= bx && x <= bx + TOOLBAR_BTN_W) return OPEN_BTN_INDEX;
    }
    {
        int bx = toolbar_btn_x(BTN_COUNT + 3);
        if (x >= bx && x <= bx + TOOLBAR_BTN_W) return RUN_BTN_INDEX;
    }
    return -1;
}

static CompType toolbar_tool_comp(ToolKind t) {
    switch (t) {
        case TOOL_RESISTOR:   return COMP_RESISTOR;
        case TOOL_SWITCH:     return COMP_SWITCH;
        case TOOL_BATTERY:    return COMP_BATTERY;
        case TOOL_LED:        return COMP_LED;
        case TOOL_GROUND:     return COMP_GROUND;
        case TOOL_CAPACITOR:  return COMP_CAPACITOR;
        case TOOL_TRANSISTOR: return COMP_TRANSISTOR;
        default: return COMP_NONE;
    }
}

static void sym_resistor(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -20, 0);
    MoveToEx(hdc, 20, 0, NULL); LineTo(hdc, 40, 0);
    Rectangle(hdc, -20, -10, 20, 10);
}

static void sym_switch(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -15, 0);
    MoveToEx(hdc, 15, 0, NULL); LineTo(hdc, 40, 0);
    Ellipse(hdc, -18, -3, -12, 3);
    Ellipse(hdc, 12, -3, 18, 3);
    MoveToEx(hdc, -15, 0, NULL); LineTo(hdc, 12, -15);
}

static void sym_battery(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -10, 0);
    MoveToEx(hdc, 10, 0, NULL); LineTo(hdc, 40, 0);
    MoveToEx(hdc, -10, -15, NULL); LineTo(hdc, -10, 15);
    MoveToEx(hdc, 10, -8, NULL); LineTo(hdc, 10, 8);
}

static void sym_led(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -12, 0);
    MoveToEx(hdc, 12, 0, NULL); LineTo(hdc, 40, 0);
    POINT tri[3] = { {-12,-15}, {-12,15}, {12,0} };
    Polygon(hdc, tri, 3);
    MoveToEx(hdc, 12, -15, NULL); LineTo(hdc, 12, 15);
    MoveToEx(hdc, -5, -18, NULL); LineTo(hdc, -12, -30);
    MoveToEx(hdc, 3, -18, NULL); LineTo(hdc, -4, -30);
}

static void sym_ground(HDC hdc) {
    MoveToEx(hdc, 0, -30, NULL); LineTo(hdc, 0, 0);
    MoveToEx(hdc, -20, 0, NULL); LineTo(hdc, 20, 0);
    MoveToEx(hdc, -12, 8, NULL); LineTo(hdc, 12, 8);
    MoveToEx(hdc, -5, 16, NULL); LineTo(hdc, 5, 16);
}

static void sym_capacitor(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -6, 0);
    MoveToEx(hdc, 6, 0, NULL); LineTo(hdc, 40, 0);
    MoveToEx(hdc, -6, -15, NULL); LineTo(hdc, -6, 15);
    MoveToEx(hdc, 6, -15, NULL); LineTo(hdc, 6, 15);
}

static void sym_transistor(HDC hdc) {
    Ellipse(hdc, -20, -20, 20, 20);
    MoveToEx(hdc, -4, -12, NULL); LineTo(hdc, -4, 12);
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -19, 0);
    MoveToEx(hdc, -19, 0, NULL); LineTo(hdc, -4, -6);
    MoveToEx(hdc, -4, 6, NULL); LineTo(hdc, 10, 12);
    MoveToEx(hdc, 10, 12, NULL); LineTo(hdc, 40, 13);
    MoveToEx(hdc, 10, 12, NULL); LineTo(hdc, 1, 11);
    MoveToEx(hdc, 10, 12, NULL); LineTo(hdc, 7, 3);
    MoveToEx(hdc, -4, 30, NULL); LineTo(hdc, -4, 12);
}

static void draw_tool_symbol(HDC hdc, ToolKind tool, int cx, int cy, BOOL active) {
    int saved = SaveDC(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xf;
    xf.eM11 = 0.42f; xf.eM12 = 0;
    xf.eM21 = 0;     xf.eM22 = 0.42f;
    xf.eDx = (FLOAT)cx;
    xf.eDy = (FLOAT)(cy - 2);
    SetWorldTransform(hdc, &xf);
    HPEN pen = CreatePen(PS_SOLID, 2, active ? RGB(50,128,255) : RGB(20,20,20));
    HPEN op = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    CompType ct = toolbar_tool_comp(tool);
    if (ct == COMP_RESISTOR) sym_resistor(hdc);
    else if (ct == COMP_SWITCH) sym_switch(hdc);
    else if (ct == COMP_BATTERY) sym_battery(hdc);
    else if (ct == COMP_LED) sym_led(hdc);
    else if (ct == COMP_GROUND) sym_ground(hdc);
    else if (ct == COMP_CAPACITOR) sym_capacitor(hdc);
    else if (ct == COMP_TRANSISTOR) sym_transistor(hdc);
    else if (tool == TOOL_SELECT) {
        POINT cur[7] = { {0,-14},{10,4},{4,4},{4,14},{-2,14},{-2,4},{-8,4} };
        HBRUSH br = CreateSolidBrush(active ? RGB(50,128,255) : RGB(40,40,40));
        HBRUSH ob2 = (HBRUSH)SelectObject(hdc, br);
        HPEN pn2 = CreatePen(PS_SOLID, 1, active ? RGB(50,128,255) : RGB(40,40,40));
        HPEN op2 = (HPEN)SelectObject(hdc, pn2);
        Polygon(hdc, cur, 7);
        SelectObject(hdc, op2);
        SelectObject(hdc, ob2);
        DeleteObject(pn2);
        DeleteObject(br);
    } else if (tool == TOOL_WIRE) {
        HPEN wp = CreatePen(PS_SOLID, 3, active ? RGB(50,128,255) : RGB(20,20,20));
        HPEN owp = (HPEN)SelectObject(hdc, wp);
        MoveToEx(hdc, -22, -10, NULL); LineTo(hdc, 0, -10); LineTo(hdc, 0, 12);
        MoveToEx(hdc, 0, 12, NULL); LineTo(hdc, 22, 12);
        SelectObject(hdc, owp);
        DeleteObject(wp);
        HBRUSH br = CreateSolidBrush(RGB(220,60,60));
        HBRUSH ob3 = (HBRUSH)SelectObject(hdc, br);
        Ellipse(hdc, -27, -15, -17, -5);
        Ellipse(hdc, 17, 7, 27, 17);
        SelectObject(hdc, ob3);
        DeleteObject(br);
    } else if (tool == TOOL_DELETE) {
        HPEN dp = CreatePen(PS_SOLID, 3, RGB(200,40,40));
        HPEN odp = (HPEN)SelectObject(hdc, dp);
        MoveToEx(hdc, -16, -12, NULL); LineTo(hdc, 16, -12);
        MoveToEx(hdc, -8, -12, NULL); LineTo(hdc, -5, -18);
        LineTo(hdc, 5, -18); LineTo(hdc, 8, -12);
        Rectangle(hdc, -13, -12, 13, 16);
        MoveToEx(hdc, -5, -6, NULL); LineTo(hdc, -5, 10);
        MoveToEx(hdc, 0, -6, NULL); LineTo(hdc, 0, 10);
        MoveToEx(hdc, 5, -6, NULL); LineTo(hdc, 5, 10);
        SelectObject(hdc, odp);
        DeleteObject(dp);
    }
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(pen);
    RestoreDC(hdc, saved);
}

void draw_toolbar(HDC hdc, int W) {
    RECT bar = { 0, 0, W, TOOLBAR_H };
    HBRUSH bg = CreateSolidBrush(RGB(245,245,245));
    FillRect(hdc, &bar, bg);
    DeleteObject(bg);
    HPEN sep = CreatePen(PS_SOLID, 1, RGB(200,200,200));
    HPEN osp = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, 0, TOOLBAR_H - 1, NULL); LineTo(hdc, W, TOOLBAR_H - 1);
    SelectObject(hdc, osp);
    DeleteObject(sep);

    HFONT hF = CreateFontW(12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        0, 0, 0, 0, 0, L"Segoe UI");
    HFONT of = (HFONT)SelectObject(hdc, hF);

    for (int i = 0; i <= BTN_COUNT + 3; i++) {
        int is_action = (i >= BTN_COUNT);
        int bx = toolbar_btn_x(i);
        int by = 8;
        ToolKind tool = TOOL_DELETE;
        BOOL active = FALSE;
        if (!is_action) { tool = btnTools[i]; active = (tool == current_tool); }
        HBRUSH bb = CreateSolidBrush(active ? RGB(220,235,255) : RGB(255,255,255));
        HBRUSH obb = (HBRUSH)SelectObject(hdc, bb);
        HPEN bp = CreatePen(PS_SOLID, active ? 2 : 1,
                            active ? RGB(50,128,255) : RGB(190,190,190));
        HPEN obp = (HPEN)SelectObject(hdc, bp);
        RoundRect(hdc, bx, by, bx + TOOLBAR_BTN_W, by + TOOLBAR_BTN_H, 8, 8);
        SelectObject(hdc, obp);
        SelectObject(hdc, obb);
        DeleteObject(bp);
        DeleteObject(bb);

        if (is_action && i == BTN_COUNT) {
            HPEN xp = CreatePen(PS_SOLID, 3, RGB(200,40,40));
            HPEN oxp = (HPEN)SelectObject(hdc, xp);
            int cx = bx + TOOLBAR_BTN_W / 2, cy = by + 15;
            MoveToEx(hdc, cx - 9, cy - 7, NULL); LineTo(hdc, cx + 9, cy + 7);
            MoveToEx(hdc, cx + 9, cy - 7, NULL); LineTo(hdc, cx - 9, cy + 7);
            SelectObject(hdc, oxp);
            DeleteObject(xp);
            SetTextColor(hdc, RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bx + 13, by + 28, L"Clear", 5);
        } else if (is_action && i == BTN_COUNT + 1) {
            HPEN sp = CreatePen(PS_SOLID, 2, RGB(30,120,60));
            HPEN osp2 = (HPEN)SelectObject(hdc, sp);
            HBRUSH sob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            int cx = bx + TOOLBAR_BTN_W / 2, cy = by + 14;
            Rectangle(hdc, cx - 11, cy - 9, cx + 11, cy + 9);
            MoveToEx(hdc, cx - 11, cy - 9, NULL); LineTo(hdc, cx - 4, cy - 9);
            LineTo(hdc, cx - 4, cy - 2); LineTo(hdc, cx - 11, cy - 2);
            MoveToEx(hdc, cx - 3, cy + 5, NULL); LineTo(hdc, cx + 1, cy + 5);
            LineTo(hdc, cx + 5, cy + 1);
            SelectObject(hdc, sob);
            SelectObject(hdc, osp2);
            DeleteObject(sp);
            SetTextColor(hdc, RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bx + 14, by + 28, L"Save", 4);
        } else if (is_action && i == BTN_COUNT + 2) {
            HPEN op2 = CreatePen(PS_SOLID, 2, RGB(30,90,200));
            HPEN osp3 = (HPEN)SelectObject(hdc, op2);
            HBRUSH oob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            int cx = bx + TOOLBAR_BTN_W / 2, cy = by + 14;
            Rectangle(hdc, cx - 11, cy - 6, cx + 11, cy + 9);
            MoveToEx(hdc, cx - 6, cy - 6, NULL); LineTo(hdc, cx - 2, cy - 11);
            LineTo(hdc, cx + 6, cy - 11); LineTo(hdc, cx + 6, cy - 6);
            MoveToEx(hdc, cx, cy - 3, NULL); LineTo(hdc, cx, cy + 5);
            MoveToEx(hdc, cx - 4, cy + 1, NULL); LineTo(hdc, cx, cy + 5);
            LineTo(hdc, cx + 4, cy + 1);
            SelectObject(hdc, oob);
            SelectObject(hdc, osp3);
            DeleteObject(op2);
            SetTextColor(hdc, RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bx + 14, by + 28, L"Open", 4);
        } else if (is_action && i == BTN_COUNT + 3) {
            /* Tombol RUN / STOP */
            int cx = bx + TOOLBAR_BTN_W / 2, cy = by + 14;
            if (sim_running) {
                HBRUSH sb2 = CreateSolidBrush(RGB(200,40,40));
                HBRUSH osb = (HBRUSH)SelectObject(hdc, sb2);
                HPEN sp3 = CreatePen(PS_SOLID, 1, RGB(200,40,40));
                HPEN osp5 = (HPEN)SelectObject(hdc, sp3);
                Rectangle(hdc, cx - 8, cy - 8, cx + 8, cy + 8);
                SelectObject(hdc, osp5);
                SelectObject(hdc, osb);
                DeleteObject(sp3);
                DeleteObject(sb2);
                SetTextColor(hdc, RGB(200,40,40));
            } else {
                POINT tri[3] = { {cx - 8, cy - 9}, {cx - 8, cy + 9}, {cx + 9, cy} };
                HBRUSH rb = CreateSolidBrush(RGB(20,150,60));
                HBRUSH orb = (HBRUSH)SelectObject(hdc, rb);
                HPEN rp = CreatePen(PS_SOLID, 1, RGB(20,150,60));
                HPEN orp = (HPEN)SelectObject(hdc, rp);
                Polygon(hdc, tri, 3);
                SelectObject(hdc, orp);
                SelectObject(hdc, orb);
                DeleteObject(rp);
                DeleteObject(rb);
                SetTextColor(hdc, RGB(20,150,60));
            }
            SetBkMode(hdc, TRANSPARENT);
            const wchar_t *rnm = sim_running ? L"Stop" : L"Run";
            TextOutW(hdc, bx + (sim_running ? 13 : 16), by + 28, rnm, (int)wcslen(rnm));
        } else if (tool == TOOL_RESISTOR || tool == TOOL_SWITCH ||
                   tool == TOOL_BATTERY || tool == TOOL_LED ||
                   tool == TOOL_GROUND || tool == TOOL_CAPACITOR ||
                   tool == TOOL_TRANSISTOR) {
            draw_tool_symbol(hdc, tool, bx + TOOLBAR_BTN_W / 2, by + 17, active);
            SetTextColor(hdc, active ? RGB(30,90,200) : RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            const wchar_t *nm = L"";
            if (tool == TOOL_RESISTOR) nm = L"R";
            else if (tool == TOOL_SWITCH) nm = L"SW";
            else if (tool == TOOL_BATTERY) nm = L"BAT";
            else if (tool == TOOL_LED) nm = L"LED";
            else if (tool == TOOL_CAPACITOR) nm = L"C";
            else if (tool == TOOL_TRANSISTOR) nm = L"Q";
            else nm = L"GND";
            TextOutW(hdc, bx + 19, by + 30, nm, (int)wcslen(nm));
        } else {
            draw_tool_symbol(hdc, tool, bx + TOOLBAR_BTN_W / 2, by + 15, active);
            const wchar_t *nm = (tool == TOOL_SELECT) ? L"Select" :
                                (tool == TOOL_WIRE) ? L"Wire" : L"Hapus";
            int offx = (tool == TOOL_SELECT) ? 13 :
                       (tool == TOOL_WIRE) ? 17 : 14;
            SetTextColor(hdc, active ? RGB(30,90,200) : RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bx + offx, by + 30, nm, (int)wcslen(nm));
        }
    }
    SelectObject(hdc, of);
    DeleteObject(hF);

    wchar_t status[256];
    const wchar_t *cur = L"";
    for (int i = 0; i < BTN_COUNT; i++)
        if (btnTools[i] == current_tool) { cur = btnLabels[i]; break; }
    _snwprintf(status, 256,
        L"%ls   |   F5: run/stop   R: rotate", cur);
    status[255] = L'\0';
    HFONT hS = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        0, 0, 0, 0, 0, L"Segoe UI");
    HFONT os2 = (HFONT)SelectObject(hdc, hS);
    SetTextColor(hdc, RGB(90,90,90));
    SetBkMode(hdc, TRANSPARENT);
    SIZE sz; GetTextExtentPoint32W(hdc, status, (int)wcslen(status), &sz);
    TextOutW(hdc, W - sz.cx - 12, 22, status, (int)wcslen(status));
    SelectObject(hdc, os2);
    DeleteObject(hS);
}

