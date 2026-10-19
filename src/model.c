#include "schematic.h"

/* ===================== State Global ===================== */
Component comps[MAX_COMP];
int  comp_count   = 0;
int  next_id      = 1;
ToolKind current_tool = TOOL_SELECT;
int  selected_id  = -1;
int  dragging_id  = -1;
int  drag_dx = 0, drag_dy = 0;

Wire wires[MAX_WIRES];
int  wire_count = 0;

int wiring_active = 0;
PinRef wiring_from = { -1, -1 };
int wiring_mx = 0, wiring_my = 0;

/* ---- Multi-select / blok rangkaian ---- */
int selection[MAX_SEL];
int sel_count = 0;
int box_selecting = 0;
int box_x1 = 0, box_y1 = 0, box_x2 = 0, box_y2 = 0;
int group_dragging = 0;
GroupDragItem group_drag_items[MAX_GROUP_DRAG];
int group_drag_n = 0;
int group_drag_ox = 0, group_drag_oy = 0;

/* Preferensi tampilan (diubah lewat menu View) */
int g_show_grid   = 1;
int g_show_status = 1;

/* ===================== Zoom & Pan Canvas ===================== */
double g_zoom  = 1.0;
int    g_pan_x = 0;
int    g_pan_y = 0;

#define ZOOM_MIN  0.25
#define ZOOM_MAX  8.0

static double clamp_zoom(double z) {
    if (z < ZOOM_MIN) return ZOOM_MIN;
    if (z > ZOOM_MAX) return ZOOM_MAX;
    return z;
}

void canvas_screen_to_canvas(int sx, int sy, int *cx, int *cy) {
    if (cx) *cx = (int)(sx / g_zoom + g_pan_x);
    if (cy) *cy = (int)(sy / g_zoom + g_pan_y);
}

void canvas_canvas_to_screen(int cx, int cy, int *sx, int *sy) {
    if (sx) *sx = (int)((cx - g_pan_x) * g_zoom);
    if (sy) *sy = (int)((cy - g_pan_y) * g_zoom);
}

void canvas_zoom_at(int sx, int sy, double factor) {
    if (factor <= 0.0) return;
    int cx, cy;
    canvas_screen_to_canvas(sx, sy, &cx, &cy);
    double nz = clamp_zoom(g_zoom * factor);
    if (nz == g_zoom) return;
    g_zoom = nz;
    g_pan_x = cx - (int)(sx / g_zoom);
    g_pan_y = cy - (int)(sy / g_zoom);
}

void canvas_zoom_center(HWND hwnd, double factor) {
    if (!hwnd) return;
    RECT rc; GetClientRect(hwnd, &rc);
    int W = rc.right;
    int canvas_h = rc.bottom - TOOLBAR_H - (g_show_status ? STATUS_H : 0);
    if (canvas_h < 1) canvas_h = 1;
    canvas_zoom_at(W / 2, canvas_h / 2, factor);
}

void canvas_zoom_fit(int view_w, int view_h) {
    if (view_w < 1) view_w = 1;
    if (view_h < 1) view_h = 1;
    if (comp_count == 0) { canvas_zoom_reset(); return; }

    int minx = 0, miny = 0, maxx = 0, maxy = 0;
    for (int i = 0; i < comp_count; i++) {
        int hw, hh;
        comp_bounding_half(&comps[i], &hw, &hh);
        int x0 = comps[i].x - hw, y0 = comps[i].y - hh;
        int x1 = comps[i].x + hw, y1 = comps[i].y + hh;
        if (i == 0) { minx = x0; miny = y0; maxx = x1; maxy = y1; }
        else {
            if (x0 < minx) minx = x0;
            if (y0 < miny) miny = y0;
            if (x1 > maxx) maxx = x1;
            if (y1 > maxy) maxy = y1;
        }
    }
    int bw = maxx - minx; if (bw < 1) bw = 1;
    int bh = maxy - miny; if (bh < 1) bh = 1;

    int margin = 40;
    double zx = (double)(view_w - 2 * margin) / bw;
    double zy = (double)(view_h - 2 * margin) / bh;
    double z  = (zx < zy) ? zx : zy;
    g_zoom = clamp_zoom(z);

    int ccx = (minx + maxx) / 2;
    int ccy = (miny + maxy) / 2;
    g_pan_x = ccx - (int)(view_w / 2.0 / g_zoom);
    g_pan_y = ccy - (int)(view_h / 2.0 / g_zoom);
}

