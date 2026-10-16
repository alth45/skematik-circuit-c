#include "schematic.h"
#include <stdbool.h>

/* ============================================================
 *  SPLASH SCREEN
 *  Jendela kecil bertema "Schematic Editor" yang tampil dulu
 *  saat .exe dijalankan (fade-in + progress bar), lalu tertutup
 *  (fade-out) tepat ketika jendela utama muncul.
 *  Murni Win32 + GDI; tidak menyentuh state aplikasi sama sekali.
 * ============================================================ */

#define SPLASH_W         540   /* lebar tetap (window tanpa border)  */
#define SPLASH_H         320   /* tinggi tetap                       */
#define SPLASH_MS        2200  /* lama tampil total (ms)             */
#define SPLASH_FADE_IN   600   /* durasi fade-in  (ms)               */
#define SPLASH_FADE_OUT  500   /* durasi fade-out (ms)               */
#define SPLASH_TICK      30    /* interval timer animasi (ms)        */

static HWND    g_splash_wnd      = NULL;
static int     g_splash_progress = 0;      /* 0..100 untuk progress bar */
static bool    g_splash_closing  = false;

/* Warna tema splash */
#define SP_DARK   RGB(18, 28, 48)
#define SP_ACCENT RGB(70, 130, 240)

/* Gambar konten splash ke DC (koordinat klien 0,0 .. SPLASH_W x SPLASH_H) */
static void splash_paint_content(HDC dc, int prog) {
    RECT rc = { 0, 0, SPLASH_W, SPLASH_H };

    /* Latar solid gelap */
    HBRUSH bg = CreateSolidBrush(SP_DARK);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    /* Aksen: garis vertikal tipis di sisi kiri */
    HBRUSH ab = CreateSolidBrush(SP_ACCENT);
    RECT arc = { 0, 0, 6, SPLASH_H };
    FillRect(dc, &arc, ab);
    DeleteObject(ab);

    /* 14 argumen CreateFontW: height,width,escapement,orientation,weight,
       italic,underline,strikeout,charset,outprecision,clipprecision,
       quality,pitchfamily,facename */
    HFONT hTitle = CreateFontW(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hSub = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hTiny = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    SetBkMode(dc, TRANSPARENT);

    /* Ikon sederhana: simbol "resistor" zig-zag di kiri atas */
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

    /* Judul + subjudul */
    SelectObject(dc, hTitle);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, 38, 78, L"Schematic Editor", 16);

    SelectObject(dc, hSub);
    SetTextColor(dc, RGB(160, 180, 210));
    TextOutW(dc, 40, 128, L"Simulasi rangkaian \x2014 Win32 GDI Edition", 38);

    /* Progress bar */
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

    /* Teks status kecil di bawah progress bar + versi */
    SelectObject(dc, hTiny);
    SetTextColor(dc, RGB(120, 135, 160));
    const wchar_t *st = (prog >= 100) ? L"Siap..." : L"Memuat editor...";
    TextOutW(dc, 40, 240, st, (int)wcslen(st));
    TextOutW(dc, 40, 288, L"v1.0", 4);

    DeleteObject(hTitle);
    DeleteObject(hSub);
    DeleteObject(hTiny);
}

/* Window proc khusus splash: WM_PAINT gambar konten; klik-kiri = drag */
static LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int W = rc.right, H = rc.bottom;

            /* double-buffering agar tidak flicker */
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
            return 1; /* latar digambar penuh oleh WM_PAINT */
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
    if (!RegisterClassW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;
    }

    /* Posisi tengah layar primer */
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x = (sw - SPLASH_W) / 2;
    int y = (sh - SPLASH_H) / 2;

    /* WS_EX_LAYERED => bisa fade in/out via SetLayeredWindowAttributes */
    HWND hs = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED, CLS, L"Schematic Editor",
        WS_POPUP, x, y, SPLASH_W, SPLASH_H,
        NULL, NULL, hInst, NULL);
    if (!hs) return;

    g_splash_wnd     = hs;
    
    g_splash_closing = false;

    SetLayeredWindowAttributes(hs, 0, 0, LWA_ALPHA); /* mulai transparan */
    ShowWindow(hs, SW_SHOWNA);
    UpdateWindow(hs);
    SetTimer(hs, 1, SPLASH_TICK, NULL);

    MSG m;
    DWORD t0 = GetTickCount();
    int fade_ticks = 0;
    bool main_shown = false;
    bool done = false;

    /* Message loop mini KHUSUS splash: hanya berjalan selama splash hidup */
    while (!done && GetMessageW(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);

        if (m.message == WM_TIMER && m.hwnd == hs && m.wParam == 1) {
            DWORD el = GetTickCount() - t0;

            if (!g_splash_closing) {
                /* progress naik menuju 100 dalam ~1.6 detik */
                g_splash_progress = (int)((el * 100) / 1600);
                if (g_splash_progress > 100) g_splash_progress = 100;

                /* alpha fade-in */
                DWORD ain = (el < SPLASH_FADE_IN) ? el : SPLASH_FADE_IN;
                BYTE a = (BYTE)(255 * ain / SPLASH_FADE_IN);
                SetLayeredWindowAttributes(hs, 0, a, LWA_ALPHA);
                InvalidateRect(hs, NULL, FALSE);

                if (el >= (DWORD)(SPLASH_MS - SPLASH_FADE_OUT)) {
                    /* splash selesai: tampilkan jendela UTAMA, lalu fade-out */
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

        /* Bila user menutup splash paksa (mis. Alt+F4) -> hentikan saja */
        if (!IsWindow(hs)) done = true;
    }

    if (hs && IsWindow(hs)) {
        KillTimer(hs, 1);
        DestroyWindow(hs);
    }
    g_splash_wnd = NULL;
    UnregisterClassW(CLS, hInst);
}
