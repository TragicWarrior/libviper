#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vkmio.h"

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_grid.h"
#include "vk_table.h"
#include "vk_color.h"
#include "vk_event.h"

static int
_vk_color_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_color_dtor(vk_object_t *object);

static int
_vk_color_update(vk_grid_t *grid);

static int
_vk_color_kmio(vk_object_t *object, int32_t keystroke);

require_klass(VK_TABLE_KLASS);

declare_klass(VK_COLOR_KLASS)
{
    .size = KLASS_SIZE(vk_color_t),
    .name = KLASS_NAME(vk_color_t),
    .ctor = _vk_color_ctor,
    .dtor = _vk_color_dtor,
};

inline vk_color_t*
vk_color_create(int width, int height, int cols, int rows, int divider_style)
{
    vk_color_t  *color;

    if(width <= 0 || height <= 0) return NULL;
    if(cols < 1 || rows < 1) return NULL;
    if(cols * rows != 16) return NULL;

    color = (vk_color_t*)vk_object_create(VK_COLOR_KLASS,
        width, height, cols, rows, divider_style);

    return color;
}

inline int
vk_color_set_selected(vk_color_t *color, short idx)
{
    if(color == NULL) return -1;
    if(idx < 0 || idx > 15) return -1;

    if(color->selected == idx) return 0;

    color->selected = idx;

    vk_object_emit(VK_OBJECT(color), VK_EVENT_ON_SELECT);

    return 0;
}

inline short
vk_color_get_selected(vk_color_t *color)
{
    if(color == NULL) return -1;
    return color->selected;
}

inline int
vk_color_set_focus_colors(vk_color_t *color, short fg, short bg)
{
    if(color == NULL) return -1;

    color->focus_fg = fg;
    color->focus_bg = bg;

    return 0;
}

inline int
vk_color_set_focus_attrs(vk_color_t *color, attr_t attrs)
{
    if(color == NULL) return -1;

    color->focus_attrs = attrs;

    return 0;
}

inline int
vk_color_update(vk_color_t *color)
{
    if(color == NULL) return -1;
    return vk_grid_update(VK_GRID(color));
}

inline void
vk_color_destroy(vk_color_t *color)
{
    if(color == NULL) return;

    if(!vk_object_assert(color, vk_color_t)) return;

    color->dtor(VK_OBJECT(color));
}

static int
_vk_color_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_color_t  *color;
    vk_grid_t   *grid;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* table takes (width, height, cols, rows, divider_style) -- no
       additional args here, vk_color is the leaf class */
    VK_TABLE_KLASS->ctor(object, argp);

    va_end(args);

    color = VK_COLOR(object);
    grid = VK_GRID(object);

    color->selected    = 0;
    color->focus_fg    = COLOR_YELLOW;
    color->focus_bg    = -1;            /* -1 = transparent (use widget bg) */
    color->focus_attrs = A_BOLD;

    color->ctor = _vk_color_ctor;
    color->dtor = _vk_color_dtor;

    /* take over the grid's update slot so vk_grid_update / vk_table_update
       on a vk_color routes through our paint + dividers + focus overlay */
    grid->_update = _vk_color_update;

    /* take over the kmio slot for arrow / Tab / Enter navigation */
    object->kmio = _vk_color_kmio;

    return 0;
}

static int
_vk_color_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;
    if(!vk_object_assert(object, vk_color_t)) return -1;

    vk_object_demote(object, vk_table_t);
    vk_table_destroy(VK_TABLE(object));

    return 0;
}

/*
    paint a single cell's interior (everything inside its dividers)
    with the given background color.  the cell rect from
    vk_grid_get_cell_rect already excludes the gap channels where the
    table's dividers live, so this never trespasses on the borders.
*/
static void
_vk_color_paint_cell(vk_widget_t *widget, vk_grid_t *grid,
    int col, int row, short color_idx)
{
    int     x, y, w, h;
    int     yy, xx;
    short   pair;
    int     attr;

    if(vk_grid_get_cell_rect(grid, col, row, &x, &y, &w, &h) != 0) return;

    pair = vdk_color_pair(COLOR_BLACK, color_idx);
    attr = COLOR_PAIR(pair);

    for(yy = 0; yy < h; yy++)
        for(xx = 0; xx < w; xx++)
            mvwaddch(widget->canvas, y + yy, x + xx, ' ' | attr);
}