void canvas_zoom_reset(void) {
    g_zoom  = 1.0;
    g_pan_x = 0;
    g_pan_y = 0;
}

/* Tombol toolbar (digambar manual di atas, bukan HWND).
   Urutan: Select, Wire, Resistor, Switch, Battery, Sumber AC, LED, Ground,
   Capacitor, Transistor, Diode, Kapasitor Polar, LDR, Buzzer, Voltmeter,
   Delete. + 4 tombol aksi. */
const ToolKind btnTools[BTN_COUNT] = {
    /* Fixed-left group (indices 0..3): */
    TOOL_SELECT, TOOL_BOX, TOOL_WIRE, TOOL_DELETE,
    /* Scrollable components (indices 4..20): */
    TOOL_RESISTOR, TOOL_SWITCH, TOOL_BATTERY, TOOL_ACSOURCE, TOOL_LED, TOOL_GROUND,
    TOOL_CAPACITOR, TOOL_TRANSISTOR, TOOL_DIODE, TOOL_CAPOLAR,
    TOOL_LDR, TOOL_BUZZER, TOOL_VOLTMETER, TOOL_AMMETER,
    TOOL_OSCPROBE, TOOL_INDUCTOR, TOOL_ZENER, TOOL_MOSFET
};
const wchar_t *btnLabels[BTN_COUNT] = {
    L"Select / Geser (V)", L"Blok / Multi-Select (B)", L"Kabel / Wire (W)",
    L"Hapus / Delete (Del)",
    L"Resistor (1)", L"Saklar / Switch (2)", L"Baterai (3)", L"Sumber AC (0)",
    L"LED (4)", L"Ground (5)",
    L"Kapasitor (6)", L"Transistor NPN (7)", L"Dioda (8)", L"Kapasitor Polar (9)",
    L"LDR / Sensor Cahaya (L)", L"Buzzer (U)", L"Voltmeter (M)", L"Ammeter (I)",
    L"Probe Osiloskop (P)", L"Induktor (J)", L"Dioda Zener (Z)", L"MOSFET N-Channel (F)"
};

/* ===================== Utilitas ===================== */
int snap(int v) {
    return (int)(round((double)v / GRID) * GRID);
}

Component *find_comp_by_id(int id) {
    if (id < 0) return NULL;
    for (int i = 0; i < comp_count; i++)
        if (comps[i].id == id) return &comps[i];
    return NULL;
}

Component *find_comp_at(int x, int y) {
    for (int i = comp_count - 1; i >= 0; i--) {
        int hw, hh;
        comp_bounding_half(&comps[i], &hw, &hh);
        int dx = abs(comps[i].x - x);
        int dy = abs(comps[i].y - y);
        if (dx < hw && dy < hh) return &comps[i];
    }
    return NULL;
}

void comp_bounding_half(const Component *c, int *hw, int *hh) {
    int w = 48, h = 38;
    if (c->type == COMP_TRANSISTOR || c->type == COMP_MOSFET) { w = 52; h = 42; }
    if (c->rot % 2 != 0) { int t = w; w = h; h = t; }
    *hw = w; *hh = h;
}

int comp_pin_count(const Component *c) {
    if (!c) return 0;
    if (c->type == COMP_GROUND) return 1;
    if (c->type == COMP_TRANSISTOR || c->type == COMP_MOSFET) return 3;
    if (c->type == COMP_NONE) return 0;
    return 2;
}

int comp_pin_valid(const Component *c, int pin_idx) {
    if (!c || pin_idx < 0) return 0;
    return pin_idx < comp_pin_count(c);
}

void comp_local_to_world(const Component *c, int lx, int ly, int *wx, int *wy) {
    if (!c || !wx || !wy) return;
    int r = ((c->rot % 4) + 4) % 4;
    int rx = lx, ry = ly;
    if (r == 1)      { rx = -ly; ry = lx; }
    else if (r == 2) { rx = -lx; ry = -ly; }
    else if (r == 3) { rx = ly;  ry = -lx; }
    *wx = c->x + rx;
    *wy = c->y + ry;
}

