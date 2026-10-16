/* ============================================================
 *  splash.c — Splash screen Win32 GDI (murni gambar prosedural,
 *  tanpa berkas eksternal => tetap portable saat jadi .exe).
 *
 *  Alur pemakaian (lihat wWinMain di src/main.c):
 *      splash_show(hInst)   -> tampil + animasi progress (blok +-2 detik)
 *      ... buat window utama ...
 *      splash_update()      -> tutup splash sebelum pesan pertama diproses
 *
 *  Desain visual: kartu gelap dengan aksen sirkuit hijau-cyan,
 *  logo "S" bergaya solder-pad, judul aplikasi, versi, tagline,
 *  bar progres beranimasi, dan teks "Loading...".
 * ============================================================ */
#define _USE_MATH_DEFINES
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>

#define SPLASH_W        480
#define SPLASH_H        280
#define SPLASH_MS       2000   /* durasi minimum tampilan (ms) */
#define SPLASH_TICK_MS  15     /* interval timer animasi */

static HWND g_splash_hwnd  = NULL;
static HBITMAP g_splash_bmp = NULL;   /* bitmap latar pra-render (anti-flicker) */
static int  g_prog         = 0;       /* progres 0..100 */
static DWORD g_start_tick  = 0;

/* ---- pra-render seluruh desain splash ke memory bitmap ---- */
static void splash_render_background(HDC hdc)
{
    /* Gradient diagonal gelap navy -> biru petroli (vertikal, murah & rapi) */
    for (int y = 0; y < SPLASH_H; y++) {
        float t = (float)y / (float)(SPLASH_H - 1);
        COLORREF c = RGB((int)(16 + t * 8), (int)(22 + t * 26), (int)(38 + t * 40));
        HPEN pen = CreatePen(PS_SOLID, 1, c);
        HPEN old = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, SPLASH_W, y);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    /* Bingkai luar tipis + garis aksen cyan di bawah area konten */
    HPEN framePen = CreatePen(PS_SOLID, 1, RGB(70, 90, 110));
    HPEN oldp = (HPEN)SelectObject(hdc, framePen);
    HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldb = (HBRUSH)SelectObject(hdc, nullBr);
    Rectangle(hdc, 0, 0, SPLASH_W - 1, SPLASH_H - 1);
    SelectObject(hdc, oldb);
    SelectObject(hdc, oldp);
    DeleteObject(framePen);

    /* Motif jalur PCB samar di kanan-bawah */
    HPEN tracePen = CreatePen(PS_SOLID, 1, RGB(38, 60, 74));
    oldp = (HPEN)SelectObject(hdc, tracePen);
    static const int traces[][4] = {
        { 330, 250, 430, 250 }, { 430, 250, 460, 220 }, { 460, 220, 460, 150 },
        { 360, 235, 400, 235 }, { 400, 235, 445, 190 }, { 445, 190, 445, 130 },
        { 300, 265, 470, 265 },
    };
    for (unsigned i = 0; i < sizeof(traces) / sizeof(traces[0]); i++) {
        MoveToEx(hdc, traces[i][0], traces[i][1], NULL);
        LineTo(hdc, traces[i][2], traces[i][3]);
    }
    /* solder pad kecil pada ujung jalur */
    HBRUSH padBr = CreateSolidBrush(RGB(45, 75, 90));
    oldb = (HBRUSH)SelectObject(hdc, padBr);
    static const POINT pads[] = { {460,150}, {445,130}, {470,265}, {430,250} };
    for (unsigned i = 0; i < sizeof(pads) / sizeof(pads[0]); i++) {
        Ellipse(hdc, pads[i].x - 3, pads[i].y - 3, pads[i].x + 3, pads[i].y + 3);
    }
    SelectObject(hdc, oldb);
    SelectObject(hdc, oldp);
    DeleteObject(padBr);
    DeleteObject(tracePen);

    /* ==== Logo: lingkaran "solder pad" dengan huruf S sirkuit ==== */
    int cx = 78, cy = 84, r = 34;
    HPEN ringPen = CreatePen(PS_SOLID, 3, RGB(0, 190, 160));
    oldp = (HPEN)SelectObject(hdc, ringPen);
    HBRUSH ringBr = CreateSolidBrush(RGB(22, 40, 52));
    oldb = (HBRUSH)SelectObject(hdc, ringBr);
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(hdc, oldb);

    /* kaki-kaki IC pendek mengelilingi logo */
    static const int legAngles[] = { 0, 45, 90, 135, 180, 225, 270, 315 };
    for (unsigned i = 0; i < sizeof(legAngles) / sizeof(legAngles[0]); i++) {
        double a = legAngles[i] * 3.14159265358979 / 180.0;
        int x1 = cx + (int)(r * cos(a) * 0.0) + 0; (void)x1;
        int sx = cx + (int)((r + 2) * sin(a));
        int sy = cy - (int)((r + 2) * cos(a));
        int ex = cx + (int)((r + 9) * sin(a));
        int ey = cy - (int)((r + 9) * cos(a));
        MoveToEx(hdc, sx, sy, NULL);
        LineTo(hdc, ex, ey);
    }
    SelectObject(hdc, oldp);
    DeleteObject(ringBr);
    DeleteObject(ringPen);

    /* Huruf "S" tebal di tengah logo */
    SetBkMode(hdc, TRANSPARENT);
    HFONT hLogo = CreateFontW(40, 0, 0, 0, FW_BLACK, 0, 0, 0,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_SWISS, L"Segoe UI");
    HFONT oldf = (HFONT)SelectObject(hdc, hLogo);
    SetTextColor(hdc, RGB(235, 245, 250));
    SIZE tsz;
    const wchar_t *S = L"S";
    GetTextExtentPoint32W(hdc, S, 1, &tsz);
    TextOutW(hdc, cx - tsz.cx / 2, cy - tsz.cy / 2, S, 1);

    /* ==== Teks judul ==== */
    HFONT hTitle = CreateFontW(30, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_SWISS, L"Segoe UI");
    SelectObject(hdc, hTitle);
    SetTextColor(hdc, RGB(240, 246, 250));
    TextOutW(hdc, 132, 56, L"Schematic Editor", 16);

    HFONT hSub = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_SWISS, L"Segoe UI");
    SelectObject(hdc, hSub);
    SetTextColor(hdc, RGB(120, 190, 180));
    TextOutW(hdc, 133, 92, L"Simulasi Rangkaian Elektronika \u2014 Win32 GDI", 43);

    /* Garis pemisah halus */
    HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(50, 75, 92));
    oldp = (HPEN)SelectObject(hdc, sepPen);
    MoveToEx(hdc, 30, 140, NULL);
    LineTo(hdc, SPLASH_W - 30, 140);
    SelectObject(hdc, oldp);
    DeleteObject(sepPen);

    /* Versi & hak cipta */
    SetTextColor(hdc, RGB(140, 155, 170));
    TextOutW(hdc, 30, 150, L"Versi 1.0", 9);
    HFONT hFoot = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_SWISS, L"Segoe UI");
    SelectObject(hdc, hFoot);
    SetTextColor(hdc, RGB(100, 115, 130));
    TextOutW(hdc, 30, SPLASH_H - 34, L"\u00A9 2026 Schematic Editor Project", 31);

    SelectObject(hdc, oldf);
    DeleteObject(hLogo);
    DeleteObject(hTitle);
    DeleteObject(hSub);
    DeleteObject(hFoot);
}

