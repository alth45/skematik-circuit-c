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
        case TOOL_VOLTMETER:  return COMP_VOLTMETER;
        case TOOL_AMMETER:    return COMP_AMMETER;
        case TOOL_OSCPROBE:   return COMP_OSCPROBE;
        case TOOL_INDUCTOR:   return COMP_INDUCTOR;
        case TOOL_ZENER:      return COMP_ZENER;
        case TOOL_MOSFET:     return COMP_MOSFET;
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
#define IDM_V_WAVE_SINE     0x0408
#define IDM_V_WAVE_SQUARE   0x0409
#define IDM_V_WAVE_TRIANGLE 0x040A
#define IDM_V_OPEN_OSC      0x040B

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
        case COMP_VOLTMETER:  return L"Voltmeter";
        case COMP_AMMETER:    return L"Ammeter";
        case COMP_OSCPROBE:   return L"Probe Osiloskop";
        case COMP_INDUCTOR:   return L"Induktor";
        case COMP_ZENER:      return L"Dioda Zener";
        case COMP_MOSFET:     return L"MOSFET N-Channel";
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
    BOOL is_ac   = c && c->type == COMP_ACSOURCE;
    BOOL is_osc  = c && c->type == COMP_OSCPROBE;

    AppendMenuW(m, MF_STRING | (has_val ? 0 : MF_GRAYED),
        IDM_V_EDIT_VALUE, L"Edit Nilai (Value)...");
    if (is_sw)
        AppendMenuW(m, MF_STRING, IDM_V_TOGGLE_SW,
            c->on ? L"Saklar: Matikan (OFF)" : L"Saklar: Nyalakan (ON)");
    if (is_ldr)
        AppendMenuW(m, MF_STRING, IDM_V_LDR_LIGHT,
            c->on ? L"Cahaya: Terang (R rendah) -> ubah ke Gelap"
                  : L"Cahaya: Gelap (R tinggi) -> ubah ke Terang");
    if (is_ac) {
        HMENU wm = CreatePopupMenu();
        AppendMenuW(wm, MF_STRING | (c->wave == WAVE_SINE     ? MF_CHECKED : 0),
                    IDM_V_WAVE_SINE,     L"Sinus");
        AppendMenuW(wm, MF_STRING | (c->wave == WAVE_SQUARE   ? MF_CHECKED : 0),
                    IDM_V_WAVE_SQUARE,   L"Kotak (Square)");
        AppendMenuW(wm, MF_STRING | (c->wave == WAVE_TRIANGLE ? MF_CHECKED : 0),
                    IDM_V_WAVE_TRIANGLE, L"Segitiga (Triangle)");
        AppendMenuW(m, MF_POPUP, (UINT_PTR)wm, L"Bentuk Gelombang");
    }
    if (is_osc)
        AppendMenuW(m, MF_STRING, IDM_V_OPEN_OSC, L"Buka Osiloskop...");
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
                if (c->type == COMP_VOLTMETER) {
                    /* informasi alat ukur: bacaan beda potensial saat RUN */
                    wchar_t extra[160];
                    if (!sim_running) {
                        wcscpy_s(extra, 160, L"\nPasang paralel: pin (+) ke titik A, pin (-) ke titik B."
                                          L" Tekan RUN untuk membaca tegangan.");
                    } else if (sim_voltmeter_connected(c)) {
                        double v = sim_voltmeter_reading(c);
                        wchar_t vs[32];
                        swprintf(vs, 32, L"%.3g V", v);
                        swprintf(extra, 160, L"\nBacaan: %ls%s", vs,
                            v < 0.0 ? L"  (kutub terbalik)" : L"");
                    } else {
                        wcscpy_s(extra, 160, L"\nBacaan: - (salah satu ujung tidak terhubung).");
                    }
                    wcscat_s(buf, 512, extra);
                }
                if (c->type == COMP_AMMETER) {
                    /* informasi alat ukur: bacaan arus cabang saat RUN */
                    wchar_t extra[200];
                    if (!sim_running) {
                        wcscpy_s(extra, 200, L"\nPasang SERI pada cabang yang diukur: putuskan kabel, "
                                          L"sambungkan pin masuk ke sisi datang dan pin keluar ke sisi pergi. "
                                          L"Tekan RUN untuk membaca arus.");
                    } else if (sim_ammeter_connected(c)) {
                        double a = sim_ammeter_reading(c);
                        if (a < 0.0) {
                            wcscpy_s(extra, 200, L"\nBacaan: - (cabang tidak dialiri arus / terbuka).");
                        } else if (a == 0.0) {
                            wcscpy_s(extra, 200, L"\nBacaan: ~ (cabang short 0 \x03A9 — arus teoretis tak hingga, korslet!).");
                        } else {
                            wchar_t as[32];
                            sim_format_si(a, L"A", as, 32);
                            swprintf(extra, 200, L"\nBacaan arus cabang: %ls", as);
                        }
                    } else {
                        wcscpy_s(extra, 200, L"\nBacaan: - (salah satu ujung tidak terhubung).");
                    }
                    wcscat_s(buf, 512, extra);
                }
                if (c->type == COMP_OSCPROBE) {
                    wchar_t extra[128];
                    wcscpy_s(extra, 128,
                        L"\nHubungkan pin kiri (sinyal) ke titik ukur dan pin kanan\n"
                        L"ke titik referensi. Buka View > Oscilloscope untuk\n"
                        L"melihat gelombangnya.");
                    wcscat_s(buf, 512, extra);
                }
                if (c->type == COMP_INDUCTOR) {
                    wchar_t extra[128];
                    swprintf(extra, 128,
                        L"\nInduktansi: %ls\n"
                        L"DC: short (0 Ω) | AC (%.0f Hz): XL = %.4g Ω",
                        (c->v > 0.0) ? L"pakai nilai" : L"default 100 uH",
                        50.0,
                        2.0 * 3.14159265 * 50.0 *
                            ((c->v > 0.0) ? c->v : 100e-6));
                    wcscat_s(buf, 512, extra);
                }
                if (c->type == COMP_ZENER) {
                    wchar_t extra[128];
                    double vz = (c->v > 0.0) ? c->v : VZENER_DEFAULT;
                    const wchar_t *st = L"off";
                    if (sim_running) {
                        if (sim_zener_breakdown(c)) st = L"breakdown (meng-clamp)";
                        else if (sim_zener_fwd(c))  st = L"forward (menghantar)";
                    }
                    swprintf(extra, 128,
                        L"\nVz = %.3g V\n"
                        L"Forward: Vf ≈ 0.7 V | Breakdown: V_K = V_A + Vz\n"
                        L"Status RUN: %ls",
                        vz, st);
                    wcscat_s(buf, 512, extra);
                }
                if (c->type == COMP_MOSFET) {
                    wchar_t extra[160];
                    double vth = (c->v > 0.0) ? c->v : MOSFET_VTH_DEFAULT;
                    const wchar_t *st = L"off";
                    if (sim_running)
                        st = sim_comp_conducting(c) ? L"ON (D-S menghantar)" : L"OFF";
                    swprintf(extra, 160,
                        L"\nTipe: N-Channel Enhancement MOSFET\n"
                        L"Vth (threshold gate) = %.3g V\n"
                        L"pin 0 = D (Drain), 1 = S (Source), 2 = G (Gate)\n"
                        L"Status RUN: %ls",
                        vth, st);
                    wcscat_s(buf, 512, extra);
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
                case IDM_V_WAVE_SINE:
        case IDM_V_WAVE_SQUARE:
        case IDM_V_WAVE_TRIANGLE:
            if (c && c->type == COMP_ACSOURCE) {
                c->wave = (cmd == IDM_V_WAVE_SQUARE)   ? WAVE_SQUARE :
                          (cmd == IDM_V_WAVE_TRIANGLE) ? WAVE_TRIANGLE :
                                                         WAVE_SINE;
                sim_recompute();
                osc_refresh();
                const wchar_t *nm =
                    (c->wave == WAVE_SQUARE)   ? L"kotak" :
                    (c->wave == WAVE_TRIANGLE) ? L"segitiga" : L"sinus";
                set_status(L"Sumber AC #%d: gelombang %ls.", c->id, nm);
            }
            break;
        case IDM_V_OPEN_OSC:
            osc_open(hwnd);
            break;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

/* ===================== Event Handler ===================== */

/* Helper: mulai drag GRUP (semua anggota seleksi bergerak rigid).
   Menyimpan posisi awal tiap anggota + offset grab agar on_mouse_move
   bisa menempatkan ulang seluruh blok tanpa akumulasi error pembulatan. */
static void begin_group_drag(HWND hwnd, Component *hit, int x, int y) {
    group_drag_n = 0;
    /* item[0] HARUS komponen yang di-grab (anchor delta drag) */
    group_drag_items[group_drag_n].idx = (int)(hit - comps);
    group_drag_items[group_drag_n].x0 = hit->x;
    group_drag_items[group_drag_n].y0 = hit->y;
    group_drag_n++;
    for (int i = 0; i < sel_count && group_drag_n < MAX_GROUP_DRAG; i++) {
        if (selection[i] == hit->id) continue;   /* sudah tercatat sebagai anchor */
        Component *c = find_comp_by_id(selection[i]);
        if (!c) continue;
        group_drag_items[group_drag_n].idx = (int)(c - comps);
        group_drag_items[group_drag_n].x0 = c->x;
        group_drag_items[group_drag_n].y0 = c->y;
        group_drag_n++;
    }
    group_drag_ox = hit->x - x;   /* offset grab dalam pixel mentah */
    group_drag_oy = hit->y - y;
    group_dragging = 1;
    dragging_id = -1;
    SetCapture(hwnd);
}

/* Mulai "marquee" (kotak blok) saat klik kiri ditekan dan user menyeret.
   box_x1..box_y2 = titik asal penekanan tombol (koordinat canvas). */
static void begin_box_select(int x, int y) {
    box_selecting = 1;
    box_x1 = box_x2 = x;
    box_y1 = box_y2 = y;
}

/* Diselesaikan saat mouse-move: terapkan seleksi kotak + status pesan.
   add=1 (Shift) -> gabungkan dengan seleksi yang sudah ada. */
static void apply_box_select(HWND hwnd, BOOL add) {
    int n = model_select_rect(box_x1, box_y1, box_x2, box_y2, add ? 1 : 0);
    if (n > 0) {
        set_status(L"Blok: %d komponen terpilih — tahan klik kiri dan drag "
                   L"blok ini untuk memindah (drag & drop), Del menghapus, "
                   L"Ctrl+D menduplikat.", n);
    } else {
        if (!add) clear_selection();
        set_status(L"Blok: tidak ada komponen yang terkena kotak.");
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

/* State "marquee tertunda": klik kiri sudah DITEKAN di area kosong canvas,
   tetapi kotak blok baru dianggap dimulai setelah kursor bergeser lebih dari
   BOX_DRAG_MIN piksel. Dengan begitu klik biasa (tanpa geser) tetap berfungsi
   membatalkan seleksi, sementara tekan + seret langsung memblok rangkaian. */
static int box_pending = 0;
static int box_pending_x = 0, box_pending_y = 0;

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
            clear_selection();
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
        /* Delete di atas anggota seleksi = hapus SELURUH blok (multi-delete) */
        Component *hit = find_comp_at(x, y);
        if (hit && sel_count > 1) {
            int i;
            for (i = 0; i < sel_count; i++) if (selection[i] == hit->id) break;
            if (i < sel_count) {
                int n = model_delete_selection();
                set_status(L"%d komponen (blok terpilih) dihapus.", n);
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
        }
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

    /* TOOL_BOX: mulai drag kotak seleksi (blok rangkaian). Klik kiri DITEKAN
       lalu diseret -> marquee; saat dilepas semua komponen di dalamnya terseleksi
       dan bisa langsung di-drag and drop sebagai satu grup. */
    if (current_tool == TOOL_BOX) {
        box_pending = 0;
        group_dragging = 0;
        dragging_id = -1;
        wiring_active = 0;
        begin_box_select(x, y);
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    /* TOOL_SELECT: prioritas pin -> mulai wiring cepat,
       kalau tidak kena pin -> pilih/drag (tunggal atau grup).
       Di area kosong: klik kiri ditekan + seret = blok marquee (multi-select),
       sama seperti canvas umumnya — tanpa perlu berpindah tool. */
    if (current_tool == TOOL_SELECT) {
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

        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        Component *hit = find_comp_at(x, y);
        if (hit) {
            if (ctrl) {
                toggle_selection(hit->id);          /* Ctrl+klik: tambah/keluar */
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
            int already = 0;
            for (int i = 0; i < sel_count; i++)
                if (selection[i] == hit->id) { already = 1; break; }
            if (already && sel_count > 1) {
                begin_group_drag(hwnd, hit, x, y);  /* drag seluruh blok */
            } else {
                clear_selection();                  /* seleksi tunggal baru */
                add_to_selection(hit->id);
                dragging_id = hit->id;              /* drag satu komponen */
                drag_dx = hit->x - x;
                drag_dy = hit->y - y;
                SetCapture(hwnd);
            }
        } else {
            /* area kosong: siapkan marquee. box_selecting baru benar-benar
               aktif setelah kursor bergeser > BOX_DRAG_MIN (lihat mouse-move),
               sehingga klik biasa tetap membatalkan seleksi. */
            begin_box_select(x, y);
            box_selecting = 0;      /* tunggu ambang geser dulu */
            group_dragging = 0;
            dragging_id = -1;
            SetCapture(hwnd);
            box_pending_x = x; box_pending_y = y;
            box_pending = 1;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
}

void on_lbutton_up(HWND hwnd, int x, int y) {
    /* Marquee tertunda dilepas tanpa pernah melewati ambang geser:
       berarti ini klik biasa di area kosong -> batalkan seleksi. */
    if (box_pending) {
        box_pending = 0;
        box_selecting = 0;
        clear_selection();
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    if (box_selecting) {
        box_selecting = 0;
        BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        box_x2 = x; box_y2 = y;
        apply_box_select(hwnd, shift);
        ReleaseCapture();
        return;
    }
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
    if (group_dragging) {
        group_dragging = 0;
        group_drag_n = 0;
        ReleaseCapture();
        sim_recompute();
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
    group_dragging = 0;
    wiring_active = 0;
    box_selecting = 0;
    box_pending = 0;
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
    /* Klik kiri sedang ditekan di area kosong: aktifkan marquee begitu
       kursor bergeser melewati ambang -> semua rangkaian dalam kotak ikut
       terseleksi secara LIVE saat menyeret (feedback visual langsung). */
    if (box_pending && !box_selecting) {
        if (abs(x - box_pending_x) > BOX_DRAG_MIN ||
            abs(y - box_pending_y) > BOX_DRAG_MIN) {
            box_pending = 0;
            box_selecting = 1;
            box_x1 = box_pending_x; box_y1 = box_pending_y;
            box_x2 = x; box_y2 = y;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return;
    }
    if (box_selecting) {
        box_x2 = x; box_y2 = y;
        /* pratinjau seleksi real-time (marquee umum di canvas lain) */
        BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        model_select_rect(box_x1, box_y1, box_x2, box_y2, shift ? 1 : 0);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    if (wiring_active) {
        wiring_mx = x; wiring_my = y;
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    if (group_dragging && group_drag_n > 0) {
        /* posisi "ideal" komponen yang di-grab, lalu dibulatkan ke grid;
           seluruh grup digeser dengan delta yang sama (rigid, tetap se-grid).
           items[0] selalu anggota yang sedang di-grab (lihat begin_group_drag). */
        int nx = snap(x + group_drag_ox);
        int ny = snap(y + group_drag_oy);
        int dx = nx - group_drag_items[0].x0;
        int dy = ny - group_drag_items[0].y0;
        for (int i = 0; i < group_drag_n; i++) {
            Component *c = &comps[group_drag_items[i].idx];
            c->x = group_drag_items[i].x0 + dx;
            c->y = group_drag_items[i].y0 + dy;
        }
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
        /* Ctrl + +/-/0 = zoom canvas */
        if (key == VK_OEM_PLUS || key == VK_ADD) {
            canvas_zoom_center(hwnd, 1.25);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        if (key == VK_OEM_MINUS || key == VK_SUBTRACT) {
            canvas_zoom_center(hwnd, 1.0 / 1.25);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        if (key == '0' || key == VK_NUMPAD0) {
            RECT rc; GetClientRect(hwnd, &rc);
            int canvas_h = rc.bottom - TOOLBAR_H - (g_show_status ? STATUS_H : 0);
            canvas_zoom_fit(rc.right, canvas_h);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
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
        if (key == 'A' || key == 'a') {          /* Ctrl+A: pilih semua */
            current_tool = TOOL_SELECT;
            sel_count = 0; selected_id = -1;
            for (int i = 0; i < comp_count; i++) add_to_selection(comps[i].id);
            set_status(L"%d komponen terpilih (semua).", sel_count);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        if (key == 'D' || key == 'd') {          /* Ctrl+D: duplikat blok */
            if (sim_running) return;
            if (sel_count > 1) {
                int n = model_duplicate_selection(3 * GRID, 3 * GRID);
                if (n > 0) set_status(L"Blok %d komponen diduplikat.", n);
                else set_status(L"Duplikat gagal: tidak ada ruang/komponen.");
            } else if (selected_id >= 0) {
                Component *c = find_comp_by_id(selected_id);
                if (c) {
                    CompType t = c->type; double v = c->v;
                    int rot = c->rot, on = c->on;
                    model_add_component(t, c->x + 3 * GRID, c->y + 3 * GRID);
                    Component *n = find_comp_by_id(selected_id);
                    if (n) { n->v = v; n->rot = rot; n->on = on; sim_recompute(); }
                    set_status(L"Komponen diduplikat.");
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        /* Ctrl + tool shortcut (mis. Ctrl+B untuk Blok) jangan dianggap B/LDR.
           Tambahan 'M' (Voltmeter) & 'I' (Ammeter) diblok di sini agar
           Ctrl+M / Ctrl+I tidak memicu pemilihan tool. */
        if (key == 'B' || key == 'b' || key == 'L' || key == 'l' ||
            key == 'U' || key == 'u' || key == 'V' || key == 'v' ||
            key == 'W' || key == 'w' || key == 'M' || key == 'm' ||
            key == 'I' || key == 'i' || key == 'P' || key == 'p' ||
            key == 'J' || key == 'j' || key == 'Z' || key == 'z' || key == 'F' || key == 'f') return;
    }
    if (key == 'R' || key == 'r') {
        if (sim_running) return; /* rotasi dikunci saat RUN */
        /* Utamakan yang sedang di-drag, kalau tidak ada pakai seleksi
           (grup terputar semua — rotasi tiap anggota di sekitar pusatnya) */
        int id = (dragging_id >= 0) ? dragging_id : selected_id;
        Component *c = find_comp_by_id(id);
        if (!c) return;
        if (sel_count > 1) {
            for (int i = 0; i < sel_count; i++) {
                Component *g = find_comp_by_id(selection[i]);
                if (g) g->rot = (g->rot + 1) % 4;
            }
            set_status(L"Blok %d komponen diputar 90\xB0.", sel_count);
        } else {
            selected_id = c->id;
            c->rot = (c->rot + 1) % 4;
        }
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
    else if (key == 'B' || key == 'b') { current_tool = TOOL_BOX; }
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
    else if (key == 'M' || key == 'm') { current_tool = TOOL_VOLTMETER; }
    else if (key == 'I' || key == 'i') { current_tool = TOOL_AMMETER; }
    else if (key == 'P' || key == 'p') { current_tool = TOOL_OSCPROBE; }
    else if (key == 'J' || key == 'j') { current_tool = TOOL_INDUCTOR; }
    else if (key == 'Z' || key == 'z') { current_tool = TOOL_ZENER; }
    else if (key == 'F' || key == 'f') { current_tool = TOOL_MOSFET; }
    else if (key == VK_DELETE || key == VK_BACK) {
        if (sim_running) return; /* hapus dikunci saat RUN */
        if (sel_count > 1) {
            /* multi-select: hapus SELURUH blok sekaligus */
            int n = model_delete_selection();
            set_status(L"%d komponen (blok terpilih) dihapus.", n);
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
        if (selected_id >= 0) {
            Component *c = find_comp_by_id(selected_id);
            if (c) { model_remove_at(c->x, c->y); sim_recompute(); }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return;
    } else if (key == VK_ESCAPE) {
        clear_selection();
        dragging_id = -1;
        group_dragging = 0;
        box_selecting = 0;
        box_pending = 0;
        wiring_active = 0;
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    } else {
        return;
    }
    dragging_id = -1;
    group_dragging = 0;
    wiring_active = 0;
    InvalidateRect(hwnd, NULL, FALSE);
}