#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_deck.h"
#include "vdk_color.h"

#define VK_SHADOW_PAIR      vdk_color_pair(COLOR_WHITE, COLOR_BLACK)

static int
_vk_deck_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_deck_dtor(vk_object_t *object);

static int
_vk_deck_draw(vk_widget_t *widget);

static int
_vk_deck_erase(vk_widget_t *widget);

static int
_vk_deck_resize(vk_widget_t *widget, int width, int height);

static int
_vk_deck_recreate(vk_widget_t *widget);

static int
_vk_deck_update(vk_deck_t *deck);

static void
_vk_deck_draw_shadow(vk_widget_t *child, WINDOW *surface);


require_klass(VK_WIDGET_KLASS);

declare_klass(VK_DECK_KLASS)
{
    .size = KLASS_SIZE(vk_deck_t),
    .name = KLASS_NAME(vk_deck_t),
    .ctor = _vk_deck_ctor,
    .dtor = _vk_deck_dtor,
};


inline vk_deck_t*
vk_deck_create(void)
{
    vk_deck_t   *deck;

    deck = (vk_deck_t *)vk_object_create(VK_DECK_KLASS, 1, 1);

    return deck;
}

inline int
vk_deck_add_widget(vk_deck_t *deck, vk_widget_t *widget, int position)
{
    if(deck == NULL) return -1;
    if(widget == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    if(position == VK_DECK_BOTTOM)
        list_add_tail(&widget->list, &deck->widget_list);
    else
        list_add(&widget->list, &deck->widget_list);

    return 0;
}

inline int
vk_deck_remove_widget(vk_deck_t *deck, vk_widget_t *widget)
{
    if(deck == NULL) return -1;
    if(widget == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    list_del(&widget->list);
    widget->surface = NULL;

    return 0;
}

inline int
vk_deck_set_top(vk_deck_t *deck, vk_widget_t *widget)
{
    if(deck == NULL) return -1;
    if(widget == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    list_move(&widget->list, &deck->widget_list);

    return 0;
}

inline vk_widget_t*
vk_deck_get_top(vk_deck_t *deck)
{
    if(deck == NULL) return NULL;

    if(!vk_object_assert(deck, vk_deck_t)) return NULL;

    if(list_empty(&deck->widget_list)) return NULL;

    return list_first_entry(&deck->widget_list, vk_widget_t, list);
}

inline int
vk_deck_cycle(vk_deck_t *deck, int vector)
{
    if(deck == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    if(list_empty(&deck->widget_list)) return 0;

    if(vector == VK_VECTOR_LEFT)
        list_rotate_left(&deck->widget_list);
    else
        list_rotate_right(&deck->widget_list);

    return 0;
}

inline int
vk_deck_set_shadow(vk_deck_t *deck, bool enabled)
{
    if(deck == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    deck->shadows = enabled;

    return 0;
}

inline int
vk_deck_update(vk_deck_t *deck)
{
    if(deck == NULL) return -1;

    if(!vk_object_assert(deck, vk_deck_t)) return -1;

    return deck->_update(deck);
}

inline void
vk_deck_destroy(vk_deck_t *deck)
{
    if(deck == NULL) return;

    if(!vk_object_assert(deck, vk_deck_t)) return;

    deck->dtor(VK_OBJECT(deck));
}


static int
_vk_deck_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_deck_t   *deck;
    vk_widget_t *widget;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    deck = VK_DECK(object);
    widget = VK_WIDGET(deck);

    delwin(widget->canvas);
    widget->canvas = NULL;
    widget->composer = NULL;

    INIT_LIST_HEAD(&deck->widget_list);
    deck->shadows = false;

    deck->ctor = _vk_deck_ctor;
    deck->dtor = _vk_deck_dtor;
    deck->_update = _vk_deck_update;

    widget->_draw = _vk_deck_draw;
    widget->_erase = _vk_deck_erase;
    widget->_resize = _vk_deck_resize;
    widget->_recreate = _vk_deck_recreate;

    return 0;
}

static int
_vk_deck_dtor(vk_object_t *object)
{
    vk_deck_t           *deck;
    struct list_head    *pos;
    struct list_head    *tmp;
    vk_widget_t         *child;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_deck_t)) return -1;

    deck = VK_DECK(object);

    list_for_each_safe(pos, tmp, &deck->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);
        list_del(&child->list);
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_deck_draw(vk_widget_t *widget)
{
    vk_deck_t           *deck;
    struct list_head    *pos;
    vk_widget_t         *child;

    if(widget == NULL) return -1;
    if(widget->surface == NULL) return -1;

    deck = VK_DECK(widget);

    list_for_each_prev(pos, &deck->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);
        child->surface = widget->surface;

        if(deck->shadows)
            _vk_deck_draw_shadow(child, widget->surface);

        vk_widget_draw(child);
    }

    return 0;
}

static int
_vk_deck_erase(vk_widget_t *widget)
{
    (void)widget;
    return 0;
}

static int
_vk_deck_resize(vk_widget_t *widget, int width, int height)
{
    (void)widget;
    (void)width;
    (void)height;
    return 0;
}

static int
_vk_deck_recreate(vk_widget_t *widget)
{
    vk_deck_t           *deck;
    struct list_head    *pos;
    vk_widget_t         *child;

    deck = VK_DECK(widget);

    list_for_each(pos, &deck->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);
        vk_widget_recreate(child);
    }

    return 0;
}

static void
_vk_deck_draw_shadow(vk_widget_t *child, WINDOW *surface)
{
    int         sx, sy;
    int         max_y, max_x;
    cchar_t     cc;
    wchar_t     wch[CCHARW_MAX];
    attr_t      attrs;
    short       color;

    getmaxyx(surface, max_y, max_x);

    // right edge: 1 column at (x + width), rows [y+1 .. y+height]
    sx = child->x + child->width;
    if(sx < max_x)
    {
        for(sy = child->y + 1; sy <= child->y + child->height; sy++)
        {
            if(sy < 0 || sy >= max_y) continue;

            mvwin_wch(surface, sy, sx, &cc);
            getcchar(&cc, wch, &attrs, &color, NULL);
            setcchar(&cc, wch, A_NORMAL, VK_SHADOW_PAIR, NULL);
            mvwadd_wch(surface, sy, sx, &cc);
        }
    }

    // bottom edge: 1 row at (y + height), cols [x+1 .. x+width-1]
    sy = child->y + child->height;
    if(sy < max_y)
    {
        for(sx = child->x + 1; sx < child->x + child->width; sx++)
        {
            if(sx < 0 || sx >= max_x) continue;

            mvwin_wch(surface, sy, sx, &cc);
            getcchar(&cc, wch, &attrs, &color, NULL);
            setcchar(&cc, wch, A_NORMAL, VK_SHADOW_PAIR, NULL);
            mvwadd_wch(surface, sy, sx, &cc);
        }
    }
}

static int
_vk_deck_update(vk_deck_t *deck)
{
    vk_widget_t *widget;

    if(deck == NULL) return -1;

    widget = VK_WIDGET(deck);

    return widget->_draw(widget);
}
