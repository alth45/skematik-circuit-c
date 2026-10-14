#include "schematic.h"

static CompType tool_to_comp(ToolKind t) {
    switch (t) {
        case TOOL_RESISTOR: return COMP_RESISTOR;
        case TOOL_SWITCH:   return COMP_SWITCH;
        case TOOL_BATTERY:  return COMP_BATTERY;
        case TOOL_LED:      return COMP_LED;
        case TOOL_GROUND:   return COMP_GROUND;
        default: return COMP_NONE;
    }
}

/* ===================== Event Handler ===================== */
void on_lbutton_down(HWND hwnd, int x, int y) {
    /* MODE RUN: klik saklar = toggle ON/OFF, klik lain = select saja */
    if (sim_running) {
        Component *hit = find_comp_at(x, y);
        if (hit && hit->type == COMP_SWITCH) {
            hit->on = !hit->on;
            selected_id = hit->id;
            sim_recompute();
            set_status(L"RUN: saklar %ls. %ls",
                hit->on ? L"ON (tertutup)" : L"OFF (terbuka)", sim_message());
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
    model_remove_at(x, y);
    sim_recompute();
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
    else if (key == '4') { current_tool = TOOL_LED; }
    else if (key == '5') { current_tool = TOOL_GROUND; }
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

