#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_menubar.h"
#include "vk_item.h"
#include "vk_event.h"

static int
_vk_menubar_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_menubar_dtor(vk_object_t *object);

static int
_vk_menubar_add_item(vk_menubar_t *menubar, char *name,
    VkWidgetFunc func, void *anything);

static int
_vk_menubar_get_item_count(vk_menubar_t *menubar);

static int
_vk_menubar_exec_item(vk_menubar_t *menubar);

static int
_vk_menubar_update(vk_menubar_t *menubar);

static int
_vk_menubar_on_recreate(vk_object_t *object, int event, void *data);

static int
_vk_menubar_on_resize(vk_object_t *object, int event, void *data);

static int
_vk_menubar_reset(vk_menubar_t *menubar);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_MENUBAR_KLASS)
{
    .size = KLASS_SIZE(vk_menubar_t),
    .name = KLASS_NAME(vk_menubar_t),
    .ctor = _vk_menubar_ctor,
    .dtor = _vk_menubar_dtor,
};

inline vk_menubar_t*
vk_menubar_create(int width)
{
    vk_menubar_t    *menubar;

    if(width < 1) return NULL;

    menubar = (vk_menubar_t *)vk_object_create(VK_MENUBAR_KLASS,
        width, 1);

    return menubar;
}

inline int
vk_menubar_add_item(vk_menubar_t *menubar, char *name,
    VkWidgetFunc func, void *anything)
{
    if(menubar == NULL) return -1;
    if(name == NULL) return -1;
    if(name[0] == '\0') return -1;

    return menubar->_add_item(menubar, name, func, anything);
}

inline int
vk_menubar_get_item_count(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    return menubar->_get_item_count(menubar);
}

inline int
vk_menubar_get_curr(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    return menubar->curr_item;
}

inline int
vk_menubar_set_curr(vk_menubar_t *menubar, int idx)
{
    if(menubar == NULL) return -1;
    if(idx < 0 || idx >= menubar->item_count) return -1;

    menubar->curr_item = idx;

    vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_SELECT);

    return 0;
}

inline int
vk_menubar_set_next(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;
    if(menubar->item_count == 0) return -1;

    menubar->curr_item++;

    if(menubar->curr_item >= menubar->item_count)
        menubar->curr_item = 0;

    vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_SELECT);

    return 0;
}

inline int
vk_menubar_set_prev(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;
    if(menubar->item_count == 0) return -1;

    menubar->curr_item--;

    if(menubar->curr_item < 0)
        menubar->curr_item = menubar->item_count - 1;

    vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_SELECT);

    return 0;
}

inline int
vk_menubar_exec_curr(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_ACTIVATE);

    return menubar->_exec_item(menubar);
}

inline int
vk_menubar_set_highlight(vk_menubar_t *menubar, int fg, int bg)
{
    if(menubar == NULL) return -1;

    menubar->highlight_fg = fg;
    menubar->highlight_bg = bg;

    return 0;
}

inline int
vk_menubar_set_focused(vk_menubar_t *menubar, bool focused)
{
    if(menubar == NULL) return -1;

    menubar->focused = focused;

    if(focused)
        vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_FOCUS);
    else
        vk_object_emit(VK_OBJECT(menubar), VK_EVENT_ON_UNFOCUS);

    return 0;
}

inline bool
vk_menubar_get_focused(vk_menubar_t *menubar)
{
    if(menubar == NULL) return false;

    return menubar->focused;
}

inline int
vk_menubar_hit_test(vk_menubar_t *menubar, int x)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 col = 0;
    int                 idx = 0;

    if(menubar == NULL) return -1;
    if(x < 0) return -1;

    list_for_each(pos, &menubar->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(idx > 0) col++;

        int item_width = strlen(item->name) + 2;

        if(x >= col && x < col + item_width)
            return idx;

        col += item_width;
        idx++;
    }

    return -1;
}

inline int
vk_menubar_get_item_position(vk_menubar_t *menubar, int idx, int *x)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 col = 0;
    int                 i = 0;

    if(menubar == NULL) return -1;
    if(idx < 0 || idx >= menubar->item_count) return -1;
    if(x == NULL) return -1;

    list_for_each(pos, &menubar->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(i > 0) col++;

        if(i == idx)
        {
            *x = col;
            return 0;
        }

        col += strlen(item->name) + 2;
        i++;
    }

    return -1;
}

inline int
vk_menubar_update(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    menubar->_update(menubar);

    return 0;
}

inline int
vk_menubar_reset(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    return menubar->_reset(menubar);
}

