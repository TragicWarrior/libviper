#ifndef _VK_MENUBAR_H_
#define _VK_MENUBAR_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "list.h"

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_item.h"

struct _vk_menubar_s
{
    vk_widget_t         parent_klass;
    struct list_head    item_list;

    int                 item_count;
    int                 curr_item;

    bool                focused;

    int                 highlight_fg;
    int                 highlight_bg;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_add_item)        (vk_menubar_t *, char *,
                                                VkWidgetFunc, void *);
    int                 (*_get_item_count)  (vk_menubar_t *);
    int                 (*_exec_item)       (vk_menubar_t *);
    int                 (*_update)          (vk_menubar_t *);
    int                 (*_reset)           (vk_menubar_t *);
};

#endif
