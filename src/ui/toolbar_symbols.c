#include "toolbar_intern.h"

/* ===================== Ikon mini toolbar (dipindah utuh dari toolbar.c) ====
   sym_* menggambar versi kecil simbol komponen untuk tombol toolbar.
   toolbar_draw_symbol = dispatcher + ikon Select/Wire/Delete. */

static CompType toolbar_tool_comp(ToolKind t) {
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
        default: return COMP_NONE;
    }
}

static void sym_resistor(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -20, 0);
    MoveToEx(hdc, 20, 0, NULL); LineTo(hdc, 40, 0);
    Rectangle(hdc, -20, -10, 20, 10);
}

static void sym_switch(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -15, 0);
    MoveToEx(hdc, 15, 0, NULL); LineTo(hdc, 40, 0);
    Ellipse(hdc, -18, -3, -12, 3);
    Ellipse(hdc, 12, -3, 18, 3);
    MoveToEx(hdc, -15, 0, NULL); LineTo(hdc, 12, -15);
}

static void sym_battery(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -10, 0);
    MoveToEx(hdc, 10, 0, NULL); LineTo(hdc, 40, 0);
    MoveToEx(hdc, -10, -15, NULL); LineTo(hdc, -10, 15);
    MoveToEx(hdc, 10, -8, NULL); LineTo(hdc, 10, 8);
}

static void sym_acsource(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -22, 0);
    MoveToEx(hdc, 22, 0, NULL); LineTo(hdc, 40, 0);
    Ellipse(hdc, -22, -22, 22, 22);
    /* sinus mini */
    {
        int started = 0;
        for (int px = -14; px <= 14; px++) {
            double ph = (double)px / 14.0 * 3.14159265358979;
            int py = (int)(-10.0 * sin(ph));
            if (!started) { MoveToEx(hdc, px, py, NULL); started = 1; }
            else LineTo(hdc, px, py);
        }
    }
}

static void sym_led(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -12, 0);
    MoveToEx(hdc, 12, 0, NULL); LineTo(hdc, 40, 0);
    POINT tri[3] = { {-12,-15}, {-12,15}, {12,0} };
    Polygon(hdc, tri, 3);
    MoveToEx(hdc, 12, -15, NULL); LineTo(hdc, 12, 15);
    MoveToEx(hdc, -5, -18, NULL); LineTo(hdc, -12, -30);
    MoveToEx(hdc, 3, -18, NULL); LineTo(hdc, -4, -30);
}

static void sym_ground(HDC hdc) {
    MoveToEx(hdc, 0, -30, NULL); LineTo(hdc, 0, 0);
    MoveToEx(hdc, -20, 0, NULL); LineTo(hdc, 20, 0);
    MoveToEx(hdc, -12, 8, NULL); LineTo(hdc, 12, 8);
    MoveToEx(hdc, -5, 16, NULL); LineTo(hdc, 5, 16);
}

static void sym_capacitor(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -6, 0);
    MoveToEx(hdc, 6, 0, NULL); LineTo(hdc, 40, 0);
    MoveToEx(hdc, -6, -15, NULL); LineTo(hdc, -6, 15);
    MoveToEx(hdc, 6, -15, NULL); LineTo(hdc, 6, 15);
}

static void sym_diode(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -12, 0);
    MoveToEx(hdc, 12, 0, NULL); LineTo(hdc, 40, 0);
    POINT tri[3] = { {-12,-15}, {-12,15}, {12,0} };
    Polygon(hdc, tri, 3);
    MoveToEx(hdc, 12, -15, NULL); LineTo(hdc, 12, 15);
}

static void sym_capolar(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -6, 0);
    MoveToEx(hdc, 6, 0, NULL); LineTo(hdc, 40, 0);
    MoveToEx(hdc, -6, -15, NULL); LineTo(hdc, -6, 15);   /* plat (+) lurus */
    MoveToEx(hdc, 6, -15, NULL);                          /* plat (-) lengkung */
    Arc(hdc, -24, -30, 42, 30, 6, 15, 6, -15);
}