/*
    redraw the focused cell's 4 edges and 4 junction characters in
    the highlight color.  uses the same junction-picker as vk_table so
    the corner glyphs stay consistent (ULCORNER vs TTEE vs PLUS etc.
    depending on whether the cell sits on an outer edge of the grid).
*/
static void
_vk_color_overdraw_focus(vk_color_t *color, vk_grid_t *grid,
    vk_widget_t *widget)
{
    vk_table_t  *table = VK_TABLE(color);
    int         cols   = grid->cols;
    int         rows   = grid->rows;
    int         f_col  = color->selected % cols;
    int         f_row  = color->selected / cols;
    int         *col_sizes;
    int         *row_sizes;
    int         *vlines;
    int         *hlines;
    short       fg;
    short       bg;
    short       pair;
    attr_t      attrs;
    int         style;
    int         x, y;
    int         i;

    style = vk_table_get_divider_style(table);
    if(style == VK_BORDER_NONE) return;

    fg = (color->focus_fg < 0) ? widget->fg : color->focus_fg;
    bg = (color->focus_bg < 0) ? widget->bg : color->focus_bg;
    if(fg < 0) fg = COLOR_YELLOW;
    if(bg < 0) bg = COLOR_BLACK;

    pair  = vdk_color_pair(fg, bg);
    attrs = COLOR_PAIR(pair) | color->focus_attrs;

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

    if(style == VK_BORDER_ASCII)
    {
        /* top + bottom edges */
        for(x = vlines[f_col] + 1; x < vlines[f_col + 1]; x++)
        {
            mvwaddch(widget->canvas, hlines[f_row],     x, '-' | attrs);
            mvwaddch(widget->canvas, hlines[f_row + 1], x, '-' | attrs);
        }
        /* left + right edges */
        for(y = hlines[f_row] + 1; y < hlines[f_row + 1]; y++)
        {
            mvwaddch(widget->canvas, y, vlines[f_col],     '|' | attrs);
            mvwaddch(widget->canvas, y, vlines[f_col + 1], '|' | attrs);
        }
        /* 4 corners */
        mvwaddch(widget->canvas, hlines[f_row],     vlines[f_col],     '+' | attrs);
        mvwaddch(widget->canvas, hlines[f_row],     vlines[f_col + 1], '+' | attrs);
        mvwaddch(widget->canvas, hlines[f_row + 1], vlines[f_col],     '+' | attrs);
        mvwaddch(widget->canvas, hlines[f_row + 1], vlines[f_col + 1], '+' | attrs);
    }
    else
    {
        const cchar_t *hl = (style == VK_BORDER_DOUBLE)
            ? WACS_D_HLINE : WACS_HLINE;
        const cchar_t *vl = (style == VK_BORDER_DOUBLE)
            ? WACS_D_VLINE : WACS_VLINE;
        cchar_t cc;
        wchar_t wch[CCHARW_MAX];
        attr_t  src_attr;
        short   dummy;

        wattr_on(widget->canvas, attrs, NULL);

        /* top + bottom edges (line chars), repainted with current attrs */
        getcchar(hl, wch, &src_attr, &dummy, NULL);
        setcchar(&cc, wch, src_attr | attrs, pair, NULL);
        for(x = vlines[f_col] + 1; x < vlines[f_col + 1]; x++)
        {
            mvwadd_wch(widget->canvas, hlines[f_row],     x, &cc);
            mvwadd_wch(widget->canvas, hlines[f_row + 1], x, &cc);
        }

        /* left + right edges */
        getcchar(vl, wch, &src_attr, &dummy, NULL);
        setcchar(&cc, wch, src_attr | attrs, pair, NULL);
        for(y = hlines[f_row] + 1; y < hlines[f_row + 1]; y++)
        {
            mvwadd_wch(widget->canvas, y, vlines[f_col],     &cc);
            mvwadd_wch(widget->canvas, y, vlines[f_col + 1], &cc);
        }

        /* 4 junctions -- same glyphs vk_table picked, just our color */
        {
            const cchar_t *jtl = _vk_table_pick_junction(style,
                f_row,     rows, f_col,     cols);
            const cchar_t *jtr = _vk_table_pick_junction(style,
                f_row,     rows, f_col + 1, cols);
            const cchar_t *jbl = _vk_table_pick_junction(style,
                f_row + 1, rows, f_col,     cols);
            const cchar_t *jbr = _vk_table_pick_junction(style,
                f_row + 1, rows, f_col + 1, cols);

            getcchar(jtl, wch, &src_attr, &dummy, NULL);
            setcchar(&cc, wch, src_attr | attrs, pair, NULL);
            mvwadd_wch(widget->canvas, hlines[f_row],     vlines[f_col],     &cc);

            getcchar(jtr, wch, &src_attr, &dummy, NULL);
            setcchar(&cc, wch, src_attr | attrs, pair, NULL);
            mvwadd_wch(widget->canvas, hlines[f_row],     vlines[f_col + 1], &cc);

            getcchar(jbl, wch, &src_attr, &dummy, NULL);
            setcchar(&cc, wch, src_attr | attrs, pair, NULL);
            mvwadd_wch(widget->canvas, hlines[f_row + 1], vlines[f_col],     &cc);

            getcchar(jbr, wch, &src_attr, &dummy, NULL);
            setcchar(&cc, wch, src_attr | attrs, pair, NULL);
            mvwadd_wch(widget->canvas, hlines[f_row + 1], vlines[f_col + 1], &cc);
        }

        wattr_off(widget->canvas, attrs, NULL);
    }

    (void)i;
    free(col_sizes);
    free(row_sizes);
    free(vlines);
    free(hlines);
}

