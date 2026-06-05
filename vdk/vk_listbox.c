#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"
#include "vk_item.h"
#include "vk_scroller.h"
#include "vk_event.h"

// base klass methods
static int
_vk_listbox_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_listbox_dtor(vk_object_t *object);

// super klass methods
static int
_vk_listbox_add_item(vk_listbox_t *listbox, char *name,
    VkWidgetFunc func, void *anything);

static int
_vk_listbox_set_item(vk_listbox_t *listbox, int idx, char *name,
    VkWidgetFunc func, void *anything);

static int
_vk_listbox_get_item(vk_listbox_t *listbox, int idx, char *buf, int buf_sz);

static int
_vk_listbox_get_item_count(vk_listbox_t *listbox);

static int
_vk_listbox_get_curr(vk_listbox_t *listbox);

static int
_vk_listbox_remove_item(vk_listbox_t *listbox, int idx);

static int
_vk_listbox_exec_item(vk_listbox_t *listbox);

static int
_vk_listbox_add_separator(vk_listbox_t *listbox, int style);

static int
_vk_listbox_update(vk_listbox_t *listbox);

static int
_vk_listbox_on_recreate(vk_object_t *object, int event, void *data);

static int
_vk_listbox_on_resize(vk_object_t *object, int event, void *data);

static int
_vk_listbox_reset(vk_listbox_t *listbox);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_LISTBOX_KLASS)
{
    .size = KLASS_SIZE(vk_listbox_t),
    .name = KLASS_NAME(vk_listbox_t),
    .ctor = _vk_listbox_ctor,
    .dtor = _vk_listbox_dtor,
};

// create a new widget from scratch
inline vk_listbox_t*
vk_listbox_create(int width, int height)
{
    vk_listbox_t    *listbox;

    if(height == 0 || width == 0) return NULL;

    listbox = (vk_listbox_t*)vk_object_create(VK_LISTBOX_KLASS,
        width, height);

    return listbox;
}

inline int
vk_listbox_set_wrap(vk_listbox_t *listbox, bool allowed)
{
    if(listbox == NULL) return -1;

    if(allowed == TRUE)
        listbox->flags |= VK_FLAG_ALLOW_WRAP;
    else
        listbox->flags &= ~VK_FLAG_ALLOW_WRAP;

    return 0;
}

inline int
vk_listbox_set_title(vk_listbox_t *listbox, char *title)
{
    if(listbox == NULL) return -1;
    if(title == NULL) return -1;

    if(listbox->title != NULL)
    {
        free(listbox->title);
        listbox->title = NULL;
    }

    listbox->title = strdup(title);

    return 0;
}

inline int
vk_listbox_get_title(vk_listbox_t *listbox, char *buf, int buf_sz)
{
    if(listbox == NULL) return -1;
    if(buf == NULL) return -1;
    if(buf_sz < 1) return -1;

    if(listbox->title != NULL)
    {
        buf = strndup(listbox->title, buf_sz - 1);
    }

    return 0;
}

inline int
vk_listbox_set_highlight(vk_listbox_t *listbox, int fg, int bg)
{
    if(listbox == NULL) return -1;

    listbox->highlight_fg = fg;
    listbox->highlight_bg = bg;

    return 0;
}

inline int
vk_listbox_add_item(vk_listbox_t *listbox, char *name,
    VkWidgetFunc func, void *anything)
{
    int idx = 0;

    if(listbox == NULL) return -1;
    if(name == NULL) return -1;
    if(name[0] == '\0') return -1;

    idx = listbox->_add_item(listbox, name, func, anything);

    return idx;
}

inline int
vk_listbox_update(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    // make sure the item is actually a listbox widget

    listbox->_update(listbox);

    return 0;
}

inline int
vk_listbox_remove_item(vk_listbox_t *listbox, int idx)
{
    int retval;

    if(listbox == NULL) return -1;
    if(idx < 0) return -1;

    retval = listbox->_remove_item(listbox, idx);

    return retval;
}

inline int
vk_listbox_get_item_count(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    return listbox->_get_item_count(listbox);
}

inline int
vk_listbox_get_scroll_pos(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    return listbox->scroll_top;
}

inline int
vk_listbox_get_curr(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    return listbox->_get_selected(listbox);
}