inline void
vk_menubar_destroy(vk_menubar_t *menubar)
{
    if(menubar == NULL) return;

    if(!vk_object_assert(menubar, vk_menubar_t)) return;

    menubar->dtor(VK_OBJECT(menubar));

    return;
}

static int
_vk_menubar_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_menubar_t        *menubar;
    va_list             args;

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

    menubar = VK_MENUBAR(object);

    menubar->highlight_fg = -1;
    menubar->highlight_bg = -1;
    menubar->focused = false;

    menubar->ctor = _vk_menubar_ctor;
    menubar->dtor = _vk_menubar_dtor;

    menubar->_add_item = _vk_menubar_add_item;
    menubar->_get_item_count = _vk_menubar_get_item_count;
    menubar->_exec_item = _vk_menubar_exec_item;
    menubar->_update = _vk_menubar_update;
    menubar->_reset = _vk_menubar_reset;

    vk_object_register_event(VK_OBJECT(menubar),
        VK_EVENT_ON_RECREATE, _vk_menubar_on_recreate, NULL);
    vk_object_register_event(VK_OBJECT(menubar),
        VK_EVENT_ON_RESIZE, _vk_menubar_on_resize, NULL);

    INIT_LIST_HEAD(&menubar->item_list);

    return 0;
}

static int
_vk_menubar_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_menubar_t)) return -1;

    VK_MENUBAR(object)->_reset(VK_MENUBAR(object));

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_menubar_add_item(vk_menubar_t *menubar, char *name,
    VkWidgetFunc func, void *anything)
{
    vk_item_t   *item;

    if(menubar == NULL) return -1;
    if(name == NULL) return -1;
    if(name[0] == '\0') return -1;

    item = (vk_item_t *)calloc(1, sizeof(vk_item_t));
    item->name = strdup(name);
    item->func = func;
    item->anything = anything;

    list_add_tail(&item->list, &menubar->item_list);
    menubar->item_count++;

    return 0;
}

static int
_vk_menubar_get_item_count(vk_menubar_t *menubar)
{
    if(menubar == NULL) return -1;

    return menubar->item_count;
}

static int
_vk_menubar_exec_item(vk_menubar_t *menubar)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 i = 0;

    if(menubar == NULL) return -1;

    list_for_each(pos, &menubar->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(i == menubar->curr_item) break;

        i++;
    }

    if(item->func == NULL) return -1;

    return item->func(VK_WIDGET(menubar), item->anything);
}

static int
_vk_menubar_on_recreate(vk_object_t *object, int event, void *data)
{
    (void)event;
    (void)data;

    return _vk_menubar_update(VK_MENUBAR(object));
}

static int
_vk_menubar_on_resize(vk_object_t *object, int event, void *data)
{
    (void)event;
    (void)data;

    return _vk_menubar_update(VK_MENUBAR(object));
}

static int
_vk_menubar_update(vk_menubar_t *menubar)
{
    vk_widget_t         *widget;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_colors;
    int                 highlight;
    int                 col = 0;
    int                 idx = 0;

    if(menubar == NULL) return -1;

    widget = VK_WIDGET(menubar);

    if(menubar->highlight_fg == -1) menubar->highlight_fg = widget->bg;
    if(menubar->highlight_bg == -1) menubar->highlight_bg = widget->fg;

    paint_colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg));
    highlight = COLOR_PAIR(vdk_color_pair(
        menubar->highlight_fg, menubar->highlight_bg));

    wattron(widget->canvas, paint_colors);
    vk_widget_fill(widget, ' ');

    list_for_each(pos, &menubar->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(idx > 0)
        {
            mvwadd_wch(widget->canvas, 0, col, WACS_VLINE);
            col++;
        }

        int item_width = strlen(item->name) + 2;

        mvwprintw(widget->canvas, 0, col, " %s ", item->name);

        if(menubar->focused && idx == menubar->curr_item)
        {
            mvwchgat(widget->canvas, 0, col, item_width,
                A_NORMAL, PAIR_NUMBER(highlight), NULL);
        }

        col += item_width;
        idx++;
    }

    return 0;
}

static int
_vk_menubar_reset(vk_menubar_t *menubar)
{
    vk_item_t           *item;
    struct list_head    *pos;
    struct list_head    *tmp;

    if(menubar == NULL) return -1;

    list_for_each_safe(pos, tmp, &menubar->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(item->name != NULL) free(item->name);
        list_del(pos);

        free(item);
    }

    menubar->item_count = 0;
    menubar->curr_item = 0;

    INIT_LIST_HEAD(&menubar->item_list);

    return 0;
}
