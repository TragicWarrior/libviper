#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ncursesw/ncurses.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_event.h"

static int
_vk_widget_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_widget_dtor(vk_object_t *object);

static int
_vk_widget_move(vk_widget_t *widget, int x, int y);

static int
_vk_widget_draw(vk_widget_t *widget);

static int
_vk_widget_resize(vk_widget_t *widget, int width, int height);

static int
_vk_widget_recreate(vk_widget_t *widget);

static int
_vk_widget_erase(vk_widget_t *widget);

require_klass(VK_OBJECT_KLASS);

declare_klass(VK_WIDGET_KLASS)
{
    .size = KLASS_SIZE(vk_widget_t),
    .name = KLASS_NAME(vk_widget_t),
    .ctor = _vk_widget_ctor,
    .dtor = _vk_widget_dtor,
};

// create a new widget from scratch
inline vk_widget_t*
vk_widget_create(int width, int height)
{
    vk_widget_t     *widget;

    if(height == 0 || width == 0) return NULL;

    widget = (vk_widget_t*)vk_object_create(VK_WIDGET_KLASS,
        width, height);

    return widget;
}

inline int
vk_widget_set_surface(vk_widget_t *widget, WINDOW *surface)
{
    if(widget == NULL) return -1;
    if(surface == NULL) return -1;

    widget->surface = surface;

    return 0;
}

inline WINDOW*
vk_widget_get_surface(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->surface;
}

inline int
vk_widget_draw(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;
    if(!(widget->state & VK_STATE_VISIBLE)) return 0;

    retval = widget->_draw(widget);

    return retval;
}

inline uint32_t
vk_widget_get_state(vk_widget_t *widget)
{
    if(widget == NULL) return 0;

    return widget->state;
}

inline void
vk_widget_set_state(vk_widget_t *widget, uint32_t state)
{
    uint32_t    old_state;

    if(widget == NULL) return;

    old_state = widget->state;
    widget->state = state;

    if((state & VK_STATE_FROZEN) && !(old_state & VK_STATE_FROZEN))
    {
        widget->composer = newwin(widget->height, widget->width, 0, 0);
        overwrite(widget->canvas, widget->composer);
    }

    if(!(state & VK_STATE_FROZEN) && (old_state & VK_STATE_FROZEN))
    {
        if(widget->composer != widget->canvas)
        {
            delwin(widget->composer);
            widget->composer = widget->canvas;
        }
    }
}

inline void
vk_widget_set_colors(vk_widget_t *widget, int fg, int bg)
{
    if(widget == NULL) return;

    widget->fg = fg;
    widget->bg = bg;

    return;
}

inline void
vk_widget_set_attrs(vk_widget_t *widget, attr_t attrs)
{
    if(widget == NULL) return;

    widget->attrs = attrs;
}

inline int
vk_widget_get_colors(vk_widget_t *widget, short *fg, short *bg)
{
    if(widget == NULL) return -1;

    if(fg != NULL) *fg = widget->fg;
    if(bg != NULL) *bg = widget->bg;

    return 0;
}

inline attr_t
vk_widget_get_attrs(vk_widget_t *widget)
{
    if(widget == NULL) return 0;

    return widget->attrs;
}

inline WINDOW*
vk_widget_get_canvas(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->canvas;
}

inline int
vk_widget_get_metrics(vk_widget_t *widget, int *width, int *height)
{
    if(widget == NULL) return -1;
    if(width == NULL && height == NULL) return -1;

    if(width !=NULL) *width = widget->width;
    if(height != NULL) *height = widget->height;

    return 0;
}

inline int
vk_widget_resize(vk_widget_t *widget, int width, int height)
{
    int retval;

    if(widget == NULL) return -1;
    if(widget->state & VK_STATE_NORESIZE) return -1;
    if(width == WSIZE_UNCHANGED) width = widget->width;
    if(height == WSIZE_UNCHANGED) height = widget->height;

    if(width < 0 || height < 0) return -1;
    if(width == widget->width && height == widget->height) return 0;

    retval = widget->_resize(widget, width, height);

    if(retval == 0)
        vk_object_emit(VK_OBJECT(widget), VK_EVENT_ON_RESIZE);

    return retval;
}

inline int
vk_widget_erase(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_erase(widget);

    return retval;
}

inline void
vk_widget_fill(vk_widget_t *widget, chtype ch)
{
    long            i;

    if(widget == NULL) return;
    if(widget->canvas == NULL) return;

    wmove(widget->canvas, 0, 0);

    i = widget->width * widget->height;

    while(i)
    {
        waddch(widget->canvas, ch);
        i--;
    }

    return;
}

inline int
vk_widget_move(vk_widget_t *widget, int x, int y)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_move(widget, x, y);

    return retval;
}