static void sym_transistor(HDC hdc) {
    /* Basis: kiri -> bar */
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -8, 0);
    /* Bar vertikal */
    MoveToEx(hdc, -8, -14, NULL); LineTo(hdc, -8, 14);
    /* Kolektor (ke kanan-atas) */
    MoveToEx(hdc, -8, -6, NULL); LineTo(hdc, 40, -28);
    /* Emitor (ke kanan-bawah) + panah NPN */
    MoveToEx(hdc, -8, 6, NULL); LineTo(hdc, 40, 28);

    /* Panah di jalur emitor. Karena toolbar pakai pen dari caller,
       panah digambar sebagai garis (bukan Polygon) agar warnanya ikut pen. */
    {
        const double ex = 48.0, ey = 22.0;
        double len = sqrt(ex * ex + ey * ey);
        double ux = ex / len, uy = ey / len;
        double nx = -uy, ny = ux;

        double t  = 0.6;
        double hx = -8 + ex * t;
        double hy =  6 + ey * t;
        const double HAL = 8.0, HAW = 3.2;

        POINT a0 = {
            (LONG)lround(hx - ux * HAL + nx * HAW),
            (LONG)lround(hy - uy * HAL + ny * HAW)
        };
        POINT a1 = {
            (LONG)lround(hx - ux * HAL - nx * HAW),
            (LONG)lround(hy - uy * HAL - ny * HAW)
        };
        POINT tip = { (LONG)lround(hx), (LONG)lround(hy) };

        MoveToEx(hdc, a0.x, a0.y, NULL);
        LineTo  (hdc, tip.x, tip.y);
        LineTo  (hdc, a1.x, a1.y);
    }

    /* Lingkaran badan (digambar terakhir = paling atas) */
    Ellipse(hdc, -20, -20, 20, 20);
}

static void sym_ldr(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -22, 0);
    MoveToEx(hdc, 22, 0, NULL); LineTo(hdc, 40, 0);
    Rectangle(hdc, -22, -12, 22, 12);          /* badan fotoresistor */
    MoveToEx(hdc, -22, -6, NULL); LineTo(hdc, 22, -6);
    MoveToEx(hdc, -22, 6, NULL);  LineTo(hdc, 22, 6);
    /* dua panah cahaya masuk */
    MoveToEx(hdc, -30, -30, NULL); LineTo(hdc, -14, -16);
    MoveToEx(hdc, -18, -34, NULL); LineTo(hdc, -2, -20);
}

static void sym_buzzer(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -14, 0);
    MoveToEx(hdc, 14, 0, NULL); LineTo(hdc, 40, 0);
    Arc(hdc, -14, -20, 26, 20, -14, -20, -14, 20);   /* dome */
    MoveToEx(hdc, -14, -20, NULL); LineTo(hdc, -14, 20);
    MoveToEx(hdc, 20, -8, NULL); LineTo(hdc, 26, 0); LineTo(hdc, 20, 8); /* bunyi > */
    MoveToEx(hdc, 28, -12, NULL); LineTo(hdc, 36, 0); LineTo(hdc, 28, 12);
}

static void sym_voltmeter(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -20, 0);
    MoveToEx(hdc, 20, 0, NULL); LineTo(hdc, 40, 0);
    Ellipse(hdc, -20, -20, 20, 20);
    /* huruf V di tengah lingkaran */
    MoveToEx(hdc, -8, -9, NULL); LineTo(hdc, 0, 9); LineTo(hdc, 8, -9);
}

static void sym_ammeter(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -20, 0);
    MoveToEx(hdc, 20, 0, NULL); LineTo(hdc, 40, 0);
    Ellipse(hdc, -20, -20, 20, 20);
    /* huruf "A" di tengah lingkaran: dua diagonal + crossbar horizontal.
       (Versi lama menggambar 3 garis siku yang justru tampak seperti ">".) */
    MoveToEx(hdc, -8, 9, NULL); LineTo(hdc, 0, -9);      /* kaki kiri    */
    MoveToEx(hdc,  0, -9, NULL); LineTo(hdc, 8, 9);      /* kaki kanan   */
    MoveToEx(hdc, -4,  2, NULL); LineTo(hdc, 4, 2);      /* crossbar     */
}

static void sym_oscprobe(HDC hdc) {
    MoveToEx(hdc, -40, 0, NULL); LineTo(hdc, -20, 0);
    MoveToEx(hdc, 20, 0, NULL); LineTo(hdc, 40, 0);
    POINT tip[3] = { { -20, 0 }, { -8, -10 }, { -8, 10 } };
    Polygon(hdc, tip, 3);
    Ellipse(hdc, -8, -10, 20, 10);
    HBRUSH br = CreateSolidBrush(RGB(20, 20, 20));
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    Ellipse(hdc, 15, -4, 23, 4);
    SelectObject(hdc, ob);
    DeleteObject(br);
}

