#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"
#include "vk_scroller.h"

static int
_vk_frame_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_frame_dtor(vk_object_t *object);

static int
_vk_frame_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_frame_set_border_style(vk_frame_t *frame, int style);

static int
_vk_frame_set_child(vk_frame_t *frame, vk_widget_t *child);

static int
_vk_frame_draw_border(vk_frame_t *frame);

static int
_vk_frame_on_resize(vk_widget_t *widget);

static int
_vk_frame_recreate(vk_widget_t *widget);

static int
_vk_frame_update(vk_frame_t *frame);

static void
_vk_frame_build_cchar(cchar_t *dest, const cchar_t *src, short pair);


require_klass(VK_CONTAINER_KLASS);

declare_klass(VK_FRAME_KLASS)
{
    .size = KLASS_SIZE(vk_frame_t),
    .name = KLASS_NAME(vk_frame_t),
    .ctor = _vk_frame_ctor,
    .dtor = _vk_frame_dtor,
    .kmio = _vk_frame_kmio,
};


inline vk_frame_t*
vk_frame_create(int width, int height)
{
    vk_frame_t  *frame;

    if(height < 3 || width < 3) return NULL;

    frame = (vk_frame_t*)vk_object_create(VK_FRAME_KLASS, width, height);

    return frame;
}

inline int
vk_frame_set_border_style(vk_frame_t *frame, int style)
{
    if(frame == NULL) return -1;

    if(!vk_object_assert(frame, vk_frame_t)) return -1;

    return frame->_set_border_style(frame, style);
}

inline int
vk_frame_set_border_colors(vk_frame_t *frame, short fg, short bg)
{
    if(frame == NULL) return -1;

    if(!vk_object_assert(frame, vk_frame_t)) return -1;

    frame->border_fg = fg;
    frame->border_bg = bg;

    return 0;
}

inline int
vk_frame_set_child(vk_frame_t *frame, vk_widget_t *child)
{
    if(frame == NULL) return -1;

    if(!vk_object_assert(frame, vk_frame_t)) return -1;

    return frame->_set_child(frame, child);
}

inline vk_widget_t*
vk_frame_get_child(vk_frame_t *frame)
{
    if(frame == NULL) return NULL;

    if(!vk_object_assert(frame, vk_frame_t)) return NULL;

    return frame->child;
}

inline int
vk_frame_update(vk_frame_t *frame)
{
    if(frame == NULL) return -1;

    if(!vk_object_assert(frame, vk_frame_t)) return -1;

    return frame->_update(frame);
}

inline void
vk_frame_destroy(vk_frame_t *frame)
{
    if(frame == NULL) return;

    if(!vk_object_assert(frame, vk_frame_t)) return;

    frame->dtor(VK_OBJECT(frame));

    return;
}


static int
_vk_frame_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_frame_t  *frame;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;

        VK_CONTAINER_KLASS->ctor(object, &args);
        va_end(args);
    }
    else
    {
        VK_CONTAINER_KLASS->ctor(object, argp);
    }

    frame = VK_FRAME(object);

    frame->border_style = VK_FRAME_SINGLE;
    frame->border_fg = -1;
    frame->border_bg = -1;
    frame->child = NULL;

    frame->ctor = _vk_frame_ctor;
    frame->dtor = _vk_frame_dtor;

    frame->_set_border_style = _vk_frame_set_border_style;
    frame->_set_child = _vk_frame_set_child;
    frame->_draw_border = _vk_frame_draw_border;
    frame->_update = _vk_frame_update;

    VK_WIDGET(frame)->_on_resize = _vk_frame_on_resize;
    VK_WIDGET(frame)->_recreate = _vk_frame_recreate;

    return 0;
}

static int
_vk_frame_dtor(vk_object_t *object)
{
    vk_frame_t      *frame;
    vk_container_t  *container;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_frame_t)) return -1;

    frame = VK_FRAME(object);
    container = VK_CONTAINER(object);

    if(frame->child != NULL)
    {
        container->remove_widget(container, frame->child);
        frame->child = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_frame_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_frame_t  *frame;

    frame = VK_FRAME(object);

    if(frame->child == NULL) return 0;

    return vk_object_push_keystroke(VK_OBJECT(frame->child), keystroke);
}

static int
_vk_frame_set_border_style(vk_frame_t *frame, int style)
{
    if(frame == NULL) return -1;

    if(style < VK_FRAME_NONE || style > VK_FRAME_ASCII) return -1;

    frame->border_style = style;

    return 0;
}

static int
_vk_frame_set_child(vk_frame_t *frame, vk_widget_t *child)
{
    vk_widget_t     *widget;
    vk_container_t  *container;

    if(frame == NULL) return -1;

    widget = VK_WIDGET(frame);
    container = VK_CONTAINER(frame);

    if(frame->child != NULL)
    {
        container->remove_widget(container, frame->child);
        frame->child = NULL;
    }

    if(child == NULL) return 0;

    frame->child = child;
    container->add_widget(container, child);

    vk_widget_set_surface(child, widget->canvas);
    vk_widget_move(child, 1, 1);
    vk_widget_resize(child, widget->width - 2, widget->height - 2);

    return 0;
}

