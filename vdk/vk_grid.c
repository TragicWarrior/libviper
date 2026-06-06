#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_grid.h"
#include "vk_event.h"

static int
_vk_grid_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_grid_dtor(vk_object_t *object);

static int
_vk_grid_on_resize(vk_object_t *object, int event, void *anything);

static int
_vk_grid_recreate(vk_widget_t *widget);

/* not static so vk_table (and other subclasses) can chain through it
   for layout before overlaying their own decorations */
int
_vk_grid_update(vk_grid_t *grid);

static int
_vk_grid_kmio(vk_object_t *object, int32_t keystroke);

/*
    Internal: compute the on-canvas size of every column (col_sizes)
    and every row (row_sizes), honoring homogeneous mode and the
    per-col / per-row natural-width + expand bits set by the consumer.

    In homogeneous mode every column gets total_w / cols (the last col
    absorbs any remainder); same for rows.  In non-homogeneous mode,
    natural widths are summed, leftover is split among expand-tagged
    columns equally, and columns with neither natural width nor expand
    bit get 0 (effectively invisible).
*/
void
_vk_grid_compute_track_sizes(vk_grid_t *grid,
    int *col_sizes, int *row_sizes)
{
    vk_widget_t *widget = VK_WIDGET(grid);
    int         effective_w;
    int         effective_h;
    int         i;

    /* subtract outer ring (2 * gap) and inner gaps (cols-1)*gap
       => total gap overhead = (cols+1)*gap. same idea for rows. */
    effective_w = widget->width  - (grid->cols + 1) * grid->gap;
    effective_h = widget->height - (grid->rows + 1) * grid->gap;
    if(effective_w < 0) effective_w = 0;
    if(effective_h < 0) effective_h = 0;

    if(grid->homogeneous)
    {
        int pos = 0;

        for(i = 0; i < grid->cols; i++)
        {
            int sz = effective_w / grid->cols;
            if(i == grid->cols - 1) sz = effective_w - pos;
            col_sizes[i] = sz;
            pos += sz;
        }

        pos = 0;
        for(i = 0; i < grid->rows; i++)
        {
            int sz = effective_h / grid->rows;
            if(i == grid->rows - 1) sz = effective_h - pos;
            row_sizes[i] = sz;
            pos += sz;
        }
        return;
    }

    /* non-homogeneous: natural + expand */
    {
        int natural_total = 0;
        int expand_count = 0;
        int leftover;
        int expand_size;
        int expand_remain;
        int expand_idx;

        for(i = 0; i < grid->cols; i++)
        {
            if(grid->col_expand[i]) expand_count++;
            else natural_total += grid->col_widths[i];
        }

        leftover = effective_w - natural_total;
        if(leftover < 0) leftover = 0;

        expand_size = (expand_count > 0) ? leftover / expand_count : 0;
        expand_remain = (expand_count > 0)
            ? leftover - expand_size * expand_count : 0;
        expand_idx = 0;

        for(i = 0; i < grid->cols; i++)
        {
            if(grid->col_expand[i])
            {
                int sz = expand_size;
                expand_idx++;
                if(expand_idx == expand_count) sz += expand_remain;
                col_sizes[i] = sz;
            }
            else
            {
                col_sizes[i] = grid->col_widths[i];
            }
        }

        natural_total = 0;
        expand_count = 0;

        for(i = 0; i < grid->rows; i++)
        {
            if(grid->row_expand[i]) expand_count++;
            else natural_total += grid->row_heights[i];
        }

        leftover = effective_h - natural_total;
        if(leftover < 0) leftover = 0;

        expand_size = (expand_count > 0) ? leftover / expand_count : 0;
        expand_remain = (expand_count > 0)
            ? leftover - expand_size * expand_count : 0;
        expand_idx = 0;

        for(i = 0; i < grid->rows; i++)
        {
            if(grid->row_expand[i])
            {
                int sz = expand_size;
                expand_idx++;
                if(expand_idx == expand_count) sz += expand_remain;
                row_sizes[i] = sz;
            }
            else
            {
                row_sizes[i] = grid->row_heights[i];
            }
        }
    }
}

require_klass(VK_CONTAINER_KLASS);

declare_klass(VK_GRID_KLASS)
{
    .size = KLASS_SIZE(vk_grid_t),
    .name = KLASS_NAME(vk_grid_t),
    .ctor = _vk_grid_ctor,
    .dtor = _vk_grid_dtor,
};

