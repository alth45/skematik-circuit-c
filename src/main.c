#include "schematic.h"

/* ---- Splash screen (src/ui/splash.c) ----
   Ditampilkan lebih dulu saat .exe dijalankan; window utama baru muncul
   setelah splash selesai (atau dilewati dengan klik).
   main_hwnd = jendela utama yang SUDAH dibuat (dalam keadaan hidden);
   splash akan ShowWindow saat animasi fade-out selesai. */
void splash_run(HINSTANCE hInst, HWND main_hwnd);
int splash_active(void); /* 1 selama splash masih tampil */

/* ===================== Window Procedure ===================== */
static void setup_keyboard(HWND hwnd) { SetFocus(hwnd); }

static void toolbar_click(HWND hwnd, int x, int y) {
  /* y relatif terhadap strip toolbar komponen (di bawah menubar) */
  int idx = toolbar_hit(x, y);
  if (idx < 0)
    return;
  if (idx == CLEAR_BTN_INDEX) {
    menubar_command(hwnd, IDM_EDIT_CLEAR);
  } else if (idx == SAVE_BTN_INDEX) {
    model_save_dialog(hwnd);
  } else if (idx == OPEN_BTN_INDEX) {
    model_load_dialog(hwnd);
    sim_recompute();
  } else if (idx == RUN_BTN_INDEX) {
    sim_set_running(!sim_running);
  } else {
    if (sim_running)
      return; /* ganti tool dikunci saat RUN */
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
    if (y < MENUBAR_H) {
      menubar_click(hwnd, x, y);
      return 0;
    }
    if (y < TOOLBAR_H) {
      toolbar_click(hwnd, x, y - MENUBAR_H);
      return 0;
    }
    setup_keyboard(hwnd);
    y -= TOOLBAR_H;
    on_lbutton_down(hwnd, x, y);
    return 0;
  }

  case WM_RBUTTONDOWN: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H) {
      if (y < MENUBAR_H)
        menubar_click(hwnd, x, y);
      return 0;
    }
    y -= TOOLBAR_H;
    on_rbutton_down(hwnd, x, y);
    return 0;
  }

  case WM_MOUSEMOVE: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H)
      y = TOOLBAR_H;
    y -= TOOLBAR_H;
    on_mouse_move(hwnd, x, y);
    return 0;
  }

  case WM_LBUTTONUP: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H)
      y = TOOLBAR_H;
    y -= TOOLBAR_H;
    on_lbutton_up(hwnd, x, y);
    return 0;
  }

  case WM_LBUTTONDBLCLK: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < MENUBAR_H) {
      menubar_click(hwnd, x, y);
      return 0;
    }
    if (y < TOOLBAR_H)
      return 0;
    y -= TOOLBAR_H;
    on_lbutton_dblclk(hwnd, x, y);
    return 0;
  }

  case WM_RBUTTONDBLCLK: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H)
      return 0;
    y -= TOOLBAR_H;
    on_rbutton_dblclk(hwnd, x, y);
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

    HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(memDC, &rc, bg);
    DeleteObject(bg);

    draw_toolbar(memDC, W);

    SaveDC(memDC);
    SetViewportOrgEx(memDC, 0, TOOLBAR_H, NULL);
    int canvas_h = H - TOOLBAR_H - (g_show_status ? STATUS_H : 0);
    IntersectClipRect(memDC, 0, 0, W, canvas_h);

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(235, 235, 235));
    HPEN op = (HPEN)SelectObject(memDC, gridPen);
    if (g_show_grid) {
      for (int x = 0; x < W; x += GRID) {
        MoveToEx(memDC, x, 0, NULL);
        LineTo(memDC, x, canvas_h);
      }
      for (int y = 0; y < canvas_h; y += GRID) {
        MoveToEx(memDC, 0, y, NULL);
        LineTo(memDC, W, y);
      }
    }
    SelectObject(memDC, op);
    DeleteObject(gridPen);

    draw_wires(memDC);
    for (int i = 0; i < comp_count; i++) {
      draw_component(memDC, &comps[i]);
    }

    draw_selection_overlay(memDC, memDC);

    if (sim_running) {
      const wchar_t *msg = sim_message();
      COLORREF fg, bgc;
      if (sim_short) {
        fg = RGB(255, 255, 255);
        bgc = RGB(200, 40, 40);
      } else if (sim_led_count > 0) {
        fg = RGB(255, 255, 255);
        bgc = RGB(20, 150, 60);
      } else {
        fg = RGB(60, 60, 60);
        bgc = RGB(255, 235, 160);
      }
      HFONT hB = CreateFontW(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0,
                             L"Segoe UI");
      HFONT oB = (HFONT)SelectObject(memDC, hB);
      SIZE bsz;
      GetTextExtentPoint32W(memDC, msg, (int)wcslen(msg), &bsz);
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

    if (g_show_status) {
      RECT sr = {0, H - STATUS_H, W, H};
      HBRUSH sb = CreateSolidBrush(RGB(245, 245, 245));
      FillRect(memDC, &sr, sb);
      DeleteObject(sb);
      HPEN sp2 = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
      HPEN osp4 = (HPEN)SelectObject(memDC, sp2);
      MoveToEx(memDC, 0, H - STATUS_H, NULL);
      LineTo(memDC, W, H - STATUS_H);
      SelectObject(memDC, osp4);
      DeleteObject(sp2);
      HFONT hSt = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0,
                              L"Segoe UI");
      HFONT oSt = (HFONT)SelectObject(memDC, hSt);
      SetTextColor(memDC, RGB(60, 60, 60));
      SetBkMode(memDC, TRANSPARENT);
      TextOutW(memDC, 10, H - STATUS_H + 5, g_status, (int)wcslen(g_status));
      wchar_t cnt[64];
      swprintf(cnt, 64, L"%d komponen, %d kabel", comp_count, wire_count);
      SIZE csz;
      GetTextExtentPoint32W(memDC, cnt, (int)wcslen(cnt), &csz);
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
    return 1;

  case WM_GETMINMAXINFO: {
    MINMAXINFO *mmi = (MINMAXINFO *)lp;
    mmi->ptMinTrackSize.x = 1120;
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

/* Handle jendela utama (dibuat tersembunyi; splash yang menampilkannya
   setelah fade-out selesai). */
static HWND g_main_hwnd = NULL;

/* Buat jendela utama dalam keadaan HIDDEN + register kelas window. */
static void main_window_create(HINSTANCE hInst) {
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = L"SchematicEditorWnd";

  RegisterClassW(&wc);

  g_main_hwnd =
      CreateWindowExW(0, L"SchematicEditorWnd", L"Schematic Editor - Win32 GDI",
                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100,
                      650, NULL, NULL, hInst, NULL);

  /* Jendela utama dibuat TERSEMBUNYI dulu; splash yang menampilkannya
     saat fade-out selesai (urutan: splash -> main). */
  ShowWindow(g_main_hwnd, SW_HIDE);
  UpdateWindow(g_main_hwnd);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int show) {
  (void)hPrev;
  (void)cmd;
  (void)show;

  /* 1) Buat jendela utama (hidden). */
  main_window_create(hInst);

  /* 2) Tampilkan splash; ia akan reveal main window saat fade-out selesai. */
  splash_run(hInst, g_main_hwnd);

  /* 3) Accelerator klasik untuk menu bar: Ctrl+N/O/S, F5 (Run/Stop), F9. */
  ACCEL accels[] = {
      {FCONTROL | FVIRTKEY, 'N', IDM_FILE_NEW},
      {FCONTROL | FVIRTKEY, 'O', IDM_FILE_OPEN},
      {FCONTROL | FVIRTKEY, 'S', IDM_FILE_SAVE},
      {FVIRTKEY, VK_F5, IDM_SIM_RUN_TOGGLE},
      {FVIRTKEY, VK_F9, IDM_SIM_RECOMPUTE},
  };
  HACCEL hAccel = CreateAcceleratorTableW(
      accels, (int)(sizeof(accels) / sizeof(accels[0])));

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (hAccel && TranslateAcceleratorW(g_main_hwnd, hAccel, &msg))
      continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  if (hAccel)
    DestroyAcceleratorTable(hAccel);
  return (int)msg.wParam;
}