#include "schematic.h"

/* ===================== Minimap =====================
   Panel 200×140 di pojok kanan-bawah canvas. Menampilkan overview seluruh
   schematic (komponen + kabel) + viewport merah. Klik/drag → pan canvas.
   Skala otomatis dari gabungan (bounds komponen ∪ viewport ∪ padding). */

#define MM_W        200
#define MM_H        140
#define MM_MARGIN   12
#define MM_PAD      6

typedef struct {
    int x0, y0, x1, y1;   /* world-space bounds of content */
    int sx, sy;           /* screen top-left of panel */
    double sc;            /* world -> minimap scale */
} MinimapGeo;

static int mm_drag_active = 0;

/* Hitung world bounds dari semua komponen + viewport saat ini + padding. */
static void mm_world_bounds(int canvas_w, int canvas_h,
                            int *x0, int *y0, int *x1, int *y1) {
    int minx, miny, maxx, maxy;

    if (comp_count == 0) {
        minx = g_pan_x;
        miny = g_pan_y;
        maxx = (int)(g_pan_x + canvas_w / g_zoom);
        maxy = (int)(g_pan_y + canvas_h / g_zoom);
    } else {
        minx =  1 << 30;  miny =  1 << 30;
        maxx = -1 << 30;  maxy = -1 << 30;
        for (int i = 0; i < comp_count; i++) {
            int hw, hh;
            comp_bounding_half(&comps[i], &hw, &hh);
            if (comps[i].x - hw < minx) minx = comps[i].x - hw;
            if (comps[i].y - hh < miny) miny = comps[i].y - hh;
            if (comps[i].x + hw > maxx) maxx = comps[i].x + hw;
            if (comps[i].y + hh > maxy) maxy = comps[i].y + hh;
        }
        int vx0 = g_pan_x;
        int vy0 = g_pan_y;
        int vx1 = (int)(g_pan_x + canvas_w / g_zoom);
        int vy1 = (int)(g_pan_y + canvas_h / g_zoom);
        if (vx0 < minx) minx = vx0;
        if (vy0 < miny) miny = vy0;
        if (vx1 > maxx) maxx = vx1;
        if (vy1 > maxy) maxy = vy1;
    }

    minx -= 40; miny -= 40;
    maxx += 40; maxy += 40;
    *x0 = minx; *y0 = miny; *x1 = maxx; *y1 = maxy;
}

static void mm_geo(int canvas_w, int canvas_h, MinimapGeo *g) {
    g->sx = canvas_w - MM_W - MM_MARGIN;
    g->sy = canvas_h - MM_H - MM_MARGIN;
    if (g->sx < 4) g->sx = 4;
    if (g->sy < 4) g->sy = 4;

    mm_world_bounds(canvas_w, canvas_h, &g->x0, &g->y0, &g->x1, &g->y1);

    int bw = g->x1 - g->x0; if (bw < 1) bw = 1;
    int bh = g->y1 - g->y0; if (bh < 1) bh = 1;
    double scx = (double)(MM_W - 2 * MM_PAD) / bw;
    double scy = (double)(MM_H - 2 * MM_PAD) / bh;
    g->sc = (scx < scy) ? scx : scy;
}

static void mm_w2s(MinimapGeo *g, int wx, int wy, int *sx, int *sy) {
    int bw = g->x1 - g->x0, bh = g->y1 - g->y0;
    int draw_w = (int)(bw * g->sc);
    int draw_h = (int)(bh * g->sc);
    int off_x = (MM_W - draw_w) / 2;
    int off_y = (MM_H - draw_h) / 2;
    *sx = g->sx + off_x + (int)((wx - g->x0) * g->sc);
    *sy = g->sy + off_y + (int)((wy - g->y0) * g->sc);
}

static void mm_s2w(MinimapGeo *g, int sx, int sy, int *wx, int *wy) {
    int bw = g->x1 - g->x0, bh = g->y1 - g->y0;
    int draw_w = (int)(bw * g->sc);
    int draw_h = (int)(bh * g->sc);
    int off_x = (MM_W - draw_w) / 2;
    int off_y = (MM_H - draw_h) / 2;
    *wx = g->x0 + (int)((sx - g->sx - off_x) / g->sc);
    *wy = g->y0 + (int)((sy - g->sy - off_y) / g->sc);
}

