#ifndef _VK_TABLE_H_
#define _VK_TABLE_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_grid.h"

/*
    vk_table_t -- vk_grid_t with rendered dividers between cells and
    around the outer edge.  Inherits all of vk_grid's layout (per-row
    and per-col sizing, slot/paint modes, expand bits) and reuses its
    gap as the divider thickness (1 char for single/double/ASCII).

    Divider colors default to the widget's own fg/bg pair; override
    them with vk_table_set_border_colors.

    The divider style follows the existing border-style flags:
        VK_BORDER_NONE      -- no dividers; sets gap=0
        VK_BORDER_SINGLE    -- single line WACS box drawing
        VK_BORDER_DOUBLE    -- double line WACS box drawing
        VK_BORDER_ASCII     -- '-', '|', '+'
*/

struct _vk_table_s
{
    vk_grid_t           parent_klass;

    int                 divider_style;
    short               border_fg;          /* -1 = use widget->fg */
    short               border_bg;          /* -1 = use widget->bg */
    attr_t              border_attrs;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

/* internal helpers shared with subclasses (e.g. vk_color uses these
   to overdraw the focused cell's border in a highlight color, and to
   chain through vk_table's full layout+divider pass) */
int             _vk_table_update(vk_grid_t *grid);
const cchar_t*  _vk_table_pick_junction(int divider_style,
                    int r, int rows, int c, int cols);
void            _vk_table_compute_lines(vk_table_t *table,
                    int *col_sizes, int *row_sizes,
                    int *vlines,    int *hlines);

#endif