inline vk_grid_t*
vk_grid_create(int width, int height, int cols, int rows)
{
    vk_grid_t   *grid;

    if(width <= 0 || height <= 0) return NULL;
    if(cols < 1 || rows < 1) return NULL;

    grid = (vk_grid_t*)vk_object_create(VK_GRID_KLASS,
        width, height, cols, rows);

    return grid;
}

inline int
vk_grid_set_homogeneous(vk_grid_t *grid, bool homogeneous)
{
    if(grid == NULL) return -1;

    grid->homogeneous = homogeneous;

    return 0;
}

inline int
vk_grid_set_gap(vk_grid_t *grid, int gap)
{
    if(grid == NULL) return -1;
    if(gap < 0) return -1;

    grid->gap = gap;

    return 0;
}

inline int
vk_grid_get_gap(vk_grid_t *grid)
{
    if(grid == NULL) return -1;
    return grid->gap;
}

inline int
vk_grid_set_col_width(vk_grid_t *grid, int col, int width)
{
    if(grid == NULL) return -1;
    if(col < 0 || col >= grid->cols) return -1;
    if(width < 0) return -1;

    grid->col_widths[col] = width;

    return 0;
}

inline int
vk_grid_set_row_height(vk_grid_t *grid, int row, int height)
{
    if(grid == NULL) return -1;
    if(row < 0 || row >= grid->rows) return -1;
    if(height < 0) return -1;

    grid->row_heights[row] = height;

    return 0;
}

inline int
vk_grid_set_col_expand(vk_grid_t *grid, int col, bool expand)
{
    if(grid == NULL) return -1;
    if(col < 0 || col >= grid->cols) return -1;

    grid->col_expand[col] = expand;

    return 0;
}

inline int
vk_grid_set_row_expand(vk_grid_t *grid, int row, bool expand)
{
    if(grid == NULL) return -1;
    if(row < 0 || row >= grid->rows) return -1;

    grid->row_expand[row] = expand;

    return 0;
}

inline int
vk_grid_set_widget(vk_grid_t *grid, int col, int row, vk_widget_t *widget)
{
    vk_container_t  *container;
    int             idx;

    if(grid == NULL) return -1;
    if(col < 0 || col >= grid->cols) return -1;
    if(row < 0 || row >= grid->rows) return -1;

    container = VK_CONTAINER(grid);
    idx = row * grid->cols + col;

    if(grid->slot_widgets[idx] != NULL)
        container->remove_widget(container, grid->slot_widgets[idx]);

    grid->slot_widgets[idx] = widget;

    if(widget != NULL)
    {
        container->add_widget(container, widget);
        vk_widget_set_surface(widget, VK_WIDGET(grid)->canvas);
    }

    return 0;
}

inline vk_widget_t*
vk_grid_get_widget(vk_grid_t *grid, int col, int row)
{
    if(grid == NULL) return NULL;
    if(col < 0 || col >= grid->cols) return NULL;
    if(row < 0 || row >= grid->rows) return NULL;

    return grid->slot_widgets[row * grid->cols + col];
}

inline int
vk_grid_get_cell_rect(vk_grid_t *grid, int col, int row,
    int *out_x, int *out_y, int *out_w, int *out_h)
{
    int     *col_sizes;
    int     *row_sizes;
    int     i;
    int     x, y;

    if(grid == NULL) return -1;
    if(col < 0 || col >= grid->cols) return -1;
    if(row < 0 || row >= grid->rows) return -1;

    col_sizes = calloc(grid->cols, sizeof(int));
    row_sizes = calloc(grid->rows, sizeof(int));
    if(col_sizes == NULL || row_sizes == NULL)
    {
        free(col_sizes);
        free(row_sizes);
        return -1;
    }

    _vk_grid_compute_track_sizes(grid, col_sizes, row_sizes);

    /* cell n's left edge sits past n+1 gap chars (outer + internal) */
    x = grid->gap;
    for(i = 0; i < col; i++) x += col_sizes[i] + grid->gap;

    y = grid->gap;
    for(i = 0; i < row; i++) y += row_sizes[i] + grid->gap;

    if(out_x != NULL) *out_x = x;
    if(out_y != NULL) *out_y = y;
    if(out_w != NULL) *out_w = col_sizes[col];
    if(out_h != NULL) *out_h = row_sizes[row];

    free(col_sizes);
    free(row_sizes);

    return 0;
}

inline int
vk_grid_get_cols(vk_grid_t *grid)
{
    if(grid == NULL) return -1;
    return grid->cols;
}

