#include "schematic.h"

int sim_running = 0;
int sim_led_count = 0;
int sim_short = 0;
int sim_has_battery = 0;

/* Net listrik: tiap pin komponen = 1 node.
   node = comp_index * 2 + pin (ground pin1 tak dipakai). */
static int parent_[MAX_COMP * 2];
static int lit_[MAX_COMP];
static int hot_[MAX_WIRES];

static int find_root(int a) {
    while (parent_[a] != a) {
        parent_[a] = parent_[parent_[a]];
        a = parent_[a];
    }
    return a;
}

static void unite(int a, int b) {
    int ra = find_root(a), rb = find_root(b);
    if (ra != rb) parent_[ra] = rb;
}


static int node_of(int comp_idx, int pin) {
    return comp_idx * 2 + pin;
}

void sim_recompute(void) {
    for (int i = 0; i < MAX_COMP * 2; i++) parent_[i] = i;
    for (int i = 0; i < MAX_COMP; i++) lit_[i] = 0;
    for (int i = 0; i < MAX_WIRES; i++) hot_[i] = 0;
    sim_led_count = 0;
    sim_short = 0;
    sim_has_battery = 0;

    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        Component *cb = find_comp_by_id(wires[i].b.comp_id);
        if (!ca || !cb) continue;
        int ia = (int)(ca - comps), ib = (int)(cb - comps);
        if (ia < 0 || ia >= comp_count || ib < 0 || ib >= comp_count) continue;
        unite(node_of(ia, wires[i].a.pin_idx), node_of(ib, wires[i].b.pin_idx));
    }

    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type == COMP_RESISTOR) {
            unite(node_of(i, 0), node_of(i, 1));
        } else if (comps[i].type == COMP_SWITCH && comps[i].on) {
            unite(node_of(i, 0), node_of(i, 1));
        }
        if (comps[i].type == COMP_BATTERY) sim_has_battery = 1;
    }

    int plus_root[MAX_COMP * 2], minus_root[MAX_COMP * 2];
    int n_plus = 0, n_minus = 0;
    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type == COMP_BATTERY) {
            plus_root[n_plus++] = find_root(node_of(i, 0));
            minus_root[n_minus++] = find_root(node_of(i, 1));
        } else if (comps[i].type == COMP_GROUND) {
            minus_root[n_minus++] = find_root(node_of(i, 0));
        }
    }

    for (int a = 0; a < n_plus; a++)
        for (int b = 0; b < n_minus; b++)
            if (plus_root[a] == minus_root[b]) sim_short = 1;

    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type != COMP_LED) continue;
        int r0 = find_root(node_of(i, 0));
        int r1 = find_root(node_of(i, 1));
        int on_plus = 0, on_minus = 0;
        for (int k = 0; k < n_plus; k++) if (plus_root[k] == r0) on_plus = 1;
        for (int k = 0; k < n_minus; k++) if (minus_root[k] == r1) on_minus = 1;
        if (on_plus && on_minus) { lit_[i] = 1; sim_led_count++; }
    }

    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        if (!ca) continue;
        int ia = (int)(ca - comps);
        if (ia < 0 || ia >= comp_count) continue;
        int r = find_root(node_of(ia, wires[i].a.pin_idx));
        for (int k = 0; k < n_plus; k++)
            if (plus_root[k] == r) { hot_[i] = 1; break; }
    }
}

void sim_set_running(int on) {
    sim_running = on ? 1 : 0;
    dragging_id = -1;
    wiring_active = 0;
    sim_recompute();
    if (sim_running) {
        if (!sim_has_battery) set_status(L"RUN: pasang Baterai dulu, lalu sambungkan kabel.");
        else set_status(L"RUN: %ls", sim_message());
    } else {
        set_status(L"Mode Edit. Klik ikon di toolbar, lalu klik canvas.");
    }
}

int sim_wire_hot(int idx) {
    if (idx < 0 || idx >= wire_count) return 0;
    return hot_[idx];
}

int sim_comp_lit(const Component *c) {
    if (!c) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return lit_[i];
}

const wchar_t *sim_message(void) {
    if (!sim_has_battery) return L"Belum ada baterai — LED tidak bisa menyala.";
    if (sim_short) return L"KORSLETING! (+) tersambung langsung ke (-).";
    if (sim_led_count > 0) return L"Rangkaian BENAR — LED MENYALA.";
    return L"Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.";
}
