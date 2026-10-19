#include "schematic.h"
#include <stdarg.h>
#include <stdlib.h>

wchar_t g_status[256] = L"Siap. Klik ikon di toolbar, lalu klik canvas.";

void set_status(const wchar_t *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(g_status, 256, fmt, ap);
    g_status[255] = L'\0';
    va_end(ap);
}

static const wchar_t *comp_type_name(CompType t) {
    switch (t) {
        case COMP_RESISTOR: return L"resistor";
        case COMP_SWITCH:   return L"switch";
        case COMP_BATTERY:  return L"battery";
        case COMP_ACSOURCE: return L"acsource";
        case COMP_LED:      return L"led";
        case COMP_GROUND:   return L"ground";
        case COMP_CAPACITOR:  return L"capacitor";
        case COMP_TRANSISTOR: return L"transistor";
        case COMP_DIODE:      return L"diode";
        case COMP_CAPOLAR:    return L"capolar";
        case COMP_VOLTMETER:  return L"voltmeter";
        case COMP_AMMETER:    return L"ammeter";
        case COMP_LDR:        return L"ldr";
        case COMP_BUZZER:     return L"buzzer";
        case COMP_OSCPROBE:  return L"oscprobe";
        case COMP_INDUCTOR:  return L"inductor";
        case COMP_ZENER:     return L"zener";
        case COMP_MOSFET:    return L"mosfet";

        default: return L"none";
    }
}

const char *comp_type_json_name(CompType t) {
    switch (t) {
        case COMP_RESISTOR:   return "resistor";
        case COMP_SWITCH:     return "switch";
        case COMP_BATTERY:    return "battery";
        case COMP_ACSOURCE:   return "acsource";
        case COMP_LED:        return "led";
        case COMP_GROUND:     return "ground";
        case COMP_CAPACITOR:  return "capacitor";
        case COMP_TRANSISTOR: return "transistor";
        case COMP_DIODE:      return "diode";
        case COMP_CAPOLAR:    return "capolar";
        case COMP_VOLTMETER:  return "voltmeter";
        case COMP_AMMETER:    return "ammeter";
        case COMP_LDR:        return "ldr";
        case COMP_BUZZER:     return "buzzer";
        case COMP_OSCPROBE:   return "oscprobe";
        case COMP_INDUCTOR:   return "inductor";
        case COMP_ZENER:      return "zener";
        case COMP_MOSFET:     return "mosfet";
        default:              return "none";
    }
}