inline int
vk_widget_recreate(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_recreate(widget);

    if(retval == 0)
        vk_object_emit(VK_OBJECT(widget), VK_EVENT_ON_RECREATE);

    return retval;
}

inline void
vk_widget_destroy(vk_widget_t *widget)
{
    if(widget == NULL) return;

    if(!vk_object_assert(widget, vk_widget_t)) return;

    widget->dtor(VK_OBJECT(widget));

    return;
}

static int
_vk_widget_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_widget_t *widget;
    va_list     args;
    int         width;
    int         height;

    if(object == NULL) return -1;

    /*
        if argp is set then we're being called by a superclass.
        otherwise, we're being called directly.
    */
    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    // install our derived klass methods
    widget = VK_WIDGET(object);

    widget->fg = COLOR_BLACK;
    widget->bg = COLOR_WHITE;
    widget->state = VK_STATE_VISIBLE;

    widget->ctor = _vk_widget_ctor;
    widget->dtor = _vk_widget_dtor;

    widget->_move = _vk_widget_move;
    widget->_draw = _vk_widget_draw;
    widget->_resize = _vk_widget_resize;
    widget->_recreate = _vk_widget_recreate;
    widget->_erase = _vk_widget_erase;

    // interate through var args after constructing base klass
    width = va_arg(*argp, int);
    height = va_arg(*argp, int);
    va_end(args);

    widget->canvas = newwin(height, width, 0, 0);
    widget->composer = widget->canvas;
    widget->width = width;
    widget->height = height;
    widget->x = 0;
    widget->y = 0;

    return 0;
}

static int
_vk_widget_move(vk_widget_t *widget, int x, int y)
{
    int     retval = 0;

    if(widget == NULL) return -1;
    if(x == WPOS_UNCHANGED && y == WPOS_UNCHANGED) return 0;

    if(x == WPOS_UNCHANGED) x = widget->x;
    if(y == WPOS_UNCHANGED) y = widget->y;

    retval = mvwin(widget->canvas, y, x);

    if(retval == ERR) return -1;

    widget->x = x;
    widget->y = y;

    return 0;
}

static int
_vk_widget_resize(vk_widget_t *widget, int width, int height)
{
    WINDOW  *copy_pad;
    int     pad_width;
    int     pad_height;
    int     pad_x;
    int     pad_y;

    // create a copy of the window contents before resizing
    getmaxyx(widget->canvas, pad_height, pad_width);
    getbegyx(widget->canvas, pad_y, pad_x);
    copy_pad = newwin(pad_height - 1, pad_width - 1, pad_y, pad_x);
    overwrite(widget->canvas, copy_pad);

    // erase and resize canvas
    werase(widget->canvas);
    wresize(widget->canvas, height, width);

    // copy the contents back (overwrite() will clip as needed)
    overwrite(copy_pad, widget->canvas);

    // delete our copy pad
    delwin(copy_pad);

    // update width and height properties of widget
    widget->width = width;
    widget->height = height;

    return 0;
}

static int
_vk_widget_draw(vk_widget_t *widget)
{
    int     retval;
    int     dmincol;
    int     dminrow;
    int     dmaxcol;
    int     dmaxrow;
    int     max_x;
    int     max_y;

    if(widget == NULL) return -1;

    getmaxyx(widget->surface, max_y, max_x);

    dmincol = widget->x;
    dminrow = widget->y;

    // set copy horizontal boundary
    if(widget->x + widget->width > max_x)
        dmaxcol = max_x;
    else
        dmaxcol = widget->x + widget->width;

    // set copy vertical boundary
    if(widget->y + widget->height > max_y)
        dmaxrow = max_y;
    else
        dmaxrow = widget->y + widget->height;

    retval = copywin(widget->composer, widget->surface,
        0, 0,
        dminrow, dmincol,
        dmaxrow - 1, dmaxcol -1,
        FALSE);

    if(retval == ERR) return -1;

    return 0;
}

static int
_vk_widget_recreate(vk_widget_t *widget)
{
    if(widget == NULL) return -1;

    if(widget->composer != widget->canvas)
        delwin(widget->composer);

    widget->canvas = newwin(widget->height, widget->width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

    return (widget->canvas == NULL) ? -1 : 0;
}

static int
_vk_widget_erase(vk_widget_t *widget)
{
    if(widget == NULL) return -1;
    if(widget->canvas == NULL) return -1;

    werase(widget->canvas);

    return 0;
}

static int
_vk_widget_dtor(vk_object_t *object)
{
    vk_widget_t *widget;

    if(object == NULL) return -1;

    widget = VK_WIDGET(object);

    if(widget->composer != widget->canvas)
        delwin(widget->composer);

    delwin(widget->canvas);

    vk_object_demote(object, vk_object_t);
    vk_object_destroy(object);

    return 0;
}
