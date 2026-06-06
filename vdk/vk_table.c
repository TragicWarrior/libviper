#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_grid.h"
#include "vk_table.h"
#include "vk_event.h"

static int
_vk_table_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_table_dtor(vk_object_t *object);

/* exposed for subclasses: layout + dividers in one call, useful when
   a subclass wants the table render unchanged then paints something
   on top (e.g. vk_color) */
int
_vk_table_update(vk_grid_t *grid);

/* exposed for subclasses (vk_color overdraws the focused cell's
   borders with these same junctions in a highlight color) */
const cchar_t*
_vk_table_pick_junction(int divider_style,
    int r, int rows, int c, int cols);

void
_vk_table_compute_lines(vk_table_t *table,
    int *col_sizes, int *row_sizes, int *vlines, int *hlines);

require_klass(VK_GRID_KLASS);

declare_klass(VK_TABLE_KLASS)
{
    .size = KLASS_SIZE(vk_table_t),
    .name = KLASS_NAME(vk_table_t),
    .ctor = _vk_table_ctor,
    .dtor = _vk_table_dtor,
};

inline vk_table_t*
vk_table_create(int width, int height, int cols, int rows, int divider_style)
{
    vk_table_t  *table;

    if(width <= 0 || height <= 0) return NULL;
    if(cols < 1 || rows < 1) return NULL;

    table = (vk_table_t*)vk_object_create(VK_TABLE_KLASS,
        width, height, cols, rows, divider_style);

    return table;
}

inline int
vk_table_set_divider_style(vk_table_t *table, int divider_style)
{
    if(table == NULL) return -1;

    table->divider_style = divider_style;

    /* gap = 1 for any drawn divider style, 0 when no dividers */
    vk_grid_set_gap(VK_GRID(table),
        (divider_style == VK_BORDER_NONE) ? 0 : 1);

    return 0;
}

inline int
vk_table_get_divider_style(vk_table_t *table)
{
    if(table == NULL) return -1;
    return table->divider_style;
}

inline int
vk_table_set_border_colors(vk_table_t *table, short fg, short bg)
{
    if(table == NULL) return -1;

    table->border_fg = fg;
    table->border_bg = bg;

    return 0;
}

inline int
vk_table_set_border_attrs(vk_table_t *table, attr_t attrs)
{
    if(table == NULL) return -1;

    table->border_attrs = attrs;

    return 0;
}

inline int
vk_table_update(vk_table_t *table)
{
    if(table == NULL) return -1;
    return vk_grid_update(VK_GRID(table));
}

inline void
vk_table_destroy(vk_table_t *table)
{
    if(table == NULL) return;

    if(!vk_object_assert(table, vk_table_t)) return;

    table->dtor(VK_OBJECT(table));
}

static int
_vk_table_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_table_t  *table;
    vk_grid_t   *grid;
    va_list     args;
    int         divider_style;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* grid takes (width, height, cols, rows) -- we add divider_style */
    VK_GRID_KLASS->ctor(object, argp);

    divider_style = va_arg(*argp, int);

    va_end(args);

    table = VK_TABLE(object);
    grid = VK_GRID(object);

    table->divider_style = divider_style;
    table->border_fg     = -1;
    table->border_bg     = -1;
    table->border_attrs  = 0;

    table->ctor = _vk_table_ctor;
    table->dtor = _vk_table_dtor;

    /* reserve 1-char gap so the dividers have somewhere to live */
    vk_grid_set_gap(grid,
        (divider_style == VK_BORDER_NONE) ? 0 : 1);

    /* override the grid's update slot -- vk_grid_update will now route
       through _vk_table_update, which does layout then dividers */
    grid->_update = _vk_table_update;

    return 0;
}

static int
_vk_table_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;
    if(!vk_object_assert(object, vk_table_t)) return -1;

    vk_object_demote(object, vk_grid_t);
    vk_grid_destroy(VK_GRID(object));

    return 0;
}

/*
    pick the line-drawing wide char for the intersection at column
    column-index c (0..cols inclusive) and row-index r (0..rows
    inclusive) within the divider grid.  corner / tee / cross selected
    by which edges (top/bottom/left/right) reach the intersection.
*/
const cchar_t*
_vk_table_pick_junction(int divider_style,
    int r, int rows, int c, int cols)
{
    bool    top    = (r > 0);
    bool    bot    = (r < rows);
    bool    left   = (c > 0);
    bool    right  = (c < cols);

    if(divider_style == VK_BORDER_DOUBLE)
    {
        if(!top && !left)               return WACS_D_ULCORNER;
        if(!top && !right)              return WACS_D_URCORNER;
        if(!bot && !left)               return WACS_D_LLCORNER;
        if(!bot && !right)              return WACS_D_LRCORNER;
        if(!top)                        return WACS_D_TTEE;
        if(!bot)                        return WACS_D_BTEE;
        if(!left)                       return WACS_D_LTEE;
        if(!right)                      return WACS_D_RTEE;
        return WACS_D_PLUS;
    }

    /* SINGLE -- WACS variants */
    if(!top && !left)                   return WACS_ULCORNER;
    if(!top && !right)                  return WACS_URCORNER;
    if(!bot && !left)                   return WACS_LLCORNER;
    if(!bot && !right)                  return WACS_LRCORNER;
    if(!top)                            return WACS_TTEE;
    if(!bot)                            return WACS_BTEE;
    if(!left)                           return WACS_LTEE;
    if(!right)                          return WACS_RTEE;
    return WACS_PLUS;
}

