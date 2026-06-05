#ifndef _VK_BOX_H_
#define _VK_BOX_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"

struct _vk_box_s
{
    vk_container_t      parent_klass;

    int                 orientation;
    int                 slots;
    vk_widget_t         **slot_widgets;
    int                 focused_slot;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_box_t *);
};

#endif
