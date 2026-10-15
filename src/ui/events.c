#include "schematic.h"

static CompType tool_to_comp(ToolKind t) {
    switch (t) {
        case TOOL_RESISTOR:   return COMP_RESISTOR;
        case TOOL_SWITCH:     return COMP_SWITCH;
        case TOOL_BATTERY:    return COMP_BATTERY;
        case TOOL_ACSOURCE:   return COMP_ACSOURCE;
        case TOOL_LED:        return COMP_LED;
        case TOOL_GROUND:     return COMP_GROUND;
        case TOOL_CAPACITOR:  return COMP_CAPACITOR;
        case TOOL_TRANSISTOR: return COMP_TRANSISTOR;
        case TOOL_DIODE:      return COMP_DIODE;
        case TOOL_CAPOLAR:    return COMP_CAPOLAR;
        case TOOL_LDR:        return COMP_LDR;
        case TOOL_BUZZER:     return COMP_BUZZER;
        default: return COMP_NONE;
    }
}

/* ===================== Menu Konteks & Dialog Nilai ===================== */

/* ID command untuk item menu konteks (jauhi IDOK/IDCANCEL). */
#define IDM_V_EDIT_VALUE  0x0401
#define IDM_V_ROTATE      0x0402
#define IDM_V_DELETE      0x0403
#define IDM_V_DUPLICATE   0x0404
#define IDM_V_TOGGLE_SW   0x0405
#define IDM_V_INFO        0x0406
#define IDM_V_LDR_LIGHT   0x0407  /* LDR: ubah kondisi cahaya Terang/Gelap */

static const wchar_t *comp_kind_name(CompType t) {
    switch (t) {
        case COMP_RESISTOR:   return L"Resistor";
        case COMP_SWITCH:     return L"Saklar";
        case COMP_BATTERY:    return L"Baterai";
        case COMP_ACSOURCE:   return L"Sumber AC";
        case COMP_LED:        return L"LED";
        case COMP_GROUND:     return L"Ground";
        case COMP_CAPACITOR:  return L"Kapasitor";
        case COMP_TRANSISTOR: return L"Transistor NPN";
        case COMP_DIODE:      return L"Dioda";
        case COMP_CAPOLAR:    return L"Kapasitor Polar";
        case COMP_LDR:        return L"LDR (Fotoresistor)";
        case COMP_BUZZER:     return L"Buzzer";
        default:              return L"Komponen";
    }
}