void comp_pin_pos(const Component *c, int pin_idx, int *px, int *py) {
    if (!c || !px || !py) return;
    if (c->type == COMP_GROUND) {
        comp_local_to_world(c, 0, -30, px, py);
    }else if (c->type == COMP_TRANSISTOR || c->type == COMP_MOSFET) {
        /* NPN standar (B di kiri, C & E di kanan):
           pin0 = Kolektor -> kanan-atas  (+40, -30)
           pin1 = Emitor   -> kanan-bawah (+40, +30)  (panah keluar)
           pin2 = Basis    -> kiri-tengah (-40,  0) */
        if (pin_idx == 2)      comp_local_to_world(c, -40,  0, px, py);
        else if (pin_idx == 1) comp_local_to_world(c,  40, 30, px, py);
        else                   comp_local_to_world(c,  40, -30, px, py);
    }else {
        comp_local_to_world(c, (pin_idx == 0) ? -40 : 40, 0, px, py);
    }
}

int find_pin_at(int x, int y, PinRef *out, int *px, int *py) {
    int best_d2 = (PIN_R + 9) * (PIN_R + 9);
    int found = 0;
    PinRef best = { -1, -1 };
    int bx = 0, by = 0;
    for (int i = 0; i < comp_count; i++) {
        int n = comp_pin_count(&comps[i]);
        for (int p = 0; p < n; p++) {
            int qx, qy;
            comp_pin_pos(&comps[i], p, &qx, &qy);
            int dx = qx - x, dy = qy - y;
            int d2 = dx * dx + dy * dy;
            if (d2 <= best_d2) {
                best_d2 = d2;
                best.comp_id = comps[i].id;
                best.pin_idx = p;
                bx = qx; by = qy;
                found = 1;
            }
        }
    }
    if (found) {
        if (out) *out = best;
        if (px) *px = bx;
        if (py) *py = by;
    }
    return found;
}

static int same_pin(PinRef a, PinRef b) {
    return a.comp_id == b.comp_id && a.pin_idx == b.pin_idx;
}

/* ===================== Multi-Select / Blok Rangkaian ===================== */

void clear_selection(void) {
    sel_count = 0;
    selected_id = -1;
}

int add_to_selection(int id) {
    if (id < 0 || !find_comp_by_id(id)) return 0;
    for (int i = 0; i < sel_count; i++)
        if (selection[i] == id) return 0;
    if (sel_count >= MAX_SEL) return 0;
    selection[sel_count++] = id;
    selected_id = id;
    return 1;
}

void toggle_selection(int id) {
    for (int i = 0; i < sel_count; i++) {
        if (selection[i] == id) {
            for (int j = i; j < sel_count - 1; j++) selection[j] = selection[j + 1];
            sel_count--;
            selected_id = (sel_count > 0) ? selection[sel_count - 1] : -1;
            return;
        }
    }
    add_to_selection(id);
}

int model_select_rect(int x1, int y1, int x2, int y2, int add) {
    int lx = (x1 < x2) ? x1 : x2, rx = (x1 < x2) ? x2 : x1;
    int ty = (y1 < y2) ? y1 : y2, by = (y1 < y2) ? y2 : y1;
    if (!add) { sel_count = 0; selected_id = -1; }
    int n = 0;
    for (int i = 0; i < comp_count; i++) {
        int hw, hh;
        comp_bounding_half(&comps[i], &hw, &hh);
        if (comps[i].x - hw >= lx && comps[i].x + hw <= rx &&
            comps[i].y - hh >= ty && comps[i].y + hh <= by) {
            if (add) { if (add_to_selection(comps[i].id)) n++; }
            else if (sel_count < MAX_SEL) {
                selection[sel_count++] = comps[i].id;
                selected_id = comps[i].id;
                n++;
            }
        }
    }
    return n;
}

void model_translate_selection(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    for (int i = 0; i < sel_count; i++) {
        Component *c = find_comp_by_id(selection[i]);
        if (c) { c->x += dx; c->y += dy; }
    }
}

