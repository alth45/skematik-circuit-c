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

/* Tombol toolbar (digambar manual di atas, bukan HWND).
   Urutan: Select, Wire, Resistor, Switch, Battery, LED, Ground,
   Capacitor, Transistor, Delete. + 4 tombol aksi. */
const ToolKind btnTools[BTN_COUNT] = {
    TOOL_SELECT, TOOL_WIRE,
    TOOL_RESISTOR, TOOL_SWITCH, TOOL_BATTERY, TOOL_LED, TOOL_GROUND,
    TOOL_CAPACITOR, TOOL_TRANSISTOR,
    TOOL_DELETE
};
const wchar_t *btnLabels[BTN_COUNT] = {
    L"Select / Geser (V)", L"Kabel / Wire (W)",
    L"Resistor (1)", L"Saklar / Switch (2)", L"Baterai (3)", L"LED (4)", L"Ground (5)",
    L"Kapasitor (6)", L"Transistor NPN (7)",
    L"Hapus / Delete (Del)"
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
        int hw = (comps[i].rot % 2 == 0) ? 48 : 38;
        int hh = (comps[i].rot % 2 == 0) ? 38 : 48;
        int dx = abs(comps[i].x - x);
        int dy = abs(comps[i].y - y);
        if (dx < hw && dy < hh) return &comps[i];
    }
    return NULL;
}

int comp_pin_count(const Component *c) {
    if (!c) return 0;
    if (c->type == COMP_GROUND) return 1;
    if (c->type == COMP_TRANSISTOR) return 3; /* C, E, B */
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
    /* rotasi searah jarum jam, koordinat layar (y ke bawah):
       r=1 (90 CW):  (x,y) -> (-y, x)
       r=2 (180):    (x,y) -> (-x,-y)
       r=3 (270 CW): (x,y) -> (y, -x) */
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
    } else if (c->type == COMP_TRANSISTOR) {
        /* NPN: pin0 = Kolektor (kiri), pin1 = Emitor (kanan),
           pin2 = Basis (bawah, segaris bar basis x=-4). */
        if (pin_idx == 2)      comp_local_to_world(c, -4, 30, px, py);
        else if (pin_idx == 1) comp_local_to_world(c, 40, 13, px, py);
        else                   comp_local_to_world(c, -40, 0, px, py);
    } else {
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

/* ===================== Operasi Model ===================== */
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
}

void model_clear_all(void) {
    comp_count = 0;
    wire_count = 0;
    selected_id = -1;
    dragging_id = -1;
    wiring_active = 0;
    for (int i = 0; i < MAX_COMP; i++) comps[i].on = 0;
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