void toolbar_draw_symbol(HDC hdc, ToolKind tool, int cx, int cy, BOOL active) {
    int saved = SaveDC(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xf;
    xf.eM11 = 0.42f; xf.eM12 = 0;
    xf.eM21 = 0;     xf.eM22 = 0.42f;
    xf.eDx = (FLOAT)cx;
    xf.eDy = (FLOAT)(cy - 2);
    SetWorldTransform(hdc, &xf);
    HPEN pen = CreatePen(PS_SOLID, 2, active ? RGB(50,128,255) : RGB(20,20,20));
    HPEN op = (HPEN)SelectObject(hdc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    CompType ct = toolbar_tool_comp(tool);
    if (ct == COMP_RESISTOR) sym_resistor(hdc);
    else if (ct == COMP_SWITCH) sym_switch(hdc);
    else if (ct == COMP_BATTERY) sym_battery(hdc);
    else if (ct == COMP_ACSOURCE) sym_acsource(hdc);
    else if (ct == COMP_LED) sym_led(hdc);
    else if (ct == COMP_GROUND) sym_ground(hdc);
    else if (ct == COMP_CAPACITOR) sym_capacitor(hdc);
    else if (ct == COMP_TRANSISTOR) sym_transistor(hdc);
    else if (ct == COMP_DIODE) sym_diode(hdc);
    else if (ct == COMP_CAPOLAR) sym_capolar(hdc);
    else if (ct == COMP_LDR) sym_ldr(hdc);
    else if (ct == COMP_BUZZER) sym_buzzer(hdc);
    else if (ct == COMP_VOLTMETER) sym_voltmeter(hdc);
    else if (ct == COMP_AMMETER) sym_ammeter(hdc);
    else if (ct == COMP_OSCPROBE) sym_oscprobe(hdc);
    else if (tool == TOOL_SELECT) {
        POINT cur[7] = { {0,-14},{10,4},{4,4},{4,14},{-2,14},{-2,4},{-8,4} };
        Polygon(hdc, cur, 7);
    } else if (tool == TOOL_WIRE) {
        MoveToEx(hdc, -22, -10, NULL); LineTo(hdc, 0, -10); LineTo(hdc, 0, 12);
        MoveToEx(hdc, 0, 12, NULL); LineTo(hdc, 22, 12);
        Ellipse(hdc, -27, -15, -17, -5);
        Ellipse(hdc, 17, 7, 27, 17);
    } else if (tool == TOOL_BOX) {
        Rectangle(hdc, -30, -26, 30, 26);
        Rectangle(hdc, -22, -12, -6, 2);            /* resistor mini */
        MoveToEx(hdc, -26, -5, NULL); LineTo(hdc, -22, -5);
        MoveToEx(hdc, -6, -5, NULL); LineTo(hdc, 2, -5);
        POINT tri[3] = { {6,-8}, {6,0}, {14,-4} };  /* dioda/LED mini */
        Polygon(hdc, tri, 3);
    } else if (tool == TOOL_DELETE) {
        MoveToEx(hdc, -16, -12, NULL); LineTo(hdc, 16, -12);
        MoveToEx(hdc, -8, -12, NULL); LineTo(hdc,-5,-18); LineTo(hdc,+5,-18); LineTo(hdc,+8,-12);
        Rectangle(hdc,-13,-12,+13,+16);
        MoveToEx(hdc,-5,-6,NULL); LineTo(hdc,-5,+10);
        MoveToEx(hdc,+0,-6,NULL); LineTo(hdc,+0,+10);
        MoveToEx(hdc,+5,-6,NULL); LineTo(hdc,+5,+10);
    }
    else if (tool == TOOL_SELECT) {
        POINT cur[7] = { {0,-14},{10,4},{4,4},{4,14},{-2,14},{-2,4},{-8,4} };
        HBRUSH br = CreateSolidBrush(active ? RGB(50,128,255) : RGB(40,40,40));
        HBRUSH ob2 = (HBRUSH)SelectObject(hdc, br);
        HPEN pn2 = CreatePen(PS_SOLID, 1, active ? RGB(50,128,255) : RGB(40,40,40));
        HPEN op2 = (HPEN)SelectObject(hdc, pn2);
        Polygon(hdc, cur, 7);
        SelectObject(hdc, op2);
        SelectObject(hdc, ob2);
        DeleteObject(pn2);
        DeleteObject(br);
    } else if (tool == TOOL_WIRE) {
        HPEN wp = CreatePen(PS_SOLID, 3, active ? RGB(50,128,255) : RGB(20,20,20));
        HPEN owp = (HPEN)SelectObject(hdc, wp);
        MoveToEx(hdc, -22, -10, NULL); LineTo(hdc, 0, -10); LineTo(hdc, 0, 12);
        MoveToEx(hdc, 0, 12, NULL); LineTo(hdc, 22, 12);
        SelectObject(hdc, owp);
        DeleteObject(wp);
        HBRUSH br = CreateSolidBrush(RGB(220,60,60));
        HBRUSH ob3 = (HBRUSH)SelectObject(hdc, br);
        Ellipse(hdc, -27, -15, -17, -5);
        Ellipse(hdc, 17, 7, 27, 17);
        SelectObject(hdc, ob3);
        DeleteObject(br);
    } else if (tool == TOOL_BOX) {
        /* ikon kotak putus-putus + 2 komponen mini di dalamnya */
        INT oldmode = SetBkMode(hdc, TRANSPARENT);
        HPEN bp2 = CreatePen(PS_DASH, 2, active ? RGB(50,128,255) : RGB(40,40,40));
        HPEN obp2 = (HPEN)SelectObject(hdc, bp2);
        Rectangle(hdc, -30, -26, 30, 26);
        SelectObject(hdc, obp2);
        DeleteObject(bp2);
        HPEN sp = CreatePen(PS_SOLID, 2, active ? RGB(50,128,255) : RGB(90,90,90));
        HPEN osp2 = (HPEN)SelectObject(hdc, sp);
        Rectangle(hdc, -22, -12, -6, 2);            /* resistor mini */
        MoveToEx(hdc, -26, -5, NULL); LineTo(hdc, -22, -5);
        MoveToEx(hdc, -6, -5, NULL); LineTo(hdc, 2, -5);
        POINT tri[3] = { {6,-8}, {6,0}, {14,-4} };  /* dioda/LED mini */
        HBRUSH brf = CreateSolidBrush(active ? RGB(50,128,255) : RGB(90,90,90));
        HBRUSH obrf = (HBRUSH)SelectObject(hdc, brf);
        Polygon(hdc, tri, 3);
        SelectObject(hdc, obrf);
        DeleteObject(brf);
        SelectObject(hdc, osp2);
        DeleteObject(sp);
        SetBkMode(hdc, oldmode);
    } else if (tool == TOOL_DELETE) {
        HPEN dp = CreatePen(PS_SOLID, 3, RGB(200,40,40));
        HPEN odp = (HPEN)SelectObject(hdc, dp);
        MoveToEx(hdc, -16, -12, NULL); LineTo(hdc, 16, -12);
        MoveToEx(hdc, -8, -12, NULL); LineTo(hdc, -5, -18);
        LineTo(hdc, 5, -18); LineTo(hdc, 8, -12);
        Rectangle(hdc, -13, -12, 13, 16);
        MoveToEx(hdc, -5, -6, NULL); LineTo(hdc, -5, 10);
        MoveToEx(hdc, 0, -6, NULL); LineTo(hdc, 0, 10);
        MoveToEx(hdc, 5, -6, NULL); LineTo(hdc, 5, 10);
        SelectObject(hdc, odp);
        DeleteObject(dp);
    }
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(pen);
    RestoreDC(hdc, saved);
}

const wchar_t *toolbar_tool_shortname(ToolKind tool) {
    if (tool == TOOL_RESISTOR) return L"R";
    if (tool == TOOL_SWITCH) return L"SW";
    if (tool == TOOL_BATTERY) return L"BAT";
    if (tool == TOOL_ACSOURCE) return L"AC";
    if (tool == TOOL_LED) return L"LED";
    if (tool == TOOL_CAPACITOR) return L"C";
    if (tool == TOOL_TRANSISTOR) return L"Q";
    if (tool == TOOL_DIODE) return L"D";
    if (tool == TOOL_CAPOLAR) return L"CP";
    if (tool == TOOL_LDR) return L"LDR";
    if (tool == TOOL_BUZZER) return L"BZR";
    if (tool == TOOL_VOLTMETER) return L"VM";
    if (tool == TOOL_GROUND) return L"GND";
    if (tool == TOOL_SELECT) return L"Select";
    if (tool == TOOL_BOX) return L"Blok";
    if (tool == TOOL_WIRE) return L"Wire";
    if (tool == TOOL_AMMETER) return L"AM";
    if (tool == TOOL_OSCPROBE) return L"OSC";
    return L"Hapus";
}

int toolbar_tool_is_component(ToolKind tool) {
    switch (tool) {
        case TOOL_RESISTOR:
        case TOOL_SWITCH:
        case TOOL_BATTERY:
        case TOOL_ACSOURCE:
        case TOOL_LED:
        case TOOL_GROUND:
        case TOOL_CAPACITOR:
        case TOOL_TRANSISTOR:
        case TOOL_DIODE:
        case TOOL_CAPOLAR:
        case TOOL_LDR:
        case TOOL_BUZZER:
        case TOOL_VOLTMETER:
        case TOOL_AMMETER:
        case TOOL_OSCPROBE:
            return 1;
        default:
            return 0;
    }
}
