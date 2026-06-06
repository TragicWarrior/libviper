#ifndef _VK_COLOR_H_
#define _VK_COLOR_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_grid.h"
#include "vk_table.h"

/*
    vk_color_t -- a 16-cell color-picker derived from vk_table.

    cells are paint-mode (no slot widgets) and arranged in any layout
    whose cols * rows == 16 (1x16, 2x8, 4x4, 8x2, 16x1).  cell colors
    are the standard 16 ANSI colors (indices 0..15).

    arrow keys move the selection within bounds; Tab / Shift-Tab cycle
    through all 16 in row-major order; Enter emits ON_ACTIVATE.

    the focused cell's surrounding divider segments are re-drawn in
    a configurable highlight color (set via vk_color_set_focus_colors)
    so the selection is visible without disrupting the cells' own
    color blocks.
*/

struct _vk_color_s
{
    vk_table_t          parent_klass;

    short               selected;       /* 0..15 */

    short               focus_fg;
    short               focus_bg;
    attr_t              focus_attrs;

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);
};

#endif