static int comp_type_from_name(const char *s) {
    if (!s) return COMP_NONE;
    if (strcmp(s, "resistor") == 0) return COMP_RESISTOR;
    if (strcmp(s, "switch") == 0)   return COMP_SWITCH;
    if (strcmp(s, "battery") == 0)  return COMP_BATTERY;
    if (strcmp(s, "acsource") == 0 || strcmp(s, "ACSource") == 0 ||
        strcmp(s, "ac_source") == 0 || strcmp(s, "ac") == 0 ||
        strcmp(s, "AC") == 0 || strcmp(s, "SumberAC") == 0) return COMP_ACSOURCE;
    if (strcmp(s, "led") == 0)      return COMP_LED;
    if (strcmp(s, "ground") == 0)   return COMP_GROUND;
    if (strcmp(s, "Resistor") == 0 || strcmp(s, "R") == 0) return COMP_RESISTOR;
    if (strcmp(s, "Switch") == 0 || strcmp(s, "SW") == 0)  return COMP_SWITCH;
    if (strcmp(s, "Battery") == 0 || strcmp(s, "BAT") == 0) return COMP_BATTERY;
    if (strcmp(s, "LED") == 0) return COMP_LED;
    if (strcmp(s, "Ground") == 0 || strcmp(s, "GND") == 0) return COMP_GROUND;
    if (strcmp(s, "capacitor") == 0) return COMP_CAPACITOR;
    if (strcmp(s, "Capacitor") == 0 || strcmp(s, "C") == 0 || strcmp(s, "CAP") == 0) return COMP_CAPACITOR;
    if (strcmp(s, "transistor") == 0) return COMP_TRANSISTOR;
    if (strcmp(s, "Transistor") == 0 || strcmp(s, "Q") == 0 || strcmp(s, "NPN") == 0) return COMP_TRANSISTOR;
    if (strcmp(s, "diode") == 0 || strcmp(s, "Diode") == 0 || strcmp(s, "D") == 0) return COMP_DIODE;
    if (strcmp(s, "capolar") == 0 || strcmp(s, "Capolar") == 0 ||
        strcmp(s, "capacitor_polar") == 0 || strcmp(s, "electrolytic") == 0 ||
        strcmp(s, "CP") == 0) return COMP_CAPOLAR;
    if (strcmp(s, "voltmeter") == 0 || strcmp(s, "Voltmeter") == 0 ||
        strcmp(s, "VM") == 0) return COMP_VOLTMETER;
    if (strcmp(s, "ammeter") == 0 || strcmp(s, "Ammeter") == 0 ||
        strcmp(s, "AM") == 0) return COMP_AMMETER;
    if (strcmp(s, "ldr") == 0 || strcmp(s, "LDR") == 0 ||
        strcmp(s, "photoresistor") == 0) return COMP_LDR;
    if (strcmp(s, "buzzer") == 0 || strcmp(s, "Buzzer") == 0 ||
        strcmp(s, "BZR") == 0) return COMP_BUZZER;
    if (strcmp(s, "oscprobe") == 0 || strcmp(s, "OSCProbe") == 0 ||
        strcmp(s, "OSC") == 0) return COMP_OSCPROBE;
    if (strcmp(s, "inductor") == 0 || strcmp(s, "Inductor") == 0 ||
        strcmp(s, "L") == 0 || strcmp(s, "IND") == 0) return COMP_INDUCTOR;
    if (strcmp(s, "zener") == 0 || strcmp(s, "Zener") == 0 ||
        strcmp(s, "Z") == 0 || strcmp(s, "ZD") == 0 ||
        strcmp(s, "zenerdiode") == 0) return COMP_ZENER;
    if (strcmp(s, "mosfet") == 0 || strcmp(s, "MOSFET") == 0 ||
        strcmp(s, "nmos") == 0 || strcmp(s, "NMOS") == 0 ||
        strcmp(s, "FET") == 0) return COMP_MOSFET;
    return COMP_NONE;
}

int comp_type_from_json_name(const char *s) {
    return comp_type_from_name(s);
}

int model_save_json(const wchar_t *path, wchar_t *err, int err_cap) {
    FILE *f = _wfopen(path, L"w, ccs=UTF-8");
    if (!f) {
        if (err) swprintf(err, err_cap, L"Tidak bisa menulis file.");
        return 0;
    }
    fwprintf(f, L"{\n");
    fwprintf(f, L"  \"app\": \"schematic-c\",\n");
    fwprintf(f, L"  \"version\": 1,\n");
    fwprintf(f, L"  \"components\": [\n");
    for (int i = 0; i < comp_count; i++) {
        wchar_t vnum[40];
        swprintf(vnum, 40, L"%.6g", comps[i].v);
        fwprintf(f, L"    {\"id\": %d, \"type\": \"%ls\", \"x\": %d, \"y\": %d, \"rot\": %d, \"on\": %d, \"wave\": %d, \"value\": %ls}%ls\n",
            comps[i].id, comp_type_name(comps[i].type),
            comps[i].x, comps[i].y, comps[i].rot % 4, comps[i].on ? 1 : 0,
            (int)comps[i].wave, vnum,
            (i + 1 < comp_count) ? L"," : L"");
    }
    fwprintf(f, L"  ],\n");
    fwprintf(f, L"  \"wires\": [\n");
    for (int i = 0; i < wire_count; i++) {
        fwprintf(f, L"    {\"a\": {\"id\": %d, \"pin\": %d}, \"b\": {\"id\": %d, \"pin\": %d}}%ls\n",
            wires[i].a.comp_id, wires[i].a.pin_idx,
            wires[i].b.comp_id, wires[i].b.pin_idx,
            (i + 1 < wire_count) ? L"," : L"");
    }
    fwprintf(f, L"  ]\n");
    fwprintf(f, L"}\n");
    fclose(f);
    return 1;
}

/* ---- Parser JSON minimal (tanpa library) ---- */
static void skip_ws(const char **p) {
    while (**p == ' ' || **p == '\t' || **p == '\r' || **p == '\n') (*p)++;
}

