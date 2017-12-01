#ifndef _VK_CONTAINER_H_
#define _VK_CONTAINER_H_

#include <inttypes.h>
#include <stdarg.h>

#include "list.h"

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_container_s
{
    vk_widget_t         parent_klass;

    struct list_head    widget_list;        // list of attached widgets

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*add_widget)       (vk_container_t *, vk_widget_t *);
    int                 (*remove_widget)    (vk_container_t *, vk_widget_t *);
    int                 (*vacate)           (vk_container_t *);
    int                 (*rotate)           (vk_container_t *, int);
};

#endif