inline int
vk_grid_get_rows(vk_grid_t *grid)
{
    if(grid == NULL) return -1;
    return grid->rows;
}

inline int
vk_grid_set_subfocus(vk_grid_t *grid, int col, int row)
{
    int     old_col;
    int     old_row;
    int     idx_old;
    int     idx_new;

    if(grid == NULL) return -1;
    if(col < 0 || col >= grid->cols) return -1;
    if(row < 0 || row >= grid->rows) return -1;

    if(col == grid->focused_col && row == grid->focused_row) return 0;

    old_col = grid->focused_col;
    old_row = grid->focused_row;
    grid->focused_col = col;
    grid->focused_row = row;

    idx_old = old_row * grid->cols + old_col;
    idx_new = row * grid->cols + col;

    if(grid->slot_widgets[idx_old] != NULL)
        vk_object_emit(VK_OBJECT(grid->slot_widgets[idx_old]),
            VK_EVENT_ON_UNFOCUS);

    if(grid->slot_widgets[idx_new] != NULL)
        vk_object_emit(VK_OBJECT(grid->slot_widgets[idx_new]),
            VK_EVENT_ON_FOCUS);

    return 0;
}

inline int
vk_grid_get_subfocus_col(vk_grid_t *grid)
{
    if(grid == NULL) return -1;
    return grid->focused_col;
}

inline int
vk_grid_get_subfocus_row(vk_grid_t *grid)
{
    if(grid == NULL) return -1;
    return grid->focused_row;
}

inline int
vk_grid_update(vk_grid_t *grid)
{
    if(grid == NULL) return -1;

    return grid->_update(grid);
}

inline void
vk_grid_destroy(vk_grid_t *grid)
{
    if(grid == NULL) return;

    if(!vk_object_assert(grid, vk_grid_t)) return;

    grid->dtor(VK_OBJECT(grid));
}

static int
_vk_grid_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_grid_t   *grid;
    va_list     args;
    int         cols;
    int         rows;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_CONTAINER_KLASS->ctor(object, argp);

    cols = va_arg(*argp, int);
    rows = va_arg(*argp, int);

    va_end(args);

    if(cols < 1) cols = 1;
    if(rows < 1) rows = 1;

    grid = VK_GRID(object);

    grid->cols = cols;
    grid->rows = rows;
    grid->slot_widgets = calloc(cols * rows, sizeof(vk_widget_t *));
    grid->col_widths   = calloc(cols, sizeof(int));
    grid->row_heights  = calloc(rows, sizeof(int));
    grid->col_expand   = calloc(cols, sizeof(bool));
    grid->row_expand   = calloc(rows, sizeof(bool));
    grid->focused_col  = 0;
    grid->focused_row  = 0;
    grid->homogeneous  = true;
    grid->gap          = 0;

    grid->ctor = _vk_grid_ctor;
    grid->dtor = _vk_grid_dtor;
    grid->_update = _vk_grid_update;

    vk_object_register_event(VK_OBJECT(grid),
        VK_EVENT_ON_RESIZE, _vk_grid_on_resize, NULL);
    VK_WIDGET(grid)->_recreate = _vk_grid_recreate;

    object->kmio = _vk_grid_kmio;

    return 0;
}

