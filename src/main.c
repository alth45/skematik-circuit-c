#include "schematic.h"

static HFONT font_banner(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0,
                            L"Segoe UI");
    return h;
}
static HFONT font_status(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0,
                            L"Segoe UI");
    return h;
}

void splash_run(HINSTANCE hInst, HWND main_hwnd);

static void setup_keyboard(HWND hwnd) { SetFocus(hwnd); }

static void toolbar_click(HWND hwnd, int x, int y) {
  int idx = toolbar_hit(x, y);
  if (idx < 0) return;
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
    if (sim_running) return;
    current_tool = btnTools[idx];
    dragging_id = -1;
    wiring_active = 0;
    setup_keyboard(hwnd);
  }
  InvalidateRect(hwnd, NULL, FALSE);
}

/* State pan (drag dengan mouse tengah). */
static int g_pan_active   = 0;
static int g_pan_start_sx = 0, g_pan_start_sy = 0;
static int g_pan_orig_px  = 0, g_pan_orig_py  = 0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_CREATE:
    setup_keyboard(hwnd);
    return 0;

  case WM_LBUTTONDOWN: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < MENUBAR_H) { menubar_click(hwnd, x, y); return 0; }
    if (y < TOOLBAR_H) { toolbar_click(hwnd, x, y - MENUBAR_H); return 0; }
    setup_keyboard(hwnd);
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_lbutton_down(hwnd, cx, cy);
    return 0;
  }

  case WM_RBUTTONDOWN: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H) {
      if (y < MENUBAR_H) menubar_click(hwnd, x, y);
      return 0;
    }
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_rbutton_down(hwnd, cx, cy);
    return 0;
  }

  case WM_MOUSEMOVE: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (g_pan_active) {
      g_pan_x = g_pan_orig_px - (int)((x - g_pan_start_sx) / g_zoom);
      g_pan_y = g_pan_orig_py - (int)((y - g_pan_start_sy) / g_zoom);
      InvalidateRect(hwnd, NULL, FALSE);
      return 0;
    }
    if (y < TOOLBAR_H) y = TOOLBAR_H;
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_mouse_move(hwnd, cx, cy);
    return 0;
  }

  case WM_LBUTTONUP: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H) y = TOOLBAR_H;
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_lbutton_up(hwnd, cx, cy);
    return 0;
  }

  case WM_MBUTTONDOWN: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H) return 0;
    g_pan_active   = 1;
    g_pan_start_sx = x;
    g_pan_start_sy = y;
    g_pan_orig_px  = g_pan_x;
    g_pan_orig_py  = g_pan_y;
    SetCapture(hwnd);
    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    return 0;
  }

  case WM_MBUTTONUP:
    if (g_pan_active) {
      g_pan_active = 0;
      ReleaseCapture();
      SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    return 0;

  case WM_MOUSEWHEEL: {
    int delta = (short)HIWORD(wp);
    POINT pt;
    pt.x = (short)LOWORD(lp);
    pt.y = (short)HIWORD(lp);
    ScreenToClient(hwnd, &pt);
    if (pt.y < TOOLBAR_H) return 0;
    int sx = pt.x;
    int sy = pt.y - TOOLBAR_H;
    if (delta > 0)      canvas_zoom_at(sx, sy, 1.25);
    else if (delta < 0) canvas_zoom_at(sx, sy, 1.0 / 1.25);
    InvalidateRect(hwnd, NULL, FALSE);
    return 0;
  }

  case WM_LBUTTONDBLCLK: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < MENUBAR_H) { menubar_click(hwnd, x, y); return 0; }
    if (y < TOOLBAR_H) return 0;
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_lbutton_dblclk(hwnd, cx, cy);
    return 0;
  }

  case WM_RBUTTONDBLCLK: {
    int x = (short)LOWORD(lp);
    int y = (short)HIWORD(lp);
    if (y < TOOLBAR_H) return 0;
    int cx, cy;
    canvas_screen_to_canvas(x, y - TOOLBAR_H, &cx, &cy);
    on_rbutton_dblclk(hwnd, cx, cy);
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

    int canvas_h = H - TOOLBAR_H - (g_show_status ? STATUS_H : 0);
    if (canvas_h < 1) canvas_h = 1;

    SaveDC(memDC);
    SetViewportOrgEx(memDC, 0, TOOLBAR_H, NULL);
    IntersectClipRect(memDC, 0, 0, W, canvas_h);

    /* Grid: screen coords (sebelum world transform, supaya tajam). */
    if (g_show_grid) {
      HPEN gp = CreatePen(PS_SOLID, 1, RGB(235, 235, 235));
      HPEN oldp = (HPEN)SelectObject(memDC, gp);
      int first_gx = (int)(ceil((double)g_pan_x / GRID) * GRID);
      int first_gy = (int)(ceil((double)g_pan_y / GRID) * GRID);
      for (int gx = first_gx; ; gx += GRID) {
        int sx = (int)((gx - g_pan_x) * g_zoom);
        if (sx >= W) break;
        if (sx >= 0) {
          MoveToEx(memDC, sx, 0, NULL);
          LineTo(memDC, sx, canvas_h);
        }
      }
      for (int gy = first_gy; ; gy += GRID) {
        int sy = (int)((gy - g_pan_y) * g_zoom);
        if (sy >= canvas_h) break;
        if (sy >= 0) {
          MoveToEx(memDC, 0, sy, NULL);
          LineTo(memDC, W, sy);
        }
      }
      SelectObject(memDC, oldp);
      DeleteObject(gp);
    }

    /* Konten: world transform (zoom + pan). */
    int saved2 = SaveDC(memDC);
    SetGraphicsMode(memDC, GM_ADVANCED);
    XFORM xf = {
        (FLOAT)g_zoom, 0,
        0, (FLOAT)g_zoom,
        (FLOAT)(-g_pan_x * g_zoom),
        (FLOAT)(-g_pan_y * g_zoom)
    };
    SetWorldTransform(memDC, &xf);

    draw_wires(memDC);
    for (int i = 0; i < comp_count; i++)
      draw_component(memDC, &comps[i]);
    draw_selection_overlay(memDC);

    RestoreDC(memDC, saved2);

    /* Overlay: screen coords (masih ter-clip ke canvas). */
    draw_marquee_overlay(memDC);

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
      HFONT hB = font_banner();
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
      HFONT hSt = font_status();
      HFONT oSt = (HFONT)SelectObject(memDC, hSt);
      SetTextColor(memDC, RGB(60, 60, 60));
      SetBkMode(memDC, TRANSPARENT);
      TextOutW(memDC, 10, H - STATUS_H + 5, g_status, (int)wcslen(g_status));
      wchar_t cnt[96];
      swprintf(cnt, 96, L"%d komp, %d kabel | zoom %.0f%%",
               comp_count, wire_count, g_zoom * 100.0);
      SIZE csz;
      GetTextExtentPoint32W(memDC, cnt, (int)wcslen(cnt), &csz);
      TextOutW(memDC, W - csz.cx - 10, H - STATUS_H + 5, cnt, (int)wcslen(cnt));
      SelectObject(memDC, oSt);
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

static HWND g_main_hwnd = NULL;

static void main_window_create(HINSTANCE hInst) {
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = L"SchematicEditorWnd";
  RegisterClassW(&wc);

  g_main_hwnd = CreateWindowExW(0, L"SchematicEditorWnd",
      L"Schematic Editor - Win32 GDI",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 650,
      NULL, NULL, hInst, NULL);

  ShowWindow(g_main_hwnd, SW_HIDE);
  UpdateWindow(g_main_hwnd);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int show) {
  (void)hPrev; (void)cmd; (void)show;

  main_window_create(hInst);
  splash_run(hInst, g_main_hwnd);

  ACCEL accels[] = {
      {FCONTROL | FVIRTKEY, 'N', IDM_FILE_NEW},
      {FCONTROL | FVIRTKEY, 'O', IDM_FILE_OPEN},
      {FCONTROL | FVIRTKEY, 'S', IDM_FILE_SAVE},
      {FVIRTKEY, VK_F5, IDM_SIM_RUN_TOGGLE},
      {FVIRTKEY, VK_F9, IDM_SIM_RECOMPUTE},
      {FVIRTKEY, VK_F12, IDM_VIEW_OSC},
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