/* ---- kanvas = latar pra-render + bar progres (diubah tiap tick) ---- */
static void splash_paint_progress(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP canvas = CreateCompatibleBitmap(hdc, SPLASH_W, SPLASH_H);
    HBITMAP oldc = (HBITMAP)SelectObject(mem, canvas);

    /* blit latar */
    if (g_splash_bmp) {
        HDC bg = CreateCompatibleDC(hdc);
        HBITMAP oldbg = (HBITMAP)SelectObject(bg, g_splash_bmp);
        BitBlt(mem, 0, 0, SPLASH_W, SPLASH_H, bg, 0, 0, SRCCOPY);
        SelectObject(bg, oldbg);
        DeleteDC(bg);
    } else {
        RECT fill = { 0, 0, SPLASH_W, SPLASH_H };
        HBRUSH b = CreateSolidBrush(RGB(20, 30, 48));
        FillRect(mem, &fill, b);
        DeleteObject(b);
    }

    /* Track bar */
    int bx = 30, by = 182, bw = SPLASH_W - 60, bh = 10;
    HPEN trkPen = CreatePen(PS_SOLID, 1, RGB(60, 82, 100));
    HPEN oldp = (HPEN)SelectObject(mem, trkPen);
    HBRUSH trkBr = CreateSolidBrush(RGB(28, 40, 56));
    HBRUSH oldb = (HBRUSH)SelectObject(mem, trkBr);
    RoundRect(mem, bx, by, bx + bw, by + bh, 8, 8);
    SelectObject(mem, oldp);
    SelectObject(mem, oldb);
    DeleteObject(trkPen);
    DeleteObject(trkBr);

    /* Isi bar (gradien sederhana: blok hijau-cyan makin terang) */
    int fw = (bw - 4) * g_prog / 100;
    if (fw > 2) {
        HBRUSH fBr = CreateSolidBrush(RGB(0, 190, 160));
        oldb = (HBRUSH)SelectObject(mem, fBr);
        HPEN fPen = CreatePen(PS_SOLID, 1, RGB(0, 150, 130));
        oldp = (HPEN)SelectObject(mem, fPen);
        RoundRect(mem, bx + 2, by + 2, bx + 2 + fw, by + bh - 2, 6, 6);
        SelectObject(mem, oldp);
        SelectObject(mem, oldb);
        DeleteObject(fPen);
        DeleteObject(fBr);
    }

    /* Teks Loading + persen */
    SetBkMode(mem, TRANSPARENT);
    HFONT hSm = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_SWISS, L"Segoe UI");
    HFONT oldf = (HFONT)SelectObject(mem, hSm);
    SetTextColor(mem, RGB(160, 175, 190));
    TextOutW(mem, bx, by + 18, L"Mempersiapkan editor...", 23);

    wchar_t pct[8];
    swprintf(pct, 8, L"%d%%", g_prog);
    SIZE psz; GetTextExtentPoint32W(mem, pct, (int)wcslen(pct), &psz);
    SetTextColor(mem, RGB(0, 200, 170));
    TextOutW(mem, bx + bw - psz.cx, by + 18, pct, (int)wcslen(pct));
    SelectObject(mem, oldf);
    DeleteObject(hSm);

    BitBlt(hdc, 0, 0, SPLASH_W, SPLASH_H, mem, 0, 0, SRCCOPY);

    SelectObject(mem, oldc);
    DeleteObject(canvas);
    DeleteDC(mem);
    ReleaseDC(hwnd, hdc);
}

static LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, SPLASH_TICK_MS, NULL);
            return 0;

        case WM_TIMER: {
            DWORD now = GetTickCount();
            int target = (int)((now - g_start_tick) * 100 / SPLASH_MS);
            if (target > 100) target = 100;
            /* percepat sedikit lalu jeda di 100 sampai durasi min lewat */
            if (g_prog < target) g_prog += 3;
            if (g_prog > 100) g_prog = 100;
            splash_paint_progress(hwnd);
            if (g_prog >= 100 && (now - g_start_tick) >= (DWORD)SPLASH_MS) {
                KillTimer(hwnd, 1);
                DestroyWindow(hwnd);   /* -> WM_DESTROY di bawah */
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            splash_paint_progress(hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:   /* klik = lewati splash */
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (hwnd == g_splash_hwnd) {
                g_splash_hwnd = NULL;
                if (g_splash_bmp) { DeleteObject(g_splash_bmp); g_splash_bmp = NULL; }
            }
            /* PENTING: jangan pernah panggil PostQuitMessage di sini.
               Splash ditutup dengan DestroyWindow biasa; pesan WM_QUIT
               hanya boleh berasal dari window utama (WndProc). */
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ===================== API publik ===================== */

void splash_show(HINSTANCE hInst)
{
    static const wchar_t *CLS = L"SplashEditorWnd";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = SplashProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;               /* digambar manual (anti flicker) */
    wc.lpszClassName = CLS;
    RegisterClassW(&wc);

    /* pra-render latar ke bitmap sekali saja */
    HDC screen = GetDC(NULL);
    if (screen) {
        HDC m = CreateCompatibleDC(screen);
        g_splash_bmp = CreateCompatibleBitmap(screen, SPLASH_W, SPLASH_H);
        HBITMAP old = (HBITMAP)SelectObject(m, g_splash_bmp);
        splash_render_background(m);
        SelectObject(m, old);
        DeleteDC(m);
        ReleaseDC(NULL, screen);
    }

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x = (sw - SPLASH_W) / 2;
    int y = (sh - SPLASH_H) / 2;

    g_prog = 0;
    g_start_tick = GetTickCount();

    g_splash_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLS, L"Schematic Editor",
        WS_POPUP,
        x, y, SPLASH_W, SPLASH_H,
        NULL, NULL, hInst, NULL);

    if (g_splash_hwnd) {
        ShowWindow(g_splash_hwnd, SW_SHOW);
        UpdateWindow(g_splash_hwnd);
    } else {
        /* gagal membuat window splash -> jangan sampai loop menunggu */
        if (g_splash_bmp) { DeleteObject(g_splash_bmp); g_splash_bmp = NULL; }
    }
}

int splash_active(void)
{
    return g_splash_hwnd != NULL && IsWindow(g_splash_hwnd);
}