int model_delete_selection(void) {
    if (sim_running) return 0;
    int removed = 0;
    while (sel_count > 0) {
        Component *c = find_comp_by_id(selection[0]);
        if (!c) {
            for (int j = 0; j < sel_count - 1; j++) selection[j] = selection[j + 1];
            sel_count--;
            continue;
        }
        int cx = c->x, cy = c->y;
        model_remove_at(cx, cy);
        for (int j = 0; j < sel_count; ) {
            if (!find_comp_by_id(selection[j])) {
                for (int k = j; k < sel_count - 1; k++) selection[k] = selection[k + 1];
                sel_count--;
                removed++;
            } else j++;
        }
    }
    sel_count = 0;
    selected_id = -1;
    sim_recompute();
    return removed;
}

int model_duplicate_selection(int dx, int dy) {
    if (sim_running) return 0;
    if (sel_count == 0) return 0;
    if (comp_count + sel_count > MAX_COMP) return 0;

    int old_ids[MAX_SEL], new_ids[MAX_SEL];
    int n = sel_count;
    for (int i = 0; i < n; i++) old_ids[i] = selection[i];

    for (int i = 0; i < n; i++) {
        Component *c = find_comp_by_id(old_ids[i]);
        if (!c) { new_ids[i] = -1; continue; }
        CompType t = c->type;
        double v = c->v;
        int rot = c->rot, on = c->on;
        model_add_component(t, c->x + dx, c->y + dy);
        Component *nc = find_comp_by_id(selected_id);
        if (nc) { nc->v = v; nc->rot = rot; nc->on = on; }
        new_ids[i] = selected_id;
    }

    int nw = wire_count;
    for (int i = 0; i < wire_count && i < nw; i++) {
        int ai = -1, bi = -1;
        for (int k = 0; k < n; k++) {
            if (wires[i].a.comp_id == old_ids[k]) ai = k;
            if (wires[i].b.comp_id == old_ids[k]) bi = k;
        }
        if (ai >= 0 && bi >= 0 && new_ids[ai] > 0 && new_ids[bi] > 0) {
            PinRef a = { new_ids[ai], wires[i].a.pin_idx };
            PinRef b = { new_ids[bi], wires[i].b.pin_idx };
            model_add_wire(a, b);
        }
    }

    sel_count = 0;
    for (int i = 0; i < n; i++)
        if (new_ids[i] > 0) add_to_selection(new_ids[i]);
    sim_recompute();
    return n;
}

/* ===================== Operasi Model ===================== */

double comp_default_value(CompType t) {
    switch (t) {
        case COMP_RESISTOR:   return 220.0;
        case COMP_BATTERY:    return 9.0;
        case COMP_ACSOURCE:   return 220.0;
        case COMP_LED:        return VF_LED_DEFAULT;
        case COMP_DIODE:      return VF_DIODE;
        case COMP_CAPACITOR:  return 100e-9;
        case COMP_CAPOLAR:    return 100e-6;
        case COMP_LDR:        return LDR_DARK_DEFAULT;
        case COMP_BUZZER:     return BUZZER_FREQ_DEFAULT;
        case COMP_AMMETER:    return 0.0;
        case COMP_OSCPROBE:   return 0.0;
        case COMP_INDUCTOR:   return IND_DEFAULT_H;
        case COMP_ZENER:      return VZENER_DEFAULT;
        case COMP_MOSFET:     return MOSFET_VTH_DEFAULT;
        default:              return 0.0;
    }
}

int comp_has_value(CompType t) {
    return t == COMP_RESISTOR || t == COMP_BATTERY || t == COMP_ACSOURCE ||
           t == COMP_LED ||
           t == COMP_DIODE || t == COMP_CAPACITOR || t == COMP_CAPOLAR ||
           t == COMP_LDR || t == COMP_BUZZER || t == COMP_INDUCTOR ||
           t == COMP_ZENER || t == COMP_MOSFET;
}

const wchar_t *comp_value_unit(CompType t) {
    switch (t) {
        case COMP_RESISTOR:  return L"\x03A9";
        case COMP_BATTERY:   return L"V";
        case COMP_ACSOURCE:  return L"VAC";
        case COMP_LED:       return L"Vf";
        case COMP_DIODE:     return L"Vf";
        case COMP_CAPACITOR: return L"F";
        case COMP_CAPOLAR:   return L"F";
        case COMP_LDR:       return L"Rgelap";
        case COMP_BUZZER:    return L"Hz";
        case COMP_INDUCTOR:  return L"H";
        case COMP_ZENER:     return L"Vz";
        case COMP_MOSFET:    return L"Vth";
        default:             return NULL;
    }
}