void minimap_render(HDC hdc, int canvas_w, int canvas_h) {
    if (!g_show_minimap) return;

    /* Auto-hide bila canvas kosong & view default (tidak ada yang perlu dinavigasi). */
    if (comp_count == 0 && g_zoom == 1.0 && g_pan_x == 0 && g_pan_y == 0) return;

    MinimapGeo g;
    mm_geo(canvas_w, canvas_h, &g);

    /* Panel background. */
    RECT panel = { g.sx, g.sy, g.sx + MM_W, g.sy + MM_H };
    HBRUSH bg = CreateSolidBrush(RGB(245, 248, 252));
    FillRect(hdc, &panel, bg);
    DeleteObject(bg);

    /* Border. */
    HPEN border = CreatePen(PS_SOLID, 1, RGB(180, 195, 215));
    HPEN ob = (HPEN)SelectObject(hdc, border);
    HBRUSH obr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, g.sx, g.sy, g.sx + MM_W, g.sy + MM_H);
    SelectObject(hdc, obr);

    int saved = SaveDC(hdc);
    IntersectClipRect(hdc, g.sx + 1, g.sy + 1, g.sx + MM_W - 1, g.sy + MM_H - 1);

    /* World content frame (faint). */
    int wx0, wy0, wx1, wy1;
    mm_w2s(&g, g.x0, g.y0, &wx0, &wy0);
    mm_w2s(&g, g.x1, g.y1, &wx1, &wy1);
    HPEN frame = CreatePen(PS_SOLID, 1, RGB(220, 225, 235));
    SelectObject(hdc, frame);
    Rectangle(hdc, wx0, wy0, wx1, wy1);
    SelectObject(hdc, border);

    /* Wires: thin gray lines. */
    HPEN wpen = CreatePen(PS_SOLID, 1, RGB(120, 120, 140));
    SelectObject(hdc, wpen);
    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        Component *cb = find_comp_by_id(wires[i].b.comp_id);
        if (!ca || !cb) continue;
        int ax, ay, bx, by;
        comp_pin_pos(ca, wires[i].a.pin_idx, &ax, &ay);
        comp_pin_pos(cb, wires[i].b.pin_idx, &bx, &by);
        int sax, say, sbx, sby;
        mm_w2s(&g, ax, ay, &sax, &say);
        mm_w2s(&g, bx, by, &sbx, &sby);
        MoveToEx(hdc, sax, say, NULL);
        LineTo(hdc, sbx, sby);
    }
    DeleteObject(wpen);

    /* Components: small filled rectangles. */
    HBRUSH comp_br = CreateSolidBrush(RGB(60, 100, 180));
    HBRUSH oldbr = (HBRUSH)SelectObject(hdc, comp_br);
    HPEN comp_pen = CreatePen(PS_SOLID, 1, RGB(40, 70, 130));
    HPEN oldpen = (HPEN)SelectObject(hdc, comp_pen);
    for (int i = 0; i < comp_count; i++) {
        int hw, hh;
        comp_bounding_half(&comps[i], &hw, &hh);
        int x0, y0, x1, y1;
        mm_w2s(&g, comps[i].x - hw, comps[i].y - hh, &x0, &y0);
        mm_w2s(&g, comps[i].x + hw, comps[i].y + hh, &x1, &y1);
        if (x1 - x0 < 3) x1 = x0 + 3;
        if (y1 - y0 < 3) y1 = y0 + 3;
        Rectangle(hdc, x0, y0, x1, y1);
    }
    SelectObject(hdc, oldpen);
    SelectObject(hdc, oldbr);
    DeleteObject(comp_pen);
    DeleteObject(comp_br);

    /* Viewport rectangle (highlight). */
    int vx0 = g_pan_x;
    int vy0 = g_pan_y;
    int vx1 = (int)(g_pan_x + canvas_w / g_zoom);
    int vy1 = (int)(g_pan_y + canvas_h / g_zoom);
    int svx0, svy0, svx1, svy1;
    mm_w2s(&g, vx0, vy0, &svx0, &svy0);
    mm_w2s(&g, vx1, vy1, &svx1, &svy1);
    if (svx1 - svx0 < 3) svx1 = svx0 + 3;
    if (svy1 - svy0 < 3) svy1 = svy0 + 3;

    HPEN vpen = CreatePen(PS_SOLID, 2, RGB(230, 60, 60));
    HPEN ovp = (HPEN)SelectObject(hdc, vpen);
    HBRUSH ovb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, svx0, svy0, svx1, svy1);
    SelectObject(hdc, ovb);
    SelectObject(hdc, ovp);
    DeleteObject(vpen);

    RestoreDC(hdc, saved);
    SelectObject(hdc, ob);
    DeleteObject(frame);
    DeleteObject(border);
}

int minimap_hit(int x, int y, int canvas_w, int canvas_h) {
    if (!g_show_minimap) return 0;
    if (comp_count == 0 && g_zoom == 1.0 && g_pan_x == 0 && g_pan_y == 0) return 0;
    MinimapGeo g;
    mm_geo(canvas_w, canvas_h, &g);
    return (x >= g.sx && x < g.sx + MM_W && y >= g.sy && y < g.sy + MM_H);
}

static void mm_center_on_world(int wx, int wy, int canvas_w, int canvas_h) {
    g_pan_x = wx - (int)(canvas_w / 2.0 / g_zoom);
    g_pan_y = wy - (int)(canvas_h / 2.0 / g_zoom);
}

void minimap_click(int x, int y, int canvas_w, int canvas_h) {
    MinimapGeo g;
    mm_geo(canvas_w, canvas_h, &g);
    int wx, wy;
    mm_s2w(&g, x, y, &wx, &wy);
    mm_center_on_world(wx, wy, canvas_w, canvas_h);
}

void minimap_begin_drag(int x, int y, int canvas_w, int canvas_h) {
    (void)x; (void)y; (void)canvas_w; (void)canvas_h;
    mm_drag_active = 1;
}

void minimap_drag(int x, int y, int canvas_w, int canvas_h) {
    if (!mm_drag_active) return;
    minimap_click(x, y, canvas_w, canvas_h);
}

void minimap_end_drag(void)  { mm_drag_active = 0; }
int  minimap_dragging(void)  { return mm_drag_active; }