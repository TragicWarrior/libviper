#ifndef _VK_LISTBOX_H_
#define _VK_LISTBOX_H_

#include <inttypes.h>
#include <stdarg.h>

#include "list.h"

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_item.h"

struct _vk_listbox_s
{
    vk_widget_t         parent_klass;
    struct list_head    item_list;          // list of listbox items

    int                 item_count;
    int                 curr_item;

    int                 scroll_top;         // first item to render
    int                 scroll_bottom;      // last item to render

    char                *title;
    unsigned int        flags;

    int                 highlight_fg;       // fg color of selected item
    int                 highlight_bg;       // bg color of selected item

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_add_item)        (vk_listbox_t *, char *,
                                                VkWidgetFunc, void *);
    int                 (*_set_item)        (vk_listbox_t *, int, char *,
                                                VkWidgetFunc, void *);
    int                 (*_remove_item)     (vk_listbox_t *, int);
    int                 (*_get_item)        (vk_listbox_t *, int, char *, int);
    int                 (*_get_item_count)  (vk_listbox_t *);
    int                 (*_get_selected)    (vk_listbox_t *);

    int                 (*_exec_item)       (vk_listbox_t *);

    int                 (*_update)          (vk_listbox_t *);
    int                 (*_reset)           (vk_listbox_t *);
};

#endif