static int parse_value_core(const wchar_t *s, double *out) {
    if (!s || !out) return 0;
    while (*s == L' ' || *s == L'\t') s++;
    if (!*s) return 0;
    wchar_t *end = NULL;
    double v = wcstod(s, &end);
    if (end == s) return 0;
    if (!(v >= 0.0)) return 0;
    while (*end == L' ' || *end == L'\t') end++;
    wchar_t mult = L'\0';
    if (*end && wcschr(L"\x03A9" L"vVfFuUnN", *end)) {
        const wchar_t *p2 = end;
        if ((p2[0] == L'O' || p2[0] == L'o') && (p2[1] == L'h' || p2[1] == L'H') &&
            (p2[2] == L'm' || p2[2] == L'M')) end += 3;
        else if (*p2 == L'\x03A9') end += 1;
    }
    while (*end == L' ') end++;
    if (*end) {
        wchar_t c = *end;
        if (c == L'k' || c == L'K') mult = L'k';
        else if (c == L'M')         mult = L'M';
        else if (c == L'm')         mult = L'm';
        else if (c == L'u' || c == L'\xB5' || c == L'\u03BC') mult = L'u';
        else if (c == L'n' || c == L'N') mult = L'n';
        else if (c == L'p' || c == L'P') mult = L'p';
        else if (c == L'e' || c == L'E') return 0;
        else return 0;
        end++;
        while (*end && (*end == L'F' || *end == L'f' || *end == L'V' || *end == L'v' ||
                        *end == L'\x03A9' || *end == L' ')) end++;
        while (*end) {
            if (*end != L' ') return 0;
            end++;
        }
    } else {
        end = (wchar_t *)end;
    }
    switch (mult) {
        case L'k': v *= 1e3; break;
        case L'M': v *= 1e6; break;
        case L'u': v *= 1e-6; break;
        case L'n': v *= 1e-9; break;
        case L'p': v *= 1e-12; break;
        default: break;
    }
    *out = v;
    return 1;
}

int parse_value_text(const wchar_t *s, double *out) {
    return parse_value_core(s, out);
}

int parse_value_text_a(const char *s, double *out) {
    wchar_t w[64];
    if (!s || !out) return 0;
    int i = 0;
    for (; i < 63 && s[i]; i++) w[i] = (wchar_t)(unsigned char)s[i];
    w[i] = 0;
    return parse_value_core(w, out);
}

void format_value(double v, const wchar_t *unit, wchar_t *out, int cap) {
    const wchar_t *pre = L"";
    double a = fabs(v);
    if (a >= 1e6)      { v /= 1e6; pre = L"M"; }
    else if (a >= 1e3) { v /= 1e3; pre = L"k"; }
    else if (a >= 1.0) { }
    else if (a >= 1e-3){ v /= 1e-3; pre = L"m"; }
    else if (a >= 1e-6){ v /= 1e-6; pre = L"u"; }
    else if (a >= 1e-9){ v /= 1e-9; pre = L"n"; }
    else if (a > 0.0)  { v /= 1e-12; pre = L"p"; }
    wchar_t num[32];
    if (v == floor(v) && fabs(v) < 1e15) swprintf(num, 32, L"%.0f", v);
    else                                 swprintf(num, 32, L"%g", v);
    swprintf(out, cap, L"%ls %ls%ls", num, pre, unit ? unit : L"");
}

void model_add_component(CompType type, int x, int y) {
    if (type == COMP_NONE) return;
    if (comp_count >= MAX_COMP) return;
    Component c;
    c.type = type;
    c.x = snap(x);
    c.y = snap(y);
    c.id = next_id++;
    c.rot = 0;
    c.on = 0;
    c.v = comp_default_value(type);
    comps[comp_count++] = c;
    selected_id = c.id;
    sim_recompute();
}