static int
_vk_grid_dtor(vk_object_t *object)
{
    vk_grid_t       *grid;
    vk_container_t  *container;
    int             total;
    int             i;

    if(object == NULL) return -1;
    if(!vk_object_assert(object, vk_grid_t)) return -1;

    grid = VK_GRID(object);
    container = VK_CONTAINER(object);

    total = grid->cols * grid->rows;
    for(i = 0; i < total; i++)
    {
        if(grid->slot_widgets[i] != NULL)
        {
            container->remove_widget(container, grid->slot_widgets[i]);
            grid->slot_widgets[i] = NULL;
        }
    }

    free(grid->slot_widgets);    grid->slot_widgets = NULL;
    free(grid->col_widths);      grid->col_widths   = NULL;
    free(grid->row_heights);     grid->row_heights  = NULL;
    free(grid->col_expand);      grid->col_expand   = NULL;
    free(grid->row_expand);      grid->row_expand   = NULL;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

/*
    On resize, recompute every track size and propagate the new cell
    dimensions to any slot-mode children.  Paint-mode cells just get
    queried via get_cell_rect when the consumer wants to repaint.
*/
static int
_vk_grid_on_resize(vk_object_t *object, int event, void *anything)
{
    vk_grid_t   *grid;
    int         *col_sizes;
    int         *row_sizes;
    int         c, r;

    (void)event;
    (void)anything;

    grid = VK_GRID(object);

    col_sizes = calloc(grid->cols, sizeof(int));
    row_sizes = calloc(grid->rows, sizeof(int));
    if(col_sizes == NULL || row_sizes == NULL)
    {
        free(col_sizes);
        free(row_sizes);
        return -1;
    }

    _vk_grid_compute_track_sizes(grid, col_sizes, row_sizes);

    for(r = 0; r < grid->rows; r++)
    {
        for(c = 0; c < grid->cols; c++)
        {
            vk_widget_t *child = grid->slot_widgets[r * grid->cols + c];
            if(child == NULL) continue;
            if(child->state & VK_STATE_EXPAND)
                vk_widget_resize(child, col_sizes[c], row_sizes[r]);
        }
    }

    free(col_sizes);
    free(row_sizes);

    return 0;
}

static int
_vk_grid_recreate(vk_widget_t *widget)
{
    vk_grid_t   *grid;
    int         total;
    int         i;

    widget->canvas = newwin(widget->height, widget->width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

    grid = VK_GRID(widget);
    total = grid->cols * grid->rows;

    for(i = 0; i < total; i++)
    {
        if(grid->slot_widgets[i] != NULL)
        {
            grid->slot_widgets[i]->surface = widget->canvas;
            vk_widget_recreate(grid->slot_widgets[i]);
        }
    }

    return 0;
}

/*
    Route keystrokes to the focused cell's widget (slot mode only).
    Paint-mode consumers override _kmio in their own subclass.
*/
static int
_vk_grid_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_grid_t   *grid = VK_GRID(object);
    vk_widget_t *child;
    int         idx;

    if(grid->focused_col < 0 || grid->focused_col >= grid->cols)
        return -1;
    if(grid->focused_row < 0 || grid->focused_row >= grid->rows)
        return -1;

    idx = grid->focused_row * grid->cols + grid->focused_col;
    child = grid->slot_widgets[idx];
    if(child == NULL) return -1;

    return vk_object_push_keystroke(VK_OBJECT(child), keystroke);
}

int
_vk_grid_update(vk_grid_t *grid)
{
    vk_widget_t *widget;
    int         *col_sizes;
    int         *row_sizes;
    int         *col_offsets;
    int         *row_offsets;
    int         c, r;
    int         pos;

    if(grid == NULL) return -1;

    widget = VK_WIDGET(grid);
    widget->_erase(widget);

    if(widget->fg >= 0 && widget->bg >= 0)
    {
        int colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg))
            | widget->attrs;
        vk_widget_fill(widget, ' ' | colors);
    }

    col_sizes   = calloc(grid->cols, sizeof(int));
    row_sizes   = calloc(grid->rows, sizeof(int));
    col_offsets = calloc(grid->cols, sizeof(int));
    row_offsets = calloc(grid->rows, sizeof(int));
    if(col_sizes == NULL || row_sizes == NULL ||
       col_offsets == NULL || row_offsets == NULL)
    {
        free(col_sizes);   free(row_sizes);
        free(col_offsets); free(row_offsets);
        return -1;
    }

    _vk_grid_compute_track_sizes(grid, col_sizes, row_sizes);

    pos = grid->gap;
    for(c = 0; c < grid->cols; c++)
    {
        col_offsets[c] = pos;
        pos += col_sizes[c] + grid->gap;
    }
    pos = grid->gap;
    for(r = 0; r < grid->rows; r++)
    {
        row_offsets[r] = pos;
        pos += row_sizes[r] + grid->gap;
    }

    for(r = 0; r < grid->rows; r++)
    {
        for(c = 0; c < grid->cols; c++)
        {
            vk_widget_t *child = grid->slot_widgets[r * grid->cols + c];
            int         xoff, yoff;

            if(child == NULL) continue;

            child->surface = widget->canvas;

            xoff = (col_sizes[c] - child->width) / 2;
            yoff = (row_sizes[r] - child->height) / 2;
            if(xoff < 0) xoff = 0;
            if(yoff < 0) yoff = 0;

            vk_widget_move(child,
                col_offsets[c] + xoff, row_offsets[r] + yoff);
            vk_widget_draw(child);
        }
    }

    free(col_sizes);
    free(row_sizes);
    free(col_offsets);
    free(row_offsets);

    return 0;
}