/* Tampilkan menu konteks TrackPopupMenu di posisi kursor. */
void show_context_menu(HWND hwnd, int sx, int sy, Component *c) {
    HMENU m = CreatePopupMenu();
    if (!m) return;

    BOOL has_val = c ? comp_has_value(c->type) : FALSE;
    BOOL is_sw   = c && c->type == COMP_SWITCH;
    BOOL is_ldr  = c && c->type == COMP_LDR;

    AppendMenuW(m, MF_STRING | (has_val ? 0 : MF_GRAYED),
        IDM_V_EDIT_VALUE, L"Edit Nilai (Value)...");
    if (is_sw)
        AppendMenuW(m, MF_STRING, IDM_V_TOGGLE_SW,
            c->on ? L"Saklar: Matikan (OFF)" : L"Saklar: Nyalakan (ON)");
    if (is_ldr)
        AppendMenuW(m, MF_STRING, IDM_V_LDR_LIGHT,
            c->on ? L"Cahaya: Terang (R rendah) -> ubah ke Gelap"
                  : L"Cahaya: Gelap (R tinggi) -> ubah ke Terang");
    AppendMenuW(m, MF_STRING, IDM_V_ROTATE, L"Putar 90\xB0");
    AppendMenuW(m, MF_STRING, IDM_V_DUPLICATE, L"Duplikat");
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_V_INFO, L"Informasi & Hasil Hitung");
    AppendMenuW(m, MF_STRING, IDM_V_DELETE, L"Hapus Komponen");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(m,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        sx, sy, 0, hwnd, NULL);
    DestroyMenu(m);
    if (cmd == 0) return;

    /* komponen mungkin sudah tidak ada setelah aksi -> selalu cari ulang */
    switch (cmd) {
        case IDM_V_EDIT_VALUE:
            edit_value_dialog(hwnd, c);
            break;
        case IDM_V_TOGGLE_SW:
            if (c) {
                c->on = !c->on;
                sim_recompute();
                set_status(L"Saklar %ls.", c->on ? L"ON (tertutup)" : L"OFF (terbuka)");
            }
            break;
        case IDM_V_LDR_LIGHT:
            if (c && c->type == COMP_LDR) {
                c->on = !c->on;   /* on = 1 => TERANG (R rendah), 0 => GELAP */
                sim_recompute();
                double reff = sim_ldr_effective_r(c);
                wchar_t rs[32];
                format_value(reff, L"\x03A9", rs, 32);
                set_status(L"LDR #%d: %ls — R efektif \xB7 %ls",
                    c->id, c->on ? L"TERANG" : L"GELAP", rs);
            }
            break;
        case IDM_V_ROTATE:
            if (c) {
                selected_id = c->id;
                model_rotate_selected();
            }
            break;
        case IDM_V_DUPLICATE:
            if (c) {
                CompType t = c->type;
                double v = c->v;
                int rot = c->rot, on = c->on;
                int nx = c->x + 3 * GRID, ny = c->y + 3 * GRID;
                model_add_component(t, nx, ny);
                Component *n = find_comp_by_id(selected_id);
                if (n) { n->v = v; n->rot = rot; n->on = on; sim_recompute(); }
                set_status(L"Komponen diduplikat (nilai ikut tersalin).");
            }
            break;
        case IDM_V_INFO:
            if (c) {
                wchar_t buf[512];
                if (comp_has_value(c->type)) {
                    const wchar_t *unit = comp_value_unit(c->type);
                    wchar_t vs[VALUE_STR_CAP];
                    format_value(c->v, unit, vs, VALUE_STR_CAP);
                    swprintf(buf, 512, L"%s #%d — Nilai: %ls",
                        comp_kind_name(c->type), c->id, vs);
                    if (c->type == COMP_LDR) {
                        /* tampilkan juga resistansi efektif sesuai cahaya */
                        wchar_t rs[32];
                        format_value(sim_ldr_effective_r(c), L"\x03A9", rs, 32);
                        wchar_t extra[96];
                        swprintf(extra, 96, L"\nCahaya: %ls — R efektif: %ls",
                            c->on ? L"TERANG" : L"GELAP", rs);
                        wcscat_s(buf, 512, extra);
                    }
                } else if (c->type == COMP_SWITCH) {
                    swprintf(buf, 512, L"Saklar #%d — Status: %ls",
                        c->id, c->on ? L"ON (tertutup)" : L"OFF (terbuka)");
                } else if (c->type == COMP_TRANSISTOR) {
                    swprintf(buf, 512, L"Transistor NPN #%d — pin 0=Kolektor, 1=Emitor, 2=Basis",
                        c->id);
                } else {
                    swprintf(buf, 512, L"%s #%d", comp_kind_name(c->type), c->id);
                }
                if (sim_running && c->type == COMP_LED) {
                    double i = sim_comp_current(c);
                    if (i >= 1e8)       wcscat_s(buf, 512, L"\nArus: ~TAK HINGGA (korslet!)");
                    else if (i < 0.0)   wcscat_s(buf, 512, L"\nArus: tidak mengalir / tak terdefinisi.");
                    else {
                        wchar_t si[32];
                        sim_format_si(i, L"A", si, 32);
                        wchar_t extra[64];
                        swprintf(extra, 64, L"\nArus LED: %ls%s", si,
                            sim_comp_blown(c) ? L"  << BAHAYA/MELEDAK" : L"");
                        wcscat_s(buf, 512, extra);
                    }
                }
                if (c->type == COMP_BUZZER) {
                    wchar_t extra[96];
                    if (sim_running) {
                        swprintf(extra, 96, L"\nStatus RUN: %ls (%.4g Hz)",
                            sim_buzzer_on(c) ? L"BERBUNYI" : L"diam",
                            c->v > 0.0 ? c->v : BUZZER_FREQ_DEFAULT);
                    } else {
                        swprintf(extra, 96, L"\nFrekuensi: %.4g Hz (tekan RUN untuk menguji)",
                            c->v > 0.0 ? c->v : BUZZER_FREQ_DEFAULT);
                    }
                    wcscat_s(buf, 512, extra);
                }
                MessageBoxW(hwnd, buf, L"Informasi Komponen", MB_OK | MB_ICONINFORMATION);
            }
            break;
        case IDM_V_DELETE:
            if (c) {
                int idx = (int)(c - comps);
                int cx = comps[idx].x, cy = comps[idx].y;
                model_remove_at(cx, cy);
                sim_recompute();
            }
            break;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

/* ===================== Event Handler ===================== */
void on_lbutton_down(HWND hwnd, int x, int y) {
    /* MODE RUN: klik saklar = toggle ON/OFF, klik LDR = toggle Terang/Gelap */
    if (sim_running) {
        Component *hit = find_comp_at(x, y);
        if (hit && hit->type == COMP_SWITCH) {
            hit->on = !hit->on;
            selected_id = hit->id;
            sim_recompute();
            set_status(L"RUN: saklar %ls. %ls",
                hit->on ? L"ON (tertutup)" : L"OFF (terbuka)", sim_message());
        } else if (hit && hit->type == COMP_LDR) {
            hit->on = !hit->on;   /* on = 1 => TERANG */
            selected_id = hit->id;
            sim_recompute();
            wchar_t rs[32];
            format_value(sim_ldr_effective_r(hit), L"\x03A9", rs, 32);
            set_status(L"RUN: LDR #%d %ls (R \xC5\x92 %ls). %ls",
                hit->id, hit->on ? L"TERANG" : L"GELAP", rs, sim_message());
        } else if (hit) {
            selected_id = hit->id;
        } else {
            selected_id = -1;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    /* Tool placement komponen */
    CompType ct = tool_to_comp(current_tool);
    if (ct != COMP_NONE) {
        model_add_component(ct, x, y);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    if (current_tool == TOOL_DELETE) {
        model_remove_at(x, y);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    if (current_tool == TOOL_WIRE) {
        PinRef p; int px, py;
        if (find_pin_at(x, y, &p, &px, &py)) {
            wiring_active = 1;
            wiring_from = p;
            wiring_mx = x; wiring_my = y;
            SetCapture(hwnd);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    /* TOOL_SELECT: prioritas pin -> mulai wiring cepat,
       kalau tidak kena pin -> drag body komponen */
    {
        PinRef p; int px, py;
        if (find_pin_at(x, y, &p, &px, &py)) {
            wiring_active = 1;
            wiring_from = p;
            wiring_mx = x; wiring_my = y;
            selected_id = p.comp_id;
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }

    Component *hit = find_comp_at(x, y);
    if (hit) {
        selected_id = hit->id;
        dragging_id = hit->id;
        drag_dx = hit->x - x;
        drag_dy = hit->y - y;
        SetCapture(hwnd);
    } else {
        selected_id = -1;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

void on_lbutton_up(HWND hwnd, int x, int y) {
    if (wiring_active) {
        PinRef dst;
        if (find_pin_at(x, y, &dst, NULL, NULL)) {
            model_add_wire(wiring_from, dst);
            sim_recompute();
            selected_id = dst.comp_id;
        }
        wiring_active = 0;
        wiring_from.comp_id = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    if (dragging_id >= 0) {
        dragging_id = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void on_rbutton_down(HWND hwnd, int x, int y) {
    if (sim_running) return; /* edit dikunci saat RUN */
    dragging_id = -1;
    wiring_active = 0;
    Component *hit = find_comp_at(x, y);
    POINT pt; GetCursorPos(&pt);
    if (hit) {
        selected_id = hit->id;
        InvalidateRect(hwnd, NULL, FALSE);
        show_context_menu(hwnd, pt.x, pt.y, hit);
    } else {
        /* kosong: menu global */
        HMENU m = CreatePopupMenu();
        AppendMenuW(m, MF_STRING, IDM_V_DELETE + 100, L"Hapus Semua Kabel di Titik Ini");
        AppendMenuW(m, MF_STRING, IDM_V_DELETE + 101, L"Clear Canvas");
        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                 pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(m);
        if (cmd == IDM_V_DELETE + 100) {
            if (model_remove_wire_at(x, y)) { sim_recompute(); set_status(L"Kabel dihapus."); }
            else set_status(L"Tidak ada kabel di titik ini.");
        } else if (cmd == IDM_V_DELETE + 101) {
            model_clear_all();
            sim_recompute();
            set_status(L"Dibersihkan. Canvas kosong.");
        }
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void on_rbutton_dblclk(HWND hwnd, int x, int y) {
    if (sim_running) return;
    model_remove_at(x, y);
    sim_recompute();
    InvalidateRect(hwnd, NULL, FALSE);
}

/* Double-klik kiri pada komponen bernilai -> dialog Edit Nilai. */
void on_lbutton_dblclk(HWND hwnd, int x, int y) {
    if (sim_running) return;
    Component *hit = find_comp_at(x, y);
    if (!hit) return;
    selected_id = hit->id;
    if (comp_has_value(hit->type)) {
        edit_value_dialog(hwnd, hit);
    } else if (hit->type == COMP_SWITCH) {
        hit->on = !hit->on;
        sim_recompute();
        set_status(L"Saklar %ls.", hit->on ? L"ON (tertutup)" : L"OFF (terbuka)");
    } else {
        wchar_t buf[96];
        swprintf(buf, 96, L"%s tidak memiliki nilai yang bisa diubah.",
            (hit->type == COMP_GROUND) ? L"Ground" :
            (hit->type == COMP_TRANSISTOR) ? L"Transistor NPN" : L"Komponen ini");
        set_status(L"%ls", buf);
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

void on_mouse_move(HWND hwnd, int x, int y) {
    if (wiring_active) {
        wiring_mx = x; wiring_my = y;
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    if (dragging_id >= 0) {
        Component *c = find_comp_by_id(dragging_id);
        if (c) {
            c->x = snap(x + drag_dx);
            c->y = snap(y + drag_dy);
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }
}

void on_key_down(HWND hwnd, WPARAM key) {
    /* Ctrl+S = save JSON, Ctrl+O = open JSON */
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        if (key == 'S' || key == 's') {
            model_save_dialog(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        if (key == 'O' || key == 'o') {
            model_load_dialog(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }
    if (key == 'R' || key == 'r') {
        if (sim_running) return; /* rotasi dikunci saat RUN */
        /* Utamakan yang sedang di-drag, kalau tidak ada pakai selected */
        int id = (dragging_id >= 0) ? dragging_id : selected_id;
        Component *c = find_comp_by_id(id);
        if (!c) return;
        selected_id = c->id;
        c->rot = (c->rot + 1) % 4;
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    /* F5 / F9 = toggle Run */
    if (key == VK_F5 || key == VK_F9) {
        sim_set_running(!sim_running);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    /* Shortcut tool toolbar */
    if (key == 'V' || key == 'v') { current_tool = TOOL_SELECT; }
    else if (key == 'W' || key == 'w') { current_tool = TOOL_WIRE; }
    else if (key == '1') { current_tool = TOOL_RESISTOR; }
    else if (key == '2') { current_tool = TOOL_SWITCH; }
    else if (key == '3') { current_tool = TOOL_BATTERY; }
    else if (key == '0') { current_tool = TOOL_ACSOURCE; }
    else if (key == '4') { current_tool = TOOL_LED; }
    else if (key == '5') { current_tool = TOOL_GROUND; }
    else if (key == '6') { current_tool = TOOL_CAPACITOR; }
    else if (key == '7') { current_tool = TOOL_TRANSISTOR; }
    else if (key == '8') { current_tool = TOOL_DIODE; }
    else if (key == '9') { current_tool = TOOL_CAPOLAR; }
    else if (key == 'L' || key == 'l') { current_tool = TOOL_LDR; }
    else if (key == 'U' || key == 'u') { current_tool = TOOL_BUZZER; }
    else if (key == VK_DELETE || key == VK_BACK) {
        if (sim_running) return; /* hapus dikunci saat RUN */
        if (selected_id >= 0) {
            Component *c = find_comp_by_id(selected_id);
            if (c) { model_remove_at(c->x, c->y); sim_recompute(); }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return;
    } else if (key == VK_ESCAPE) {
        selected_id = -1;
        dragging_id = -1;
        wiring_active = 0;
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    } else {
        return;
    }
    dragging_id = -1;
    wiring_active = 0;
    InvalidateRect(hwnd, NULL, FALSE);
}