static void
_vk_frame_build_cchar(cchar_t *dest, const cchar_t *src, short pair)
{
    wchar_t     wch[CCHARW_MAX];
    attr_t      attrs;
    short       dummy;

    getcchar(src, wch, &attrs, &dummy, NULL);
    setcchar(dest, wch, attrs, pair, NULL);
}

static int
_vk_frame_draw_border(vk_frame_t *frame)
{
    vk_widget_t *widget;
    short       fg;
    short       bg;
    int         border_colors;

    if(frame == NULL) return -1;

    if(frame->border_style == VK_FRAME_NONE) return 0;

    widget = VK_WIDGET(frame);

    fg = (frame->border_fg == -1) ? widget->fg : frame->border_fg;
    bg = (frame->border_bg == -1) ? widget->bg : frame->border_bg;
    border_colors = COLOR_PAIR(vdk_color_pair(fg, bg));

    wattron(widget->canvas, border_colors);

    switch(frame->border_style)
    {
        case VK_FRAME_ASCII:
        {
            wborder(widget->canvas,
                '|', '|', '-', '-', '+', '+', '+', '+');
            break;
        }

        case VK_FRAME_SINGLE:
        {
            cchar_t ls, rs, ts, bs, tl, tr, bl, br;
            short   pair = vdk_color_pair(fg, bg);

            _vk_frame_build_cchar(&ls, WACS_VLINE, pair);
            _vk_frame_build_cchar(&rs, WACS_VLINE, pair);
            _vk_frame_build_cchar(&ts, WACS_HLINE, pair);
            _vk_frame_build_cchar(&bs, WACS_HLINE, pair);
            _vk_frame_build_cchar(&tl, WACS_ULCORNER, pair);
            _vk_frame_build_cchar(&tr, WACS_URCORNER, pair);
            _vk_frame_build_cchar(&bl, WACS_LLCORNER, pair);
            _vk_frame_build_cchar(&br, WACS_LRCORNER, pair);

            wborder_set(widget->canvas,
                &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);
            break;
        }

        case VK_FRAME_DOUBLE:
        {
            cchar_t ls, rs, ts, bs, tl, tr, bl, br;
            short   pair = vdk_color_pair(fg, bg);

            _vk_frame_build_cchar(&ls, WACS_D_VLINE, pair);
            _vk_frame_build_cchar(&rs, WACS_D_VLINE, pair);
            _vk_frame_build_cchar(&ts, WACS_D_HLINE, pair);
            _vk_frame_build_cchar(&bs, WACS_D_HLINE, pair);
            _vk_frame_build_cchar(&tl, WACS_D_ULCORNER, pair);
            _vk_frame_build_cchar(&tr, WACS_D_URCORNER, pair);
            _vk_frame_build_cchar(&bl, WACS_D_LLCORNER, pair);
            _vk_frame_build_cchar(&br, WACS_D_LRCORNER, pair);

            wborder_set(widget->canvas,
                &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);
            break;
        }
    }

    wattroff(widget->canvas, border_colors);

    return 0;
}

static int
_vk_frame_on_resize(vk_widget_t *widget)
{
    vk_frame_t  *frame;

    frame = VK_FRAME(widget);

    if(frame->child != NULL)
        vk_widget_resize(frame->child, widget->width - 2, widget->height - 2);

    if(widget->vscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->vscroller), 1, widget->height);
        vk_widget_move(VK_WIDGET(widget->vscroller), widget->width - 1, 0);
    }

    if(widget->hscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->hscroller), widget->width, 1);
        vk_widget_move(VK_WIDGET(widget->hscroller), 0, widget->height - 1);
    }

    return 0;
}

static int
_vk_frame_recreate(vk_widget_t *widget)
{
    vk_frame_t  *frame;

    widget->canvas = newwin(widget->height, widget->width, 0, 0);

    frame = VK_FRAME(widget);

    if(widget->vscroller != NULL)
    {
        VK_WIDGET(widget->vscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        VK_WIDGET(widget->hscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->hscroller));
    }

    if(frame->child != NULL)
    {
        frame->child->surface = widget->canvas;
        vk_widget_recreate(frame->child);
    }

    return 0;
}

static int
_vk_frame_update(vk_frame_t *frame)
{
    vk_widget_t *widget;

    if(frame == NULL) return -1;

    widget = VK_WIDGET(frame);

    widget->_erase(widget);
    frame->_draw_border(frame);

    if(widget->vscroller != NULL)
    {
        if(vk_scroller_update(widget->vscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        if(vk_scroller_update(widget->hscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->hscroller));
    }

    if(frame->child != NULL)
    {
        vk_widget_draw(frame->child);
    }

    return 0;
}