void model_remove_at(int x, int y) {
    if (model_remove_wire_at(x, y)) return;
    Component *hit = find_comp_at(x, y);
    if (!hit) return;
    int dead_id = hit->id;
    int idx = (int)(hit - comps);
    for (int i = idx; i < comp_count - 1; i++) comps[i] = comps[i + 1];
    comp_count--;
    for (int i = wire_count - 1; i >= 0; i--) {
        if (wires[i].a.comp_id == dead_id || wires[i].b.comp_id == dead_id) {
            for (int j = i; j < wire_count - 1; j++) wires[j] = wires[j + 1];
            wire_count--;
        }
    }
    if (selected_id == dead_id) selected_id = -1;
    if (dragging_id == dead_id) dragging_id = -1;
    if (wiring_active && wiring_from.comp_id == dead_id) wiring_active = 0;
    for (int i = 0; i < sel_count; ) {
        if (selection[i] == dead_id) {
            for (int j = i; j < sel_count - 1; j++) selection[j] = selection[j + 1];
            sel_count--;
        } else i++;
    }
    if (sel_count > 0 && selected_id < 0) selected_id = selection[sel_count - 1];
}

void model_clear_all(void) {
    comp_count = 0;
    wire_count = 0;
    selected_id = -1;
    dragging_id = -1;
    wiring_active = 0;
    sel_count = 0;
    group_dragging = 0;
    group_drag_n = 0;
    box_selecting = 0;
    for (int i = 0; i < MAX_COMP; i++) { comps[i].on = 0; comps[i].v = 0.0; }
    sim_recompute();
}

void model_rotate_selected(void) {
    Component *c = find_comp_by_id(selected_id);
    if (!c) return;
    c->rot = (c->rot + 1) % 4;
    sim_recompute();
}

int model_add_wire(PinRef a, PinRef b) {
    if (a.comp_id < 0 || b.comp_id < 0) return 0;
    if (same_pin(a, b)) return 0;
    Component *ca = find_comp_by_id(a.comp_id);
    Component *cb = find_comp_by_id(b.comp_id);
    if (!ca || !cb) return 0;
    if (!comp_pin_valid(ca, a.pin_idx) || !comp_pin_valid(cb, b.pin_idx)) return 0;
    for (int i = 0; i < wire_count; i++) {
        if ((same_pin(wires[i].a, a) && same_pin(wires[i].b, b)) ||
            (same_pin(wires[i].a, b) && same_pin(wires[i].b, a)))
            return 0;
    }
    if (wire_count >= MAX_WIRES) return 0;
    wires[wire_count].a = a;
    wires[wire_count].b = b;
    wire_count++;
    sim_recompute();
    return 1;
}

static double dist_pt_seg(int px, int py, int x1, int y1, int x2, int y2) {
    double dx = (double)(x2 - x1), dy = (double)(y2 - y1);
    double L2 = dx * dx + dy * dy;
    double t = 0.0;
    if (L2 > 0.0) {
        t = ((px - x1) * dx + (py - y1) * dy) / L2;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }
    double cx = x1 + t * dx, cy = y1 + t * dy;
    double ex = px - cx, ey = py - cy;
    return sqrt(ex * ex + ey * ey);
}

static int wire_hit(int x, int y, int x1, int y1, int x2, int y2) {
    if (dist_pt_seg(x, y, x1, y1, x2, y1) <= WIRE_HIT) return 1;
    if (dist_pt_seg(x, y, x2, y1, x2, y2) <= WIRE_HIT) return 1;
    return 0;
}

int model_remove_wire_at(int x, int y) {
    for (int i = wire_count - 1; i >= 0; i--) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        Component *cb = find_comp_by_id(wires[i].b.comp_id);
        if (!ca || !cb) {
            for (int j = i; j < wire_count - 1; j++) wires[j] = wires[j + 1];
            wire_count--;
            continue;
        }
        int ax, ay, bx, by;
        comp_pin_pos(ca, wires[i].a.pin_idx, &ax, &ay);
        comp_pin_pos(cb, wires[i].b.pin_idx, &bx, &by);
        if (wire_hit(x, y, ax, ay, bx, by)) {
            for (int j = i; j < wire_count - 1; j++) wires[j] = wires[j + 1];
            wire_count--;
            return 1;
        }
    }
    return 0;
}

/* ===================== Dialog Edit Nilai (Value) Komponen =====================
   Catatan: PostQuitMessage(0) TIDAK dipakai di WM_DESTROY dialog. Loop
   modal memakai flag ctx->done; setiap jalur penutupan memanggil
   ve_do_close() yang mem-PostThreadMessageW(WM_NULL) sebagai wake-up
   agar GetMessageW tidak memblokir setelah dialog dihancurkan. */

