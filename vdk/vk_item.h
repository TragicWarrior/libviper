#ifndef _VK_ITEM_H_
#define _VK_ITEM_H_

#include "list.h"
#include "vdk.h"


struct _vk_item_s
{
    struct list_head    list;

    char                *name;              // display name of item

    int                 separator_style;    // divider to use on a menu
    unsigned int        flags;

    VkWidgetFunc        func;               // run when item is activated
    void                *anything;          // passed in as argument for func()

    /* per-item colors (vk_listbox_set_item_colors); has_colors == 0 means
       the row uses the widget's own colors */
    int                 has_colors;
    int                 fg;
    int                 bg;
    attr_t              attrs;

    /* the row opens a submenu: the listbox draws its submenu marker at
       the row's right edge (vk_listbox_set_item_submenu) */
    int                 submenu;
};

typedef struct _vk_item_s   vk_item_t;

#endif
