#ifndef _VK_GRID_H_
#define _VK_GRID_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"

/*
    vk_grid_t -- a 2D layout container, sister to vk_box_t.

    Cells are addressed as (col, row), col-major in struct fields.
    Two modes coexist on the same grid:

      slot mode  : a cell with vk_grid_set_widget(g, col, row, w) holds
                   widget w; the grid resizes / repositions / composites
                   w like vk_box does.
      paint mode : a cell with no slot widget (the default) is left for
                   the consumer to paint into.  Use vk_grid_get_cell_rect
                   to query its geometry on the grid's canvas.

    Homogeneous mode (default) gives every column the same width and
    every row the same height.  Non-homogeneous mode honors per-column
    widths and per-row heights set by the consumer, distributing
    leftover space to columns/rows tagged with vk_grid_set_col_expand /
    vk_grid_set_row_expand.
*/

struct _vk_grid_s
{
    vk_container_t      parent_klass;

    int                 cols;
    int                 rows;

    /* slot_widgets[row * cols + col] -- NULL = paint-mode cell */
    vk_widget_t         **slot_widgets;

    /* per-column / per-row natural sizing (non-homogeneous mode) */
    int                 *col_widths;        /* size = cols */
    int                 *row_heights;       /* size = rows */
    bool                *col_expand;        /* size = cols */
    bool                *row_expand;        /* size = rows */

    int                 focused_col;
    int                 focused_row;
    bool                homogeneous;

    /* uniform gap reserved between cells AND around the outer edge.
       gap=0 means cells touch and fill the canvas (default).  gap=1
       leaves a 1-char ring around the grid and 1-char gaps between
       cells -- vk_table draws dividers into that space. */
    int                 gap;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_grid_t *);
};

/* internal: vk_grid's default layout routine, exposed so subclasses
   (vk_table, etc.) can chain through it before overlaying decorations */
int     _vk_grid_update(vk_grid_t *grid);

/* internal: writes per-column and per-row sizes into the provided
   arrays (length grid->cols / grid->rows).  used by subclasses that
   need to query the same layout the grid renders. */
void    _vk_grid_compute_track_sizes(vk_grid_t *grid,
            int *col_sizes, int *row_sizes);

#endif