typedef struct {
    Component *c;
    HWND   edit;
    wchar_t start[VALUE_STR_CAP];
    int    ok;      /* 1 = user menekan OK dgn nilai valid */
    int    done;    /* 1 = dialog sudah dihancurkan (WM_DESTROY) */
} ValEditCtx;

#define VE_ID_EDIT 1001

static ValEditCtx *ve_get_ctx(HWND hDlg) {
    return (ValEditCtx *)(LONG_PTR)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
}

static void ve_set_ctx(HWND hDlg, ValEditCtx *ctx) {
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)ctx);
}

/* Tutup dialog secara "aman": DestroyWindow + wake-up thread queue. */
static void ve_do_close(ValEditCtx *ctx, HWND hDlg) {
    if (ctx) ctx->done = 1;
    if (hDlg && IsWindow(hDlg)) DestroyWindow(hDlg);
    PostThreadMessageW(GetCurrentThreadId(), WM_NULL, 0, 0);
}

/* Font dialog di-cache sekali per proses. Signature WAJIB `(void)`.
   Handle yang diberikan ke control lewat WM_SETFONT tidak boleh
   di-DeleteObject selama control masih hidup — karena itu font
   di-cache untuk umur proses, bukan dibuat/dihapus per dialog. */
static HFONT ve_font(void) {
    static HFONT h = NULL;
    if (!h) h = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    return h;
}

static LRESULT CALLBACK ValEditProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lp;
        if (cs && cs->lpCreateParams) ve_set_ctx(hDlg, (ValEditCtx *)cs->lpCreateParams);
        return DefWindowProcW(hDlg, msg, wp, lp);
    }

    ValEditCtx *ctx = ve_get_ctx(hDlg);

    switch (msg) {
        case WM_CREATE: {
            if (!ctx) return 0;
            HINSTANCE hi = ((CREATESTRUCTW *)lp)->hInstance;

            const int DLG_W = 380;
            const int cw = 12, ch = 10;
            const int lblH = 22;
            const int edX = cw, edY = ch + lblH + 4;
            const int edW = DLG_W - 2 * cw - 120;
            const int btnY = edY + 30 + 14;

            const wchar_t *unit = comp_value_unit(ctx->c->type);

            wchar_t lbl[96];
            swprintf(lbl, 96, L"Nilai (%ls) — Enter/OK untuk simpan:", unit ? unit : L"?");
            HWND hLbl = CreateWindowExW(0, L"STATIC", lbl,
                WS_CHILD | WS_VISIBLE | SS_LEFT, cw, ch, DLG_W - 2 * cw, lblH,
                hDlg, NULL, hi, NULL);

            HWND hEd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", ctx->start,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL,
                edX, edY, edW, 26, hDlg, (HMENU)(INT_PTR)VE_ID_EDIT, hi, NULL);
            ctx->edit = hEd;

            wchar_t hint[48];
            swprintf(hint, 48, L"satuan: %ls", unit ? unit : L"-");
            HWND hHint = CreateWindowExW(0, L"STATIC", hint,
                WS_CHILD | WS_VISIBLE | SS_LEFT, edX + edW + 10, edY + 4, 110, 20,
                hDlg, NULL, hi, NULL);

            HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                DLG_W - 190, btnY, 85, 28, hDlg, (HMENU)(INT_PTR)IDOK, hi, NULL);
            HWND hCa = CreateWindowExW(0, L"BUTTON", L"Batal",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                DLG_W - 100, btnY, 85, 28, hDlg, (HMENU)(INT_PTR)IDCANCEL, hi, NULL);

            HFONT hfont = ve_font();
            if (hfont) {
                SendMessageW(hLbl,  WM_SETFONT, (WPARAM)hfont, TRUE);
                SendMessageW(hEd,   WM_SETFONT, (WPARAM)hfont, TRUE);
                SendMessageW(hHint, WM_SETFONT, (WPARAM)hfont, TRUE);
                SendMessageW(hOk,   WM_SETFONT, (WPARAM)hfont, TRUE);
                SendMessageW(hCa,   WM_SETFONT, (WPARAM)hfont, TRUE);
            }
            /* font di-cache: JANGAN DeleteObject(hfont) */
            return 0;
        }

        case WM_SETFOCUS:
            if (ctx && ctx->edit) SetFocus(ctx->edit);
            return 0;

        case WM_COMMAND:
            if (!ctx) return 0;
            if (LOWORD(wp) == IDOK) {
                wchar_t txt[64] = L"";
                GetWindowTextW(ctx->edit, txt, 64);
                double v = 0.0;
                if (!parse_value_text(txt, &v)) {
                    MessageBoxW(hDlg,
                        L"Nilai tidak valid.\n\nContoh yang diterima:\n"
                        L"  resistor   : 220, 4.7k, 1M, 10k\n"
                        L"  baterai/AC : 9, 4.5, 220 (AC = VAC RMS)\n"
                        L"  kapasitor  : 100n, 47u, 10uF",
                        L"Edit Nilai", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                ctx->c->v = v;
                ctx->ok = 1;
                ve_do_close(ctx, hDlg);
                return 0;
            }
            if (LOWORD(wp) == IDCANCEL) {
                ve_do_close(ctx, hDlg);
                return 0;
            }
            return 0;

        case WM_CLOSE:
            ve_do_close(ctx, hDlg);
            return 0;

        case WM_DESTROY:
            if (ctx) ctx->done = 1;
            return 0;
    }
    return DefWindowProcW(hDlg, msg, wp, lp);
}