static int expect_char(const char **p, char c) {
    skip_ws(p);
    if (**p != c) return 0;
    (*p)++;
    return 1;
}

static int parse_string(const char **p, char *out, int cap) {
    skip_ws(p);
    if (**p != '"') return 0;
    (*p)++;
    int n = 0;
    while (**p && **p != '"') {
        char c = **p;
        if (c == '\\') {
            (*p)++;
            c = **p;
            if (!c) break;
            if (c == 'n') c = '\n';
            else if (c == 't') c = '\t';
        }
        if (n + 1 < cap) out[n++] = c;
        (*p)++;
    }
    out[n < cap ? n : cap - 1] = '\0';
    if (**p != '"') return 0;
    (*p)++;
    return 1;
}

static int parse_int(const char **p, int *out) {
    skip_ws(p);
    char *end = NULL;
    long v = strtol(*p, &end, 10);
    if (end == *p) return 0;
    *p = end;
    *out = (int)v;
    return 1;
}

static int parse_double(const char **p, double *out) {
    skip_ws(p);
    char *end = NULL;
    double v = strtod(*p, &end);
    if (end == *p) return 0;
    *p = end;
    *out = v;
    return 1;
}

/* Cari key JSON pada level struktur (BUKAN di dalam string value).
   `key` sudah termasuk tanda kutip pembuka-penutup, mis. "\"components\"".
   Cocokkan hanya bila karakter saat ini berada di luar string DAN
   substring persis sama (termasuk tanda kutip) diikuti `:` (boleh
   dipisah whitespace). Ini menggantikan strstr() naif yang bisa
   salah cocok di dalam string value komponen mana pun. */
static int find_key(const char **p, const char *key) {
    const char *s = *p;
    int in_str = 0, esc = 0;
    size_t klen = strlen(key);
    while (*s) {
        if (in_str) {
            if (esc)                  esc = 0;
            else if (*s == '\\')      esc = 1;
            else if (*s == '"')       in_str = 0;
            s++;
            continue;
        }
        if (*s == '"') {
            if (strncmp(s, key, klen) == 0) {
                const char *t = s + klen;
                while (*t == ' ' || *t == '\t' || *t == '\r' || *t == '\n') t++;
                if (*t == ':') {
                    *p = t + 1;
                    return 1;
                }
            }
            in_str = 1;
            s++;
            continue;
        }
        s++;
    }
    return 0;
}

