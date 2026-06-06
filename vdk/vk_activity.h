#ifndef _VK_ACTIVITY_H_
#define _VK_ACTIVITY_H_

#include <stdarg.h>
#include <stdbool.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_activity_s
{
    vk_widget_t         parent_klass;

    int                 style;
    int                 speed;
    int                 frame;
    int                 tick_count;
    bool                running;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