static ATOM val_edit_atom(void) {
    static ATOM atom = 0;
    if (atom) return atom;
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof wc;
    wc.lpfnWndProc   = ValEditProc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"SchematicValEditDlg";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    atom = RegisterClassExW(&wc);
    return atom;
}

void edit_value_dialog(HWND hwnd, Component *c) {
    if (!c || !comp_has_value(c->type)) return;

    ATOM a = val_edit_atom();
    if (!a) return;

    const wchar_t *unit = comp_value_unit(c->type);
    wchar_t start[VALUE_STR_CAP];
    format_value(c->v, unit, start, VALUE_STR_CAP);

    wchar_t title[96];
    swprintf(title, 96, L"Edit Nilai Komponen — %s",
        (c->type == COMP_RESISTOR)  ? L"Resistor" :
        (c->type == COMP_BATTERY)   ? L"Baterai" :
        (c->type == COMP_ACSOURCE)  ? L"Sumber AC (VAC RMS)" :
        (c->type == COMP_LED)       ? L"LED (Vf)" :
        (c->type == COMP_DIODE)     ? L"Dioda (Vf)" :
        (c->type == COMP_CAPACITOR) ? L"Kapasitor" :
        (c->type == COMP_CAPOLAR)   ? L"Kapasitor Polar" : L"Komponen");

    ValEditCtx ctx;
    ZeroMemory(&ctx, sizeof ctx);
    ctx.c = c;
    ctx.ok = 0;
    ctx.done = 0;
    lstrcpynW(ctx.start, start, VALUE_STR_CAP);

    const int DLG_W = 380, DLG_H = 150;
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        (LPCWSTR)(UINT_PTR)a, title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, DLG_W, DLG_H,
        hwnd, NULL, GetModuleHandleW(NULL), &ctx);
    if (!hDlg) return;

    RECT pr; GetWindowRect(hwnd, &pr);
    int px = pr.left + (((pr.right - pr.left) - DLG_W) / 2);
    int py = pr.top  + (((pr.bottom - pr.top) - DLG_H) / 2);
    if (px < 0) px = 0;
    if (py < 0) py = 0;
    SetWindowPos(hDlg, HWND_TOPMOST, px, py, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);
    if (ctx.edit) { SetFocus(ctx.edit); SendMessageW(ctx.edit, EM_SETSEL, 0, -1); }

    EnableWindow(hwnd, FALSE);

    MSG m;
    while (!ctx.done && GetMessageW(&m, NULL, 0, 0) > 0) {
        if (IsWindow(hDlg) && IsDialogMessageW(hDlg, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    if (IsWindow(hDlg)) DestroyWindow(hDlg);

    EnableWindow(hwnd, TRUE);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);

    if (ctx.ok) {
        wchar_t nv[VALUE_STR_CAP];
        format_value(c->v, unit, nv, VALUE_STR_CAP);
        set_status(L"Nilai baru dipasang: %ls.", nv);
        sim_recompute();
    }
}