int model_load_json(const wchar_t *path, wchar_t *err, int err_cap) {
    FILE *f = _wfopen(path, L"rb");
    if (!f) {
        if (err) swprintf(err, err_cap, L"File tidak ditemukan.");
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 4 * 1024 * 1024) {
        fclose(f);
        if (err) swprintf(err, err_cap, L"Ukuran file tidak valid.");
        return 0;
    }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        if (err) swprintf(err, err_cap, L"Memori habis.");
        return 0;
    }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    const char *p = buf;
    if (got >= 3 && (unsigned char)buf[0] == 0xEF &&
        (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) p += 3;

    Component tmp_comps[MAX_COMP];
    int tmp_n = 0, tmp_next = 1;
    Wire tmp_wires[MAX_WIRES];
    int tmp_wn = 0;

    const char *q = p;
    if (!find_key(&q, "\"components\"") || !expect_char(&q, '[')) {
        free(buf);
        if (err) swprintf(err, err_cap, L"JSON rusak: 'components' hilang.");
        return 0;
    }
    skip_ws(&q);
    if (*q == ']') { q++; }
    else {
        while (1) {
            int id = tmp_next, x = 0, y = 0, rot = 0, on = 0, wave = 0;
            double v = -1.0;
            char tname[32] = "";
            if (!expect_char(&q, '{')) goto bad;
            while (1) {
                char key[48] = "";
                if (!parse_string(&q, key, sizeof key)) goto bad;
                if (!expect_char(&q, ':')) goto bad;
                if (strcmp(key, "id") == 0) { if (!parse_int(&q, &id)) goto bad; }
                else if (strcmp(key, "type") == 0) {
                    if (!parse_string(&q, tname, sizeof tname)) goto bad;
                }
                else if (strcmp(key, "x") == 0) { if (!parse_int(&q, &x)) goto bad; }
                else if (strcmp(key, "y") == 0) { if (!parse_int(&q, &y)) goto bad; }
                else if (strcmp(key, "rot") == 0 || strcmp(key, "rotation") == 0) {
                    if (!parse_int(&q, &rot)) goto bad;
                }
                else if (strcmp(key, "value") == 0 || strcmp(key, "v") == 0) {
                    if (*q == '\"') {
                        char sv[64] = "";
                        if (!parse_string(&q, sv, sizeof sv)) goto bad;
                        double tv = 0.0;
                        if (parse_value_text_a(sv, &tv)) v = tv;
                    } else {
                        if (!parse_double(&q, &v)) goto bad;
                    }
                }else if (strcmp(key, "wave") == 0) {
                    if (!parse_int(&q, &wave)) goto bad;
                }else if (strcmp(key, "on") == 0 || strcmp(key, "closed") == 0 ||
                         strcmp(key, "state") == 0) {
                    if (*q == '"') {
                        char sv[16] = "";
                        if (!parse_string(&q, sv, sizeof sv)) goto bad;
                        if (strcmp(sv, "on") == 0 || strcmp(sv, "ON") == 0 ||
                            strcmp(sv, "1") == 0 || strcmp(sv, "closed") == 0) on = 1;
                    } else {
                        if (!parse_int(&q, &on)) goto bad;
                    }
                } else {
                    skip_ws(&q);
                    if (*q == '"') { char d[8]; parse_string(&q, d, sizeof d); }
                    else { int d; if (!parse_int(&q, &d)) { q++; } }
                }
                skip_ws(&q);
                if (*q == ',') { q++; continue; }
                if (*q == '}') { q++; break; }
                goto bad;
            }
            if (tmp_n < MAX_COMP) {
                int tp = comp_type_from_name(tname);
                if (tp != COMP_NONE) {
                    tmp_comps[tmp_n].type = (CompType)tp;
                    tmp_comps[tmp_n].x = x;
                    tmp_comps[tmp_n].y = y;
                    tmp_comps[tmp_n].id = id;
                    tmp_comps[tmp_n].rot = ((rot % 4) + 4) % 4;
                    tmp_comps[tmp_n].on =
                        ((tp == COMP_SWITCH || tp == COMP_LDR) && on) ? 1 : 0;
                    if (v > 0.0) tmp_comps[tmp_n].v = v;
                    else         tmp_comps[tmp_n].v = comp_default_value((CompType)tp);
                    tmp_n++;
                    if (id >= tmp_next) tmp_next = id + 1;
                    tmp_comps[tmp_n].wave =
                        (tp == COMP_ACSOURCE && wave >= 0 && wave <= 2)
                        ? (WaveKind)wave : WAVE_SINE;

                }
            }
            skip_ws(&q);
            if (*q == ',') { q++; continue; }
            if (*q == ']') { q++; break; }
            goto bad;
        }
    }

    if (!find_key(&q, "\"wires\"") || !expect_char(&q, '[')) {
        free(buf);
        if (err) swprintf(err, err_cap, L"JSON rusak: 'wires' hilang.");
        return 0;
    }
    skip_ws(&q);
    if (*q == ']') { q++; }
    else {
        while (1) {
            int aid = -1, apin = 0, bid = -1, bpin = 0;
            if (!expect_char(&q, '{')) goto bad;
            while (1) {
                char key[32] = "";
                if (!parse_string(&q, key, sizeof key)) goto bad;
                if (!expect_char(&q, ':')) goto bad;
                if (strcmp(key, "a") == 0 || strcmp(key, "b") == 0) {
                    int is_a = (key[0] == 'a');
                    if (!expect_char(&q, '{')) goto bad;
                    while (1) {
                        char k2[32] = "";
                        if (!parse_string(&q, k2, sizeof k2)) goto bad;
                        if (!expect_char(&q, ':')) goto bad;
                        int v = 0;
                        if (!parse_int(&q, &v)) goto bad;
                        if (strcmp(k2, "pin") == 0) {
                            if (is_a) apin = v; else bpin = v;
                        } else {
                            if (is_a) aid = v; else bid = v;
                        }
                        skip_ws(&q);
                        if (*q == ',') { q++; continue; }
                        if (*q == '}') { q++; break; }
                        goto bad;
                    }
                } else {
                    int d;
                    if (*q == '"') { char s2[16]; parse_string(&q, s2, sizeof s2); }
                    else if (!parse_int(&q, &d)) goto bad;
                }
                skip_ws(&q);
                if (*q == ',') { q++; continue; }
                if (*q == '}') { q++; break; }
                goto bad;
            }
            if (tmp_wn < MAX_WIRES && aid >= 0 && bid >= 0) {
                tmp_wires[tmp_wn].a.comp_id = aid;
                tmp_wires[tmp_wn].a.pin_idx = apin;
                tmp_wires[tmp_wn].b.comp_id = bid;
                tmp_wires[tmp_wn].b.pin_idx = bpin;
                tmp_wn++;
            }
            skip_ws(&q);
            if (*q == ',') { q++; continue; }
            if (*q == ']') { q++; break; }
            goto bad;
        }
    }
    free(buf);

    {
        Wire keep[MAX_WIRES];
        int kn = 0;
        for (int i = 0; i < tmp_wn; i++) {
            int fa = 0, fb = 0;
            for (int j = 0; j < tmp_n; j++) {
                if (tmp_comps[j].id == tmp_wires[i].a.comp_id) fa = 1;
                if (tmp_comps[j].id == tmp_wires[i].b.comp_id) fb = 1;
            }
            if (fa && fb) keep[kn++] = tmp_wires[i];
        }
        for (int i = 0; i < tmp_n; i++) comps[i] = tmp_comps[i];
        comp_count = tmp_n;
        for (int i = 0; i < kn; i++) wires[i] = keep[i];
        wire_count = kn;
        next_id = tmp_next;
        selected_id = -1;
        dragging_id = -1;
        wiring_active = 0;
        undo_reset();
        sim_recompute();
    }
    return 1;

bad:
    free(buf);
    if (err) swprintf(err, err_cap, L"JSON rusak / format tak dikenal.");
    return 0;
}

