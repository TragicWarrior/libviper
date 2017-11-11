#ifndef _VK_CONTAINER_H_
#define _VK_CONTAINER_H_

#include <inttypes.h>
#include <stdarg.h>

#include "list.h"
#include "vk_widget.h"

struct _vk_container_s
{
    vk_widget_t         parent_klass;

    struct list_head    widget_list;    // list of attached widgets

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);

    int                 (*add)          (vk_container_t *, vk_widget_t *);
    int                 (*remove)       (vk_container_t *, vk_widget_t *);
};

#endif


