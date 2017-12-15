#ifndef _VK_ITEM_H_
#define _VK_ITEM_H_

#include "list.h"
#include "viper.h"


struct _vk_item_s
{
    struct list_head    list;

    char                *name;              // display name of item

    int                 separator_style;    // divider to use on a menu

    VkWidgetFunc        func;               // run when item is activated
    void                *anything;          // passed in as argument for func()
};

typedef struct _vk_item_s   vk_item_t;

#endif