void
_vk_table_compute_lines(vk_table_t *table,
    int *col_sizes, int *row_sizes, int *vlines, int *hlines)
{
    vk_grid_t   *grid = VK_GRID(table);
    int         cols  = grid->cols;
    int         rows  = grid->rows;
    int         pos;
    int         i;

    _vk_grid_compute_track_sizes(grid, col_sizes, row_sizes);

    vlines[0] = 0;
    pos = 0;
    for(i = 0; i < cols; i++)
    {
        pos += grid->gap + col_sizes[i];
        vlines[i + 1] = pos;
    }
    hlines[0] = 0;
    pos = 0;
    for(i = 0; i < rows; i++)
    {
        pos += grid->gap + row_sizes[i];
        hlines[i + 1] = pos;
    }
}

static void
_vk_table_draw_dividers(vk_table_t *table)
{
    vk_grid_t   *grid   = VK_GRID(table);
    vk_widget_t *widget = VK_WIDGET(table);
    short       fg;
    short       bg;
    short       pair;
    attr_t      attrs;
    int         cols    = grid->cols;
    int         rows    = grid->rows;
    int         *vlines;
    int         *hlines;
    int         *col_sizes;
    int         *row_sizes;
    int         r, c, i;

    if(table->divider_style == VK_BORDER_NONE) return;
    if(cols < 1 || rows < 1) return;

    fg = (table->border_fg == -1) ? widget->fg : table->border_fg;
    bg = (table->border_bg == -1) ? widget->bg : table->border_bg;
    if(fg < 0) fg = COLOR_WHITE;
    if(bg < 0) bg = COLOR_BLACK;

    pair = vdk_color_pair(fg, bg);
    attrs = COLOR_PAIR(pair) | table->border_attrs;

    col_sizes = calloc(cols, sizeof(int));
    row_sizes = calloc(rows, sizeof(int));
    vlines    = calloc(cols + 1, sizeof(int));
    hlines    = calloc(rows + 1, sizeof(int));
    if(col_sizes == NULL || row_sizes == NULL ||
       vlines == NULL || hlines == NULL)
    {
        free(col_sizes); free(row_sizes);
        free(vlines);    free(hlines);
        return;
    }

    _vk_table_compute_lines(table, col_sizes, row_sizes, vlines, hlines);

    wattr_on(widget->canvas, attrs, NULL);

    if(table->divider_style == VK_BORDER_ASCII)
    {
        /* horizontal runs */
        for(r = 0; r <= rows; r++)
        {
            int y = hlines[r];
            for(i = 0; i < widget->width; i++)
                mvwaddch(widget->canvas, y, i, '-' | attrs);
        }
        /* vertical runs */
        for(c = 0; c <= cols; c++)
        {
            int x = vlines[c];
            for(i = 0; i < widget->height; i++)
                mvwaddch(widget->canvas, i, x, '|' | attrs);
        }
        /* junctions */
        for(r = 0; r <= rows; r++)
            for(c = 0; c <= cols; c++)
                mvwaddch(widget->canvas, hlines[r], vlines[c], '+' | attrs);
    }
    else
    {
        const cchar_t *hl = (table->divider_style == VK_BORDER_DOUBLE)
            ? WACS_D_HLINE : WACS_HLINE;
        const cchar_t *vl = (table->divider_style == VK_BORDER_DOUBLE)
            ? WACS_D_VLINE : WACS_VLINE;

        /* horizontal runs across the full width on each hline row */
        for(r = 0; r <= rows; r++)
        {
            int y = hlines[r];
            for(i = 0; i < widget->width; i++)
                mvwadd_wch(widget->canvas, y, i, hl);
        }
        /* vertical runs down the full height on each vline column */
        for(c = 0; c <= cols; c++)
        {
            int x = vlines[c];
            for(i = 0; i < widget->height; i++)
                mvwadd_wch(widget->canvas, i, x, vl);
        }
        /* junctions overlay */
        for(r = 0; r <= rows; r++)
            for(c = 0; c <= cols; c++)
            {
                const cchar_t *j = _vk_table_pick_junction(
                    table->divider_style, r, rows, c, cols);
                mvwadd_wch(widget->canvas, hlines[r], vlines[c], j);
            }
    }

    wattr_off(widget->canvas, attrs, NULL);

    free(col_sizes);
    free(row_sizes);
    free(vlines);
    free(hlines);
}

int
_vk_table_update(vk_grid_t *grid)
{
    vk_table_t  *table;
    int         rc;

    /* run the standard grid layout to paint background + position
       slot widgets; then overlay our dividers on top of the gap
       regions the grid already left blank */
    rc = _vk_grid_update(grid);
    if(rc != 0) return rc;

    table = VK_TABLE(grid);
    _vk_table_draw_dividers(table);

    return 0;
}
