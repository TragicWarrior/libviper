#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_scroller.h"
#include "vk_viewport.h"

static int
_vk_viewport_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_viewport_dtor(vk_object_t *object);

static int
_vk_viewport_update(vk_viewport_t *vp);

void
vk_viewport_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x);

static int
_vk_viewport_clamp_scroll(vk_viewport_t *vp);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_VIEWPORT_KLASS)
{
    .size = KLASS_SIZE(vk_viewport_t),
    .name = KLASS_NAME(vk_viewport_t),
    .ctor = _vk_viewport_ctor,
    .dtor = _vk_viewport_dtor,
};

inline vk_viewport_t*
vk_viewport_create(int width, int height)
{
    vk_viewport_t   *vp;

    if(width <= 0 || height <= 0) return NULL;

    vp = (vk_viewport_t*)vk_object_create(VK_VIEWPORT_KLASS, width, height);

    return vp;
}

inline int
vk_viewport_set_src(vk_viewport_t *vp,
    const vk_viewport_src_t *src)
{
    if(vp == NULL) return -1;

    if(src == NULL)
    {
        memset(&vp->src, 0, sizeof(vp->src));
        vp->src.rows = -1;
        vp->src.cols = -1;
    }
    else
    {
        vp->src = *src;
    }

    _vk_viewport_clamp_scroll(vp);

    return 0;
}

inline int
vk_viewport_set_scroll(vk_viewport_t *vp, int row, int col)
{
    if(vp == NULL) return -1;

    vp->scroll_row = row;
    vp->scroll_col = col;

    _vk_viewport_clamp_scroll(vp);

    return 0;
}

inline int
vk_viewport_get_scroll(vk_viewport_t *vp, int *row, int *col)
{
    if(vp == NULL) return -1;

    if(row != NULL) *row = vp->scroll_row;
    if(col != NULL) *col = vp->scroll_col;

    return 0;
}

inline int
vk_viewport_scroll_by(vk_viewport_t *vp, int drow, int dcol)
{
    if(vp == NULL) return -1;

    return vk_viewport_set_scroll(vp,
        vp->scroll_row + drow, vp->scroll_col + dcol);
}

inline int
vk_viewport_pgup(vk_viewport_t *vp)
{
    int     h;

    if(vp == NULL) return -1;

    h = VK_WIDGET(vp)->height;
    return vk_viewport_scroll_by(vp, -h, 0);
}

inline int
vk_viewport_pgdn(vk_viewport_t *vp)
{
    int     h;

    if(vp == NULL) return -1;

    h = VK_WIDGET(vp)->height;
    return vk_viewport_scroll_by(vp, h, 0);
}

inline int
vk_viewport_update(vk_viewport_t *vp)
{
    if(vp == NULL) return -1;

    return vp->_update(vp);
}

inline void
vk_viewport_destroy(vk_viewport_t *vp)
{
    if(vp == NULL) return;

    if(!vk_object_assert(vp, vk_viewport_t)) return;

    vp->dtor(VK_OBJECT(vp));
}

/*
    Clamp scroll_row / scroll_col into [0, max] given the source
    extents and the visible area.  Sources with rows == -1 or
    cols == -1 are treated as unbounded and aren't clamped.
*/
static int
_vk_viewport_clamp_scroll(vk_viewport_t *vp)
{
    vk_widget_t *widget;
    int         max_row;
    int         max_col;

    if(vp == NULL) return -1;

    widget = VK_WIDGET(vp);

    if(vp->src.rows >= 0)
    {
        max_row = vp->src.rows - widget->height;
        if(max_row < 0) max_row = 0;
        if(vp->scroll_row > max_row) vp->scroll_row = max_row;
    }
    if(vp->scroll_row < 0) vp->scroll_row = 0;

    if(vp->src.cols >= 0)
    {
        max_col = vp->src.cols - widget->width;
        if(max_col < 0) max_col = 0;
        if(vp->scroll_col > max_col) vp->scroll_col = max_col;
    }
    if(vp->scroll_col < 0) vp->scroll_col = 0;

    return 0;
}

static int
_vk_viewport_update(vk_viewport_t *vp)
{
    vk_widget_t *widget;
    cchar_t     *row_buf;
    int         visible_rows;
    int         visible_cols;
    int         logical_row;
    int         filled;
    int         r;
    int         c;

    if(vp == NULL) return -1;

    widget = VK_WIDGET(vp);

    widget->_erase(widget);

    if(vp->src.get_row == NULL) return 0;

    visible_rows = widget->height;
    visible_cols = widget->width;

    if(visible_cols <= 0 || visible_rows <= 0) return 0;

    row_buf = (cchar_t *)calloc(visible_cols, sizeof(cchar_t));
    if(row_buf == NULL) return -1;

    for(r = 0; r < visible_rows; r++)
    {
        logical_row = vp->scroll_row + r;

        if(vp->src.rows >= 0 && logical_row >= vp->src.rows) break;

        memset(row_buf, 0, sizeof(cchar_t) * visible_cols);

        filled = vp->src.get_row(vp->src.anything, logical_row,
            vp->scroll_col, row_buf, visible_cols);

        if(filled < 0) continue;
        if(filled > visible_cols) filled = visible_cols;

        for(c = 0; c < filled; c++)
            mvwadd_wch(widget->canvas, r, c, &row_buf[c]);
    }

    free(row_buf);

    /* refresh attached scroller if any */
    if(widget->vscroller != NULL) vk_scroller_update(widget->vscroller);
    if(widget->hscroller != NULL) vk_scroller_update(widget->hscroller);

    return 0;
}

/*
    Default scroll-info callback for a vk_scroller attached to a
    viewport -- reports the source's content extents and the
    viewport's current scroll position.  Wire it on the scroller
    before attaching to the viewport widget:

        vk_scroller_set_scroll_info(scr, vk_viewport_scroll_info);
        vk_widget_attach_scroller(VK_WIDGET(vp), scr);
*/
void
vk_viewport_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_viewport_t   *vp;

    if(child == NULL) return;
    vp = VK_VIEWPORT(child);

    if(content_h != NULL)
        *content_h = (vp->src.rows >= 0) ? vp->src.rows : 0;
    if(content_w != NULL)
        *content_w = (vp->src.cols >= 0) ? vp->src.cols : 0;
    if(scroll_y != NULL) *scroll_y = vp->scroll_row;
    if(scroll_x != NULL) *scroll_x = vp->scroll_col;
}

static int
_vk_viewport_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_viewport_t   *vp;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;

        VK_WIDGET_KLASS->ctor(object, &args);
        va_end(args);
    }
    else
    {
        VK_WIDGET_KLASS->ctor(object, argp);
    }

    vp = VK_VIEWPORT(object);

    memset(&vp->src, 0, sizeof(vp->src));
    vp->src.rows = -1;
    vp->src.cols = -1;

    vp->scroll_row = 0;
    vp->scroll_col = 0;

    vp->ctor = _vk_viewport_ctor;
    vp->dtor = _vk_viewport_dtor;
    vp->_update = _vk_viewport_update;

    return 0;
}

static int
_vk_viewport_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;
    if(!vk_object_assert(object, vk_viewport_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}