static void show_msg(HWND hwnd, const wchar_t *title, const wchar_t *fmt, ...) {
    wchar_t buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 512, fmt, ap);
    buf[511] = L'\0';
    va_end(ap);
    MessageBoxW(hwnd, buf, title, MB_OK | MB_ICONINFORMATION);
}

static void show_err(HWND hwnd, const wchar_t *title, const wchar_t *msg) {
    MessageBoxW(hwnd, msg, title, MB_OK | MB_ICONERROR);
}

void model_save_dialog(HWND hwnd) {
    wchar_t file[MAX_PATH] = L"schematic.json";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;
    wchar_t err[256] = L"";
    if (model_save_json(file, err, 256)) {
        const wchar_t *base = wcsrchr(file, L'\\');
        set_status(L"Tersimpan: %ls (%d komponen, %d kabel).",
            base ? base + 1 : file, comp_count, wire_count);
        show_msg(hwnd, L"Save JSON", L"Tersimpan:\n%ls\n\n%d komponen, %d kabel.",
            file, comp_count, wire_count);
    } else {
        show_err(hwnd, L"Save JSON", err[0] ? err : L"Gagal menyimpan.");
    }
}

void model_load_dialog(HWND hwnd) {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    wchar_t err[256] = L"";
    if (model_load_json(file, err, 256)) {
        const wchar_t *base = wcsrchr(file, L'\\');
        set_status(L"Dimuat: %ls (%d komponen, %d kabel).",
            base ? base + 1 : file, comp_count, wire_count);
        show_msg(hwnd, L"Open JSON", L"Dimuat:\n%ls\n\n%d komponen, %d kabel.",
            file, comp_count, wire_count);
    } else {
        show_err(hwnd, L"Open JSON", err[0] ? err : L"Gagal membuka.");
    }
}


void model_export_png_dialog(HWND hwnd) {
    wchar_t file[MAX_PATH] = L"schematic.png";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrFilter = L"PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt  = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    wchar_t err[256] = L"";
    if (export_canvas_png(hwnd, file, err, 256)) {
        const wchar_t *base = wcsrchr(file, L'\\');
        set_status(L"PNG tersimpan: %ls", base ? base + 1 : file);
        MessageBoxW(hwnd,
            L"Canvas berhasil diekspor ke PNG.",
            L"Export PNG", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(hwnd, err[0] ? err : L"Gagal menyimpan PNG.",
                    L"Export PNG", MB_OK | MB_ICONERROR);
    }
}