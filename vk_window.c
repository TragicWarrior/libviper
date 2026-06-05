#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"
#include "vk_scroller.h"
#include "vk_window.h"

static int
_vk_window_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_window_dtor(vk_object_t *object);

static int
_vk_window_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_window_update(vk_frame_t *frame);

static int
_vk_window_draw_title(vk_window_t *window);

static int
_vk_window_recreate(vk_widget_t *widget);


require_klass(VK_FRAME_KLASS);

declare_klass(VK_WINDOW_KLASS)
{
    .size = KLASS_SIZE(vk_window_t),
    .name = KLASS_NAME(vk_window_t),
    .ctor = _vk_window_ctor,
    .dtor = _vk_window_dtor,
    .kmio = _vk_window_kmio,
};


vk_window_t*
vk_window_create(int width, int height)
{
    vk_window_t *window;

    if(height < 3 || width < 3) return NULL;

    window = (vk_window_t*)vk_object_create(VK_WINDOW_KLASS,
        width, height);

    return window;
}

int
vk_window_set_title(vk_window_t *window, const char *title)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    if(window->title != NULL) free(window->title);

    window->title = (title != NULL) ? strdup(title) : NULL;

    return 0;
}

const char*
vk_window_get_title(vk_window_t *window)
{
    if(window == NULL) return NULL;

    if(!vk_object_assert(window, vk_window_t)) return NULL;

    return window->title;
}

int
vk_window_set_title_justify(vk_window_t *window, int justify)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    if(justify < VK_JUSTIFY_LEFT || justify > VK_JUSTIFY_CENTER) return -1;

    window->title_justify = justify;

    return 0;
}

int
vk_window_set_decorate(vk_window_t *window, VkWindowDecorateFunc func,
    void *data)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    window->on_decorate = func;
    window->decorate_data = data;

    return 0;
}

int
vk_window_set_border_style(vk_window_t *window, int style)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    return VK_FRAME(window)->_set_border_style(VK_FRAME(window), style);
}

int
vk_window_set_border_colors(vk_window_t *window, short fg, short bg)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    VK_FRAME(window)->border_fg = fg;
    VK_FRAME(window)->border_bg = bg;

    return 0;
}

int
vk_window_set_child(vk_window_t *window, vk_widget_t *child)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    return VK_FRAME(window)->_set_child(VK_FRAME(window), child);
}

vk_widget_t*
vk_window_get_child(vk_window_t *window)
{
    if(window == NULL) return NULL;

    if(!vk_object_assert(window, vk_window_t)) return NULL;

    return VK_FRAME(window)->child;
}

int
vk_window_update(vk_window_t *window)
{
    if(window == NULL) return -1;

    if(!vk_object_assert(window, vk_window_t)) return -1;

    return VK_FRAME(window)->_update(VK_FRAME(window));
}

void
vk_window_destroy(vk_window_t *window)
{
    if(window == NULL) return;

    if(!vk_object_assert(window, vk_window_t)) return;

    window->dtor(VK_OBJECT(window));

    return;
}


static int
_vk_window_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_frame_t  *frame;

    frame = VK_FRAME(object);

    if(frame->child == NULL) return 0;

    return vk_object_push_keystroke(VK_OBJECT(frame->child), keystroke);
}

static int
_vk_window_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_window_t *window;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;

        VK_FRAME_KLASS->ctor(object, &args);
        va_end(args);
    }
    else
    {
        VK_FRAME_KLASS->ctor(object, argp);
    }

    window = VK_WINDOW(object);

    window->title = NULL;
    window->title_justify = VK_JUSTIFY_CENTER;
    window->on_decorate = NULL;
    window->decorate_data = NULL;

    window->ctor = _vk_window_ctor;
    window->dtor = _vk_window_dtor;

    window->_draw_title = _vk_window_draw_title;

    VK_FRAME(window)->_update = _vk_window_update;
    VK_WIDGET(window)->_recreate = _vk_window_recreate;

    return 0;
}

static int
_vk_window_dtor(vk_object_t *object)
{
    vk_window_t     *window;
    vk_frame_t      *frame;
    vk_container_t  *container;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_window_t)) return -1;

    window = VK_WINDOW(object);
    frame = VK_FRAME(object);
    container = VK_CONTAINER(object);

    if(window->title != NULL)
    {
        free(window->title);
        window->title = NULL;
    }

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
_vk_window_update(vk_frame_t *frame)
{
    vk_window_t *window;
    vk_widget_t *widget;

    if(frame == NULL) return -1;

    window = VK_WINDOW(frame);
    widget = VK_WIDGET(frame);

    widget->_erase(widget);
    frame->_draw_border(frame);
    window->_draw_title(window);

    if(window->on_decorate != NULL)
    {
        window->on_decorate(window, widget->canvas, window->decorate_data);
    }

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

static int
_vk_window_draw_title(vk_window_t *window)
{
    vk_frame_t  *frame;
    vk_widget_t *widget;
    short       fg, bg;
    int         border_colors;
    int         title_len;
    int         avail;
    int         col;

    if(window == NULL) return -1;

    frame = VK_FRAME(window);
    widget = VK_WIDGET(window);

    if(window->title == NULL) return 0;
    if(frame->border_style == VK_FRAME_NONE) return 0;

    title_len = strlen(window->title);
    avail = widget->width - 4;

    if(avail <= 0) return 0;
    if(title_len > avail) title_len = avail;

    switch(window->title_justify)
    {
        case VK_JUSTIFY_LEFT:
            col = 2;
            break;

        case VK_JUSTIFY_RIGHT:
            col = widget->width - 2 - title_len;
            break;

        case VK_JUSTIFY_CENTER:
        default:
            col = (widget->width - title_len) / 2;
            break;
    }

    fg = (frame->border_fg == -1) ? widget->fg : frame->border_fg;
    bg = (frame->border_bg == -1) ? widget->bg : frame->border_bg;
    border_colors = VIPER_COLORS(fg, bg);

    wattron(widget->canvas, border_colors | A_BOLD);
    mvwprintw(widget->canvas, 0, col, "%.*s", title_len, window->title);
    wattroff(widget->canvas, border_colors | A_BOLD);

    return 0;
}

static int
_vk_window_recreate(vk_widget_t *widget)
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
