#include "toolbar_intern.h"

/* Palette: hit-test + tombol aksi Clear/Save/Open/Run. */

int toolbar_btn_x(int idx) {
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
    if (x >= toolbar_btn_x(BTN_COUNT) &&
        x <= toolbar_btn_x(BTN_COUNT) + TOOLBAR_BTN_W) {
        if (is_clear) *is_clear = 1;
        return CLEAR_BTN_INDEX;
    }
    if (x >= toolbar_btn_x(BTN_COUNT + 1) &&
        x <= toolbar_btn_x(BTN_COUNT + 1) + TOOLBAR_BTN_W) return SAVE_BTN_INDEX;
    if (x >= toolbar_btn_x(BTN_COUNT + 2) &&
        x <= toolbar_btn_x(BTN_COUNT + 2) + TOOLBAR_BTN_W) return OPEN_BTN_INDEX;
    if (x >= toolbar_btn_x(BTN_COUNT + 3) &&
        x <= toolbar_btn_x(BTN_COUNT + 3) + TOOLBAR_BTN_W) return RUN_BTN_INDEX;
    return -1;
}



void draw_toolbar(HDC hdc, int W) {
    draw_menubar(hdc, W);
    int saved = SaveDC(hdc);
    OffsetViewportOrgEx(hdc, 0, MENUBAR_H, NULL);
    RECT bar = { 0, 0, W, PALETTE_H };
    HBRUSH bg = CreateSolidBrush(RGB(245,245,245));
    FillRect(hdc, &bar, bg);
    DeleteObject(bg);
    HPEN sep = CreatePen(PS_SOLID, 1, RGB(200,200,200));
    HPEN osp = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, 0, PALETTE_H - 1, NULL); LineTo(hdc, W, PALETTE_H - 1);
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
        if (is_action && i == BTN_COUNT) palette_draw_clear(hdc, bx, by);
        else if (is_action && i == BTN_COUNT + 1) palette_draw_save(hdc, bx, by);
        else if (is_action && i == BTN_COUNT + 2) palette_draw_open(hdc, bx, by);
        else if (is_action && i == BTN_COUNT + 3) palette_draw_run(hdc, bx, by);
        else if (toolbar_tool_is_component(tool)) {
            toolbar_draw_symbol(hdc, tool, bx + TOOLBAR_BTN_W / 2, by + 17, active);
            SetTextColor(hdc, active ? RGB(30,90,200) : RGB(90,90,90));
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, bx + 19, by + 30, toolbar_tool_shortname(tool),
                     (int)wcslen(toolbar_tool_shortname(tool)));
        } else {
            toolbar_draw_symbol(hdc, tool, bx + TOOLBAR_BTN_W / 2, by + 15, active);
            const wchar_t *nm = toolbar_tool_shortname(tool);
            int offx = (tool == TOOL_SELECT) ? 13 :
                       (tool == TOOL_WIRE) ? 17 :
                       (tool == TOOL_BOX) ? 16 : 14;
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
    _snwprintf(status, 256, L"%ls   |   F5: run/stop   R: rotate", cur);
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
    RestoreDC(hdc, saved);
}