static int
_vk_color_update(vk_grid_t *grid)
{
    vk_color_t  *color  = VK_COLOR(grid);
    vk_widget_t *widget = VK_WIDGET(grid);
    int         cols    = grid->cols;
    int         rows    = grid->rows;
    int         c, r;

    /* 1. let vk_table do its full pass: bg fill + dividers in the
          table's normal border color.  cells stay empty since we run
          in paint mode (no slot widgets). */
    if(_vk_table_update(grid) != 0) return -1;

    /* 2. paint each of the 16 cells with its color */
    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            short idx = r * cols + c;
            _vk_color_paint_cell(widget, grid, c, r, idx);
        }
    }

    /* 3. overdraw the focused cell's border in the highlight color */
    _vk_color_overdraw_focus(color, grid, widget);

    return 0;
}

/*
    arrow keys move within the grid bounds (clamped), Tab and Shift-Tab
    cycle through all cells in row-major order (wrapping), Enter emits
    ON_ACTIVATE.  any other keystroke propagates back unhandled.
*/
static int
_vk_color_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_color_t  *color = VK_COLOR(object);
    vk_grid_t   *grid  = VK_GRID(object);
    int         cols   = grid->cols;
    int         rows   = grid->rows;
    int         total  = cols * rows;
    int         new_sel = color->selected;
    int         col, row;

    col = color->selected % cols;
    row = color->selected / cols;

    switch(keystroke)
    {
        case KEY_RIGHT:
            if(col < cols - 1) new_sel = color->selected + 1;
            break;
        case KEY_LEFT:
            if(col > 0) new_sel = color->selected - 1;
            break;
        case KEY_DOWN:
            if(row < rows - 1) new_sel = color->selected + cols;
            break;
        case KEY_UP:
            if(row > 0) new_sel = color->selected - cols;
            break;
        case '\t':                  /* KEY_TAB == '\t' */
            new_sel = (color->selected + 1) % total;
            break;
        case KEY_BTAB:
            new_sel = (color->selected + total - 1) % total;
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            vk_object_emit(object, VK_EVENT_ON_ACTIVATE);
            return KMIO_HANDLED;
        default:
            return keystroke;
    }

    if(new_sel != color->selected)
    {
        color->selected = new_sel;
        vk_object_emit(object, VK_EVENT_ON_SELECT);
    }

    return KMIO_HANDLED;
}