inline int
vk_listbox_get_item(vk_listbox_t *listbox, int idx, char *buf, int buf_sz)
{
    if(listbox == NULL) return -1;

    return listbox->_get_item(listbox, idx, buf, buf_sz);
}

inline int
vk_listbox_set_item(vk_listbox_t *listbox, int idx, char *name,
    VkWidgetFunc func, void *anything)
{
    if(listbox == NULL) return -1;

    return listbox->_set_item(listbox, idx, name, func, anything);
}

inline int
vk_listbox_set_curr(vk_listbox_t *listbox, int idx)
{
    if(listbox == NULL) return -1;
    if(idx < 0 || idx >= listbox->item_count) return -1;

    listbox->curr_item = idx;

    vk_object_emit(VK_OBJECT(listbox), VK_EVENT_ON_SELECT);

    return 0;
}

inline int
vk_listbox_exec_curr(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    vk_object_emit(VK_OBJECT(listbox), VK_EVENT_ON_ACTIVATE);

    return listbox->_exec_item(listbox);
}

inline int
vk_listbox_set_next(vk_listbox_t *listbox)
{
    int idx;

    if(listbox == NULL) return -1;
    if(listbox->item_count == 0) return -1;

    idx = listbox->curr_item;

    do
    {
        idx++;

        if(idx >= listbox->item_count)
        {
            if(listbox->flags & VK_FLAG_ALLOW_WRAP)
                idx = 0;
            else
                return 0;
        }

        if(idx == listbox->curr_item) return 0;
    }
    while(vk_listbox_item_is_separator(listbox, idx));

    listbox->curr_item = idx;

    vk_object_emit(VK_OBJECT(listbox), VK_EVENT_ON_SELECT);

    return 0;
}

inline int
vk_listbox_set_prev(vk_listbox_t *listbox)
{
    int idx;

    if(listbox == NULL) return -1;
    if(listbox->item_count == 0) return -1;

    idx = listbox->curr_item;

    do
    {
        idx--;

        if(idx < 0)
        {
            if(listbox->flags & VK_FLAG_ALLOW_WRAP)
                idx = listbox->item_count - 1;
            else
                return 0;
        }

        if(idx == listbox->curr_item) return 0;
    }
    while(vk_listbox_item_is_separator(listbox, idx));

    listbox->curr_item = idx;

    vk_object_emit(VK_OBJECT(listbox), VK_EVENT_ON_SELECT);

    return 0;
}

inline bool
vk_listbox_item_is_separator(vk_listbox_t *listbox, int idx)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 i = 0;

    if(listbox == NULL) return false;
    if(idx < 0 || idx >= listbox->item_count) return false;

    list_for_each(pos, &listbox->item_list)
    {
        if(i == idx)
        {
            item = list_entry(pos, vk_item_t, list);
            return (item->separator_style > 0) ? true : false;
        }
        i++;
    }

    return false;
}

inline int
vk_listbox_get_metrics(vk_listbox_t *listbox, int *width, int *height)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 max_len = 0;
    int                 len;

    if(listbox == NULL) return -1;
    if(width == NULL && height == NULL) return -1;

    if(height != NULL) *height = listbox->item_count;

    if(width != NULL)
    {
        if(listbox->item_count == 0)
        {
            *width = 0;
            return 0;
        }

        list_for_each(pos, &listbox->item_list)
        {
            item = list_entry(pos, vk_item_t, list);

            if(item->name != NULL)
            {
                len = strlen(item->name);
                if(len > max_len) max_len = len;
            }
        }
    }

    *width = max_len;

    return 0;
}

inline int
vk_listbox_reset(vk_listbox_t *listbox)
{
    int retval;

    if(listbox == NULL) return -1;

    // make sure the item is actually a listbox widget

    retval = listbox->_reset(listbox);

    return retval;
}

inline int
vk_listbox_add_separator(vk_listbox_t *listbox, int style)
{
    if(listbox == NULL) return -1;

    return listbox->_add_separator(listbox, style);
}

inline void
vk_listbox_destroy(vk_listbox_t *listbox)
{
    if(listbox == NULL) return;

    // make sure the object is actually a listbox
    if(!vk_object_assert(listbox, vk_listbox_t)) return;

    listbox->dtor(VK_OBJECT(listbox));

    return;
}

static int
_vk_listbox_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_listbox_t        *listbox;
    va_list             args;

    if(object == NULL) return -1;

    /*
        if argp is set then we're being called by a superclass.
        otherwise, we're being called directly.
    */
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

    // install our derived klass methods
    listbox = VK_LISTBOX(object);

    listbox->highlight_fg = -1;
    listbox->highlight_bg = -1;

    listbox->ctor = _vk_listbox_ctor;
    listbox->dtor = _vk_listbox_dtor;

    listbox->_add_item = _vk_listbox_add_item;
    listbox->_get_item = _vk_listbox_get_item;
    listbox->_set_item = _vk_listbox_set_item;
    listbox->_get_item_count = _vk_listbox_get_item_count;
    listbox->_get_selected = _vk_listbox_get_curr;
    listbox->_remove_item = _vk_listbox_remove_item;
    listbox->_exec_item = _vk_listbox_exec_item;
    listbox->_add_separator = _vk_listbox_add_separator;
    listbox->_update = _vk_listbox_update;
    listbox->_reset = _vk_listbox_reset;

    vk_object_register_event(VK_OBJECT(listbox),
        VK_EVENT_ON_RECREATE, _vk_listbox_on_recreate, NULL);
    vk_object_register_event(VK_OBJECT(listbox),
        VK_EVENT_ON_RESIZE, _vk_listbox_on_resize, NULL);

    INIT_LIST_HEAD(&listbox->item_list);

    return 0;
}

static int
_vk_listbox_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_listbox_t)) return -1;

    // destroy all the list items
    VK_LISTBOX(object)->_reset(VK_LISTBOX(object));

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_listbox_add_item(vk_listbox_t *listbox, char *name,
    VkWidgetFunc func, void *anything)
{
    vk_item_t   *item;

    if(listbox == NULL) return -1;
    if(name == NULL) return -1;
    if(name[0] == '\0') return -1;

    item = (vk_item_t *)calloc(1, sizeof(vk_item_t));
    item->name = strdup(name);
    item->func = func;
    item->anything = anything;

    list_add_tail(&item->list, &listbox->item_list);
    listbox->item_count++;

    return 0;
}

static int
_vk_listbox_add_separator(vk_listbox_t *listbox, int style)
{
    vk_item_t   *item;

    if(listbox == NULL) return -1;
    if(style < 1) style = VK_SEPARATOR_BLANK;

    item = (vk_item_t *)calloc(1, sizeof(vk_item_t));
    item->separator_style = style;

    list_add_tail(&item->list, &listbox->item_list);
    listbox->item_count++;

    return 0;
}

static int
_vk_listbox_set_item(vk_listbox_t *listbox, int idx, char *name,
    VkWidgetFunc func, void *anything)
{
    vk_item_t           *item = NULL;
    struct list_head    *pos;
    int                 i = 0;

    if(listbox == NULL) return -1;
    if(idx < 0) return -1;
    if(name == NULL) return -1;
    if(name[0] == '\0') return -1;

    list_for_each(pos, &listbox->item_list)
    {
        if(i == idx) break;
        i++;
    }

    // list was shorter than caller expected
    if(i < idx) return -1;

    item = list_entry(pos, vk_item_t, list);

    if(item->name != NULL) free(item->name);
    item->name = strdup(name);
    item->func = func;
    item->anything = anything;

    return 0;
}

static int
_vk_listbox_get_item(vk_listbox_t *listbox, int idx, char *buf, int buf_sz)
{
    vk_item_t           *item = NULL;
    struct list_head    *pos;
    int                 i = 0;

    if(listbox == NULL) return -1;
    if(idx < 0) return -1;
    if(buf == NULL) return -1;
    if(buf_sz < 1) return -1;

    list_for_each(pos, &listbox->item_list)
    {
        if(i == idx) break;
        i++;
    }

    // list was shorter than caller expected
    if(i < idx) return -1;

    item = list_entry(pos, vk_item_t, list);

    memset(buf, 0, buf_sz);
    memcpy(buf, item->name, buf_sz - 1);

    return 0;
}

static int
_vk_listbox_get_item_count(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    return listbox->item_count;
}

static int
_vk_listbox_get_curr(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    return listbox->curr_item;
}

static int
_vk_listbox_remove_item(vk_listbox_t *listbox, int idx)
{
    if(listbox == NULL) return -1;
    if(idx < 0) return -1;
    if(idx >= listbox->item_count) return -1;

    list_del(&listbox->item_list);
    listbox->item_count--;

    return 0;
}

static int
_vk_listbox_exec_item(vk_listbox_t *listbox)
{
    vk_item_t           *item;
    struct list_head    *pos;
    int                 retval;
    int                 i = 0;

    if(listbox == NULL) return -1;

    list_for_each(pos, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(i == listbox->curr_item) break;

        i++;
    }

    if(item->func == NULL) return -1;

    retval = item->func(VK_WIDGET(listbox), item->anything);

    return retval;
}

static int
_vk_listbox_on_recreate(vk_object_t *object, int event, void *data)
{
    vk_widget_t *widget = VK_WIDGET(object);

    (void)event;
    (void)data;

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

    return _vk_listbox_update(VK_LISTBOX(widget));
}

static int
_vk_listbox_on_resize(vk_object_t *object, int event, void *data)
{
    vk_widget_t *widget = VK_WIDGET(object);

    (void)event;
    (void)data;

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

    return _vk_listbox_update(VK_LISTBOX(widget));
}

static int
_vk_listbox_update(vk_listbox_t *listbox)
{
    vk_widget_t         *widget;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_width;
    int                 paint_height;
    int                 paint_colors;
    int                 highlight;
    attr_t              highlight_attr;
    int                 idx = 0;
    int                 x = 0;
    int                 y = 0;

    if(listbox == NULL) return -1;

    widget = VK_WIDGET(listbox);

    widget->_erase(widget);
    paint_width = widget->width;
    paint_height = widget->height;

    if(widget->vscroller != NULL) paint_width--;
    if(widget->hscroller != NULL) paint_height--;

    // if highlight colors net set, use inverted widget colors
    if(listbox->highlight_fg == -1) listbox->highlight_fg = widget->bg;
    if(listbox->highlight_bg == -1) listbox->highlight_bg = widget->fg;

    paint_colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg));
    highlight = COLOR_PAIR(vdk_color_pair(listbox->highlight_fg, listbox->highlight_bg));

    wattron(widget->canvas, paint_colors);

    // if the title exists we need to show it and account for it
    if(listbox->title != NULL)
    {
        mvwaddstr(widget->canvas, 0, (paint_width / 2), listbox->title);
        paint_height--;
        y = 1;
    }

    // set simple bounds when all items will fit in the paint area
    if(listbox->item_count <= paint_height)
    {
        listbox->scroll_top = 0;
    }

    listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);

    // clamp region when items don't fit in paint area
    if(listbox->item_count > paint_height)
    {
        while(listbox->curr_item < listbox->scroll_top)
        {
            listbox->scroll_top--;
            listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);
        }

        while(listbox->curr_item > listbox->scroll_bottom)
        {
            listbox->scroll_top++;
            listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);
        }
    }

    list_for_each(pos, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(idx < listbox->scroll_top)
        {
            idx++;
            continue;
        }

        if(idx > listbox->scroll_bottom) break;

        highlight_attr = A_NORMAL;

        if(item->separator_style > 0)
        {
            if(item->separator_style == VK_SEPARATOR_BLANK)
            {
                mvwprintw(widget->canvas, y, x, "%-*c",
                    paint_width, ' ');
            }

            if(item->separator_style == VK_SEPARATOR_SINGLE)
            {
                mvwhline_set(widget->canvas, y, x, WACS_HLINE, paint_width);
            }
        }
        else
        {
            mvwprintw(widget->canvas, y, x, "%-*.*s",
                paint_width,
                paint_width,
                item->name);
        }

        if(idx == listbox->curr_item)
        {
            mvwchgat(widget->canvas, y, x, paint_width,
                highlight_attr, PAIR_NUMBER(highlight), NULL);
        }

        y++;
        idx++;
    }

    while(y < paint_height)
    {
        mvwprintw(widget->canvas, y, x, "%-*c",
            paint_width, ' ');

        y++;
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

    return 0;
}

static int
_vk_listbox_reset(vk_listbox_t *listbox)
{
    vk_item_t           *item;
    struct list_head    *pos;
    struct list_head    *tmp;

    if(listbox == NULL) return -1;

    list_for_each_safe(pos, tmp, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(item->name != NULL) free(item->name);
        list_del(pos);

        free(item);
    }

    listbox->item_count = 0;
    listbox->curr_item = 0;

    INIT_LIST_HEAD(&listbox->item_list);

    return 0;
}
