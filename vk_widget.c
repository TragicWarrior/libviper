#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <curses.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"

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
_vk_widget_erase(vk_widget_t *widget);


static vk_object_t VK_WIDGET_KLASS =
{
    .size = KLASS_SIZE(vk_widget_t),
    .name = KLASS_NAME(vk_widget_t),
    .ctor = _vk_widget_ctor,
    .dtor = _vk_widget_dtor,
};


// create a new widget from scratch
vk_widget_t*
vk_widget_create(int width, int height)
{
    vk_widget_t     *widget;

    if(height == 0 || width == 0) return NULL;

    widget = (vk_widget_t*)vk_object_create(VK_WIDGET_KLASS,
        width, height);

    return widget;
}


int
vk_widget_set_surface(vk_widget_t *widget, WINDOW *surface)
{
    if(widget == NULL) return -1;
    if(surface == NULL) return -1;

    widget->surface = surface;

    return 0;
}

WINDOW*
vk_widget_get_surface(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->surface;
}

void
vk_widget_set_colors(vk_widget_t *widget, short color_pair)
{
    if(widget == NULL) return;
    if(color_pair < 0) return;

    wbkgdset(widget->canvas, COLOR_PAIR(color_pair));
    wcolor_set(widget->canvas, color_pair, NULL);
    pair_content(color_pair, &widget->fg, &widget->bg);

    return;
}

int
vk_widget_erase(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_erase(widget);

    return retval;
}

void
vk_widget_fill(vk_widget_t *widget, chtype ch)
{
    long            i;

    if(widget == NULL) return;

    i = widget->width * widget->height;

    while(i)
    {
        waddch(widget->canvas, ch);
        i--;
    }

    return;
}

int
vk_widget_move(vk_widget_t *widget, int x, int y)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_move(widget, x, y);

    return retval;
}

void
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
    widget->ctor = _vk_widget_ctor;
    widget->dtor = _vk_widget_dtor;
    widget->_move = _vk_widget_move;
    widget->_draw = _vk_widget_draw;
    widget->_resize = _vk_widget_resize;
    widget->_erase = _vk_widget_erase;

    // interate through var args after constructing base klass
    width = va_arg(*argp, int);
    height = va_arg(*argp, int);
    va_end(args);

    widget->canvas = newwin(height, width, 0, 0);
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
    int     top;
    int     bottom;
    int     left;
    int     right;

    if(widget == NULL) return -1;

    left = widget->x;
    right = widget->x + widget->width;
    top = widget->y;
    bottom = widget->y + widget->height;

    // destructive copy of canvas to surface
    copywin(widget->canvas, widget->surface,
        0, 0, left, top, right, bottom, FALSE);

    return 0;
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
    if(object == NULL) return -1;

    delwin(VK_WIDGET(object)->canvas);

    vk_object_demote(object, vk_object_t);
    vk_object_destroy(object);

    return 0;
}
