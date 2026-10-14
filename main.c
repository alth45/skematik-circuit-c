#include "schematic.h"

/* ===================== Window Procedure ===================== */
static void setup_keyboard(HWND hwnd) {
    SetFocus(hwnd);
}

static void toolbar_click(HWND hwnd, int x, int y) {
    int idx = toolbar_hit(x, y);
    if (idx < 0) return;
    if (idx == CLEAR_BTN_INDEX) {
        if (sim_running) return;
        model_clear_all();
        sim_recompute();
        set_status(L"Dibersihkan. Canvas kosong.");
    } else if (idx == SAVE_BTN_INDEX) {
        model_save_dialog(hwnd);
    } else if (idx == OPEN_BTN_INDEX) {
        model_load_dialog(hwnd);
        sim_recompute();
    } else if (idx == RUN_BTN_INDEX) {
        sim_set_running(!sim_running);
    } else {
        if (sim_running) return; /* ganti tool dikunci saat RUN */
        current_tool = btnTools[idx];
        dragging_id = -1;
        wiring_active = 0;
        setup_keyboard(hwnd);
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            setup_keyboard(hwnd);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lp);
            int y = (short)HIWORD(lp);
            if (y < TOOLBAR_H) { toolbar_click(hwnd, x, y); return 0; }
            setup_keyboard(hwnd);
            y -= TOOLBAR_H;
            on_lbutton_down(hwnd, x, y);
            return 0;
        }

        case WM_RBUTTONDOWN: {
            int x = (short)LOWORD(lp);
            int y = (short)HIWORD(lp);
            if (y < TOOLBAR_H) return 0;
            y -= TOOLBAR_H;
            on_rbutton_down(hwnd, x, y);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = (short)LOWORD(lp);
            int y = (short)HIWORD(lp);
            if (y < TOOLBAR_H) y = TOOLBAR_H;
            y -= TOOLBAR_H;
            on_mouse_move(hwnd, x, y);
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = (short)LOWORD(lp);
            int y = (short)HIWORD(lp);
            if (y < TOOLBAR_H) y = TOOLBAR_H;
            y -= TOOLBAR_H;
            on_lbutton_up(hwnd, x, y);
            return 0;
        }

        case WM_KEYDOWN:
            on_key_down(hwnd, wp);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int W = rc.right, H = rc.bottom;

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, W, H);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            // Background putih untuk seluruh window
            HBRUSH bg = CreateSolidBrush(RGB(255,255,255));
            FillRect(memDC, &rc, bg);
            DeleteObject(bg);

            // Toolbar atas (ikon simbol)
            draw_toolbar(memDC, W);

            // Grid + komponen, offset Y = TOOLBAR_H
            SaveDC(memDC);
            SetViewportOrgEx(memDC, 0, TOOLBAR_H, NULL);
            IntersectClipRect(memDC, 0, 0, W, H - TOOLBAR_H - STATUS_H);

            HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(235,235,235));
            HPEN op = (HPEN)SelectObject(memDC, gridPen);
            for (int x = 0; x < W; x += GRID) {
                MoveToEx(memDC, x, 0, NULL); LineTo(memDC, x, H - TOOLBAR_H - STATUS_H);
            }
            for (int y = 0; y < H - TOOLBAR_H - STATUS_H; y += GRID) {
                MoveToEx(memDC, 0, y, NULL); LineTo(memDC, W, y);
            }
            SelectObject(memDC, op);
            DeleteObject(gridPen);

            draw_wires(memDC);
            for (int i = 0; i < comp_count; i++) {
                draw_component(memDC, &comps[i]);
            }

            /* Banner hasil simulasi saat RUN */
            if (sim_running) {
                const wchar_t *msg = sim_message();
                COLORREF fg, bgc;
                if (sim_short) { fg = RGB(255,255,255); bgc = RGB(200,40,40); }
                else if (sim_led_count > 0) { fg = RGB(255,255,255); bgc = RGB(20,150,60); }
                else { fg = RGB(60,60,60); bgc = RGB(255,235,160); }
                HFONT hB = CreateFontW(16, 0, 0, 0, FW_BOLD, 0, 0, 0,
                    0, 0, 0, 0, 0, L"Segoe UI");
                HFONT oB = (HFONT)SelectObject(memDC, hB);
                SIZE bsz; GetTextExtentPoint32W(memDC, msg, (int)wcslen(msg), &bsz);
                int bw = bsz.cx + 28, bh = bsz.cy + 14;
                int bx = (W - bw) / 2, by = 10;
                HBRUSH bb2 = CreateSolidBrush(bgc);
                HBRUSH obb2 = (HBRUSH)SelectObject(memDC, bb2);
                HPEN bp2 = CreatePen(PS_SOLID, 1, fg);
                HPEN obp2 = (HPEN)SelectObject(memDC, bp2);
                RoundRect(memDC, bx, by, bx + bw, by + bh, 10, 10);
                SelectObject(memDC, obp2);
                SelectObject(memDC, obb2);
                DeleteObject(bp2);
                DeleteObject(bb2);
                SetTextColor(memDC, fg);
                SetBkMode(memDC, TRANSPARENT);
                TextOutW(memDC, bx + 14, by + 7, msg, (int)wcslen(msg));
                SelectObject(memDC, oB);
                DeleteObject(hB);
            }

            RestoreDC(memDC, -1);

            /* Status bar bawah */
            {
                RECT sr = { 0, H - STATUS_H, W, H };
                HBRUSH sb = CreateSolidBrush(RGB(245,245,245));
                FillRect(memDC, &sr, sb);
                DeleteObject(sb);
                HPEN sp2 = CreatePen(PS_SOLID, 1, RGB(200,200,200));
                HPEN osp4 = (HPEN)SelectObject(memDC, sp2);
                MoveToEx(memDC, 0, H - STATUS_H, NULL); LineTo(memDC, W, H - STATUS_H);
                SelectObject(memDC, osp4);
                DeleteObject(sp2);
                HFONT hSt = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                    0, 0, 0, 0, 0, L"Segoe UI");
                HFONT oSt = (HFONT)SelectObject(memDC, hSt);
                SetTextColor(memDC, RGB(60,60,60));
                SetBkMode(memDC, TRANSPARENT);
                TextOutW(memDC, 10, H - STATUS_H + 5, g_status, (int)wcslen(g_status));
                wchar_t cnt[64];
                swprintf(cnt, 64, L"%d komponen, %d kabel", comp_count, wire_count);
                SIZE csz; GetTextExtentPoint32W(memDC, cnt, (int)wcslen(cnt), &csz);
                TextOutW(memDC, W - csz.cx - 10, H - STATUS_H + 5, cnt, (int)wcslen(cnt));
                SelectObject(memDC, oSt);
                DeleteObject(hSt);
            }

            BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // hindari flicker (background diurus WM_PAINT)

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO *)lp;
            mmi->ptMinTrackSize.x = 920;
            mmi->ptMinTrackSize.y = 600;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ===================== Entry Point ===================== */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int show) {
    (void)hPrev; (void)cmd;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"SchematicEditorWnd";

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"SchematicEditorWnd",
        L"Schematic Editor - Win32 GDI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 650,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
