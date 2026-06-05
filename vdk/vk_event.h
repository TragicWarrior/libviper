#ifndef _VK_EVENT_H_
#define _VK_EVENT_H_

#include "list.h"
#include "vdk.h"

struct vk_event_handler
{
    struct list_head    list;
    int                 event;
    VkEventFunc         func;
    void                *anything;
};

int     vk_object_emit(vk_object_t *object, int event);

#endif
