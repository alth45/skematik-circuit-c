#ifndef TOOLBAR_INTERN_H
#define TOOLBAR_INTERN_H

#include "schematic.h"

/* Header internal khusus keluarga toolbar (src/ui/toolbar_*).
   Tidak dipakai modul lain — API publik tetap di schematic.h. */

/* toolbar_symbols.c */
void toolbar_draw_symbol(HDC hdc, ToolKind tool, int cx, int cy, BOOL active);
const wchar_t *toolbar_tool_shortname(ToolKind tool);
int toolbar_tool_is_component(ToolKind tool);

/* toolbar_actions.c */
void palette_draw_clear(HDC hdc, int bx, int by);
void palette_draw_save(HDC hdc, int bx, int by);
void palette_draw_open(HDC hdc, int bx, int by);
void palette_draw_run(HDC hdc, int bx, int by);

/* toolbar_palette.c */
int toolbar_btn_x(int idx);

#endif /* TOOLBAR_INTERN_H */
