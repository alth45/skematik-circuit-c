#include "schematic.h"
#include <stdbool.h>

#define SPLASH_W         540
#define SPLASH_H         320
#define SPLASH_MS        2200
#define SPLASH_FADE_IN   600
#define SPLASH_FADE_OUT  500
#define SPLASH_TICK      30

static HWND    g_splash_wnd      = NULL;
static int     g_splash_progress = 0;
static bool    g_splash_closing  = false;

#define SP_DARK   RGB(18, 28, 48)
#define SP_ACCENT RGB(70, 130, 240)

/* Font splash di-cache per proses. */
static HFONT font_splash_title(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return h;
}
static HFONT font_splash_sub(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return h;
}
static HFONT font_splash_tiny(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return h;
}

static void splash_paint_content(HDC dc, int prog) {
    RECT rc = { 0, 0, SPLASH_W, SPLASH_H };

    HBRUSH bg = CreateSolidBrush(SP_DARK);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    HBRUSH ab = CreateSolidBrush(SP_ACCENT);
    RECT arc = { 0, 0, 6, SPLASH_H };
    FillRect(dc, &arc, ab);
    DeleteObject(ab);

    HFONT hTitle = font_splash_title();
    HFONT hSub = font_splash_sub();
    HFONT hTiny = font_splash_tiny();

    SetBkMode(dc, TRANSPARENT);

    HPEN pen = CreatePen(PS_SOLID, 3, SP_ACCENT);
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob2 = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    {
        int ix = 40, iy = 52;
        static const int dz[] = { -10, 10, -10, 10, -10, 10, -10, 10 };
        MoveToEx(dc, ix, iy, NULL);
        LineTo(dc, ix + 14, iy);
        int px = ix + 14;
        for (unsigned k = 0; k < sizeof(dz) / sizeof(dz[0]); k++) {
            px += 9;
            LineTo(dc, px, iy + dz[k]);
        }
        px += 9;
        MoveToEx(dc, px, iy, NULL);
        LineTo(dc, px + 14, iy);
    }
    SelectObject(dc, op);
    SelectObject(dc, ob2);
    DeleteObject(pen);

    SelectObject(dc, hTitle);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, 38, 78, L"Schematic Editor", 16);

    SelectObject(dc, hSub);
    SetTextColor(dc, RGB(160, 180, 210));
    TextOutW(dc, 40, 128, L"Simulasi rangkaian \x2014 Win32 GDI Edition", 38);

    int bx = 40, by = 220, bw = SPLASH_W - 80, bh = 10;
    HPEN bp = CreatePen(PS_SOLID, 1, RGB(60, 75, 105));
    HPEN obp = (HPEN)SelectObject(dc, bp);
    HBRUSH bb = CreateSolidBrush(RGB(32, 42, 66));
    HBRUSH obb = (HBRUSH)SelectObject(dc, bb);
    RoundRect(dc, bx, by, bx + bw, by + bh, 10, 10);
    SelectObject(dc, obp);
    SelectObject(dc, obb);
    DeleteObject(bp);
    DeleteObject(bb);

    if (prog > 0) {
        int fw = (bw - 4) * prog / 100;
        HBRUSH fb = CreateSolidBrush(SP_ACCENT);
        HBRUSH ofb = (HBRUSH)SelectObject(dc, fb);
        PatBlt(dc, bx + 2, by + 2, fw, bh - 4, PATCOPY);
        SelectObject(dc, ofb);
        DeleteObject(fb);
    }

    SelectObject(dc, hTiny);
    SetTextColor(dc, RGB(120, 135, 160));
    const wchar_t *st = (prog >= 100) ? L"Siap..." : L"Memuat editor...";
    TextOutW(dc, 40, 240, st, (int)wcslen(st));
    TextOutW(dc, 40, 288, L"v1.0", 4);
}

static LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int W = rc.right, H = rc.bottom;

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, W, H);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            splash_paint_content(memDC, g_splash_progress);

            BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN: {
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }
        case WM_DESTROY:
            g_splash_wnd = NULL;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void splash_run(HINSTANCE hInst, HWND hwnd_main) {
    static const wchar_t CLS[] = L"SchematicSplashWnd";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = SplashProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLS;
    BOOL we_registered = RegisterClassW(&wc);
    if (!we_registered) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            ShowWindow(hwnd_main, SW_SHOW);
            UpdateWindow(hwnd_main);
            SetForegroundWindow(hwnd_main);
            return;
        }
    }

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x = (sw - SPLASH_W) / 2;
    int y = (sh - SPLASH_H) / 2;

    HWND hs = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED, CLS, L"Schematic Editor",
        WS_POPUP, x, y, SPLASH_W, SPLASH_H,
        NULL, NULL, hInst, NULL);
    if (!hs) {
        ShowWindow(hwnd_main, SW_SHOW);
        UpdateWindow(hwnd_main);
        SetForegroundWindow(hwnd_main);
        if (we_registered) UnregisterClassW(CLS, hInst);
        return;
    }

    g_splash_wnd      = hs;
    g_splash_progress = 0;
    g_splash_closing  = false;

    SetLayeredWindowAttributes(hs, 0, 0, LWA_ALPHA);
    ShowWindow(hs, SW_SHOWNA);
    UpdateWindow(hs);
    SetTimer(hs, 1, SPLASH_TICK, NULL);

    MSG m;
    DWORD t0 = GetTickCount();
    int fade_ticks = 0;
    bool main_shown = false;
    bool done = false;

    while (!done && GetMessageW(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);

        if (m.message == WM_TIMER && m.hwnd == hs && m.wParam == 1) {
            DWORD el = GetTickCount() - t0;

            if (!g_splash_closing) {
                g_splash_progress = (int)((el * 100) / 1600);
                if (g_splash_progress > 100) g_splash_progress = 100;

                DWORD ain = (el < SPLASH_FADE_IN) ? el : SPLASH_FADE_IN;
                BYTE a = (BYTE)(255 * ain / SPLASH_FADE_IN);
                SetLayeredWindowAttributes(hs, 0, a, LWA_ALPHA);
                InvalidateRect(hs, NULL, FALSE);

                if (el >= (DWORD)(SPLASH_MS - SPLASH_FADE_OUT)) {
                    g_splash_closing = true;
                    if (!main_shown) {
                        main_shown = true;
                        ShowWindow(hwnd_main, SW_SHOW);
                        UpdateWindow(hwnd_main);
                        SetForegroundWindow(hwnd_main);
                    }
                }
            } else {
                fade_ticks++;
                int steps = SPLASH_FADE_OUT / SPLASH_TICK;
                int a = 255 - (255 * fade_ticks) / (steps > 0 ? steps : 1);
                if (a <= 0) {
                    KillTimer(hs, 1);
                    DestroyWindow(hs);
                    done = true;
                } else {
                    SetLayeredWindowAttributes(hs, 0, (BYTE)a, LWA_ALPHA);
                }
            }
        }

        if (!IsWindow(hs)) done = true;
    }

    if (hs && IsWindow(hs)) {
        KillTimer(hs, 1);
        DestroyWindow(hs);
    }
    g_splash_wnd = NULL;

    if (!main_shown) {
        ShowWindow(hwnd_main, SW_SHOW);
        UpdateWindow(hwnd_main);
    }
    SetForegroundWindow(hwnd_main);

    if (we_registered) {
        UnregisterClassW(CLS, hInst);
    }
}