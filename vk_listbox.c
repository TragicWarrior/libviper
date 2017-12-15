#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"
#include "vk_item.h"

// base klass methods
static int
_vk_listbox_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_listbox_dtor(vk_object_t *object);

static int
_vk_listbox_kmio(vk_object_t *object, int32_t keystroke);

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
_vk_listbox_get_selected(vk_listbox_t *listbox);

static int
_vk_listbox_remove_item(vk_listbox_t *listbox, int idx);

static int
_vk_listbox_exec_item(vk_listbox_t *listbox);

static int
_vk_listbox_update(vk_listbox_t *listbox);

static int
_vk_listbox_reset(vk_listbox_t *listbox);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_LISTBOX_KLASS)
{
    .size = KLASS_SIZE(vk_listbox_t),
    .name = KLASS_NAME(vk_listbox_t),
    .ctor = _vk_listbox_ctor,
    .dtor = _vk_listbox_dtor,
    .kmio = _vk_listbox_kmio,
};


// create a new widget from scratch
vk_listbox_t*
vk_listbox_create(int width, int height)
{
    vk_listbox_t    *listbox;

    if(height == 0 || width == 0) return NULL;

    listbox = (vk_listbox_t*)vk_object_create(VK_LISTBOX_KLASS,
        width, height);

    return listbox;
}

int
vk_listbox_set_wrap(vk_listbox_t *listbox, bool allowed)
{
    if(listbox == NULL) return -1;

    if(allowed == TRUE)
        listbox->flags |= VK_FLAG_ALLOW_WRAP;
    else
        listbox->flags &= ~VK_FLAG_ALLOW_WRAP;

    return 0;
}

int
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

int
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

int
vk_listbox_set_highlight(vk_listbox_t *listbox, int fg, int bg)
{
    if(listbox == NULL) return -1;

    listbox->highlight_fg = fg;
    listbox->highlight_bg = bg;

    return 0;
}

int
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

int
vk_listbox_update(vk_listbox_t *listbox)
{
    if(listbox == NULL) return -1;

    // make sure the item is actually a listbox widget
    if(!vk_object_assert(listbox, vk_listbox_t)) return -1;

    listbox->_update(listbox);

    return 0;
}

int
vk_listbox_remove_item(vk_listbox_t *listbox, int idx)
{
    int retval;

    if(listbox == NULL) return -1;
    if(idx < 0) return -1;

    retval = listbox->_remove_item(listbox, idx);

    return retval;
}

int
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

int
vk_listbox_reset(vk_listbox_t *listbox)
{
    int retval;

    if(listbox == NULL) return -1;

    // make sure the item is actually a listbox widget
    if(!vk_object_assert(listbox, vk_listbox_t)) return -1;

    retval = listbox->_reset(listbox);

    return retval;
}

void
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
    listbox->_get_selected = _vk_listbox_get_selected;
    listbox->_remove_item = _vk_listbox_remove_item;
    listbox->_exec_item = _vk_listbox_exec_item;
    listbox->_update = _vk_listbox_update;
    listbox->_reset = _vk_listbox_reset;

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
_vk_listbox_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_listbox_t    *listbox;
    int             retval;

    listbox = VK_LISTBOX(object);

    if(list_empty(&listbox->item_list)) return 0;

    switch(keystroke)
    {
        case KEY_UP:    listbox->curr_item--;           break;
        case KEY_DOWN:  listbox->curr_item++;           break;
        case 10:
        {
            retval = listbox->_exec_item(listbox);
            return retval;
        }
    }

    if(listbox->curr_item < 0)
    {
        if(listbox->flags & VK_FLAG_ALLOW_WRAP)
            listbox->curr_item = listbox->item_count - 1;
        else
            listbox->curr_item = 0;
    }

    if(listbox->curr_item > (listbox->item_count - 1))
    {
        if(listbox->flags & VK_FLAG_ALLOW_WRAP)
            listbox->curr_item = 0;
        else
            listbox->curr_item--;
    }

    // now cause the widget to redraw
    listbox->_update(listbox);
    VK_WIDGET(object)->_draw(VK_WIDGET(object));

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
_vk_listbox_get_selected(vk_listbox_t *listbox)
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
_vk_listbox_update(vk_listbox_t *listbox)
{
    vk_widget_t         *widget;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_height;
    int                 paint_colors;
    int                 highlight;
    int                 idx = 0;
    int                 x = 0;
    int                 y = 0;

    if(listbox == NULL) return -1;

    widget = VK_WIDGET(listbox);

    widget->_erase(widget);
    paint_height = widget->height;

    // if highlight colors net set, use inverted widget colors
    if(listbox->highlight_fg == -1) listbox->highlight_fg = widget->bg;
    if(listbox->highlight_bg == -1) listbox->highlight_bg = widget->fg;

    paint_colors = VIPER_COLORS(widget->fg, widget->bg);
    highlight = VIPER_COLORS(listbox->highlight_fg, listbox->highlight_bg);

    wattron(widget->canvas, paint_colors);

    // if the title exists we need to show it and account for it
    if(listbox->title != NULL)
    {
        mvwaddstr(widget->canvas, 0, (widget->width / 2), listbox->title);
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

        mvwprintw(widget->canvas, y, x, "%-*s",
            widget->width,
            item->name);

        if(idx == listbox->curr_item)
        {
            mvwchgat(widget->canvas, y, 0, -1,
                 A_NORMAL, PAIR_NUMBER(highlight), NULL);
        }

        y++;
        idx++;
    }

    while(y < widget->height)
    {
        mvwprintw(widget->canvas, y, x, "%-*c",
            widget->width, ' ');

        y++;
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
