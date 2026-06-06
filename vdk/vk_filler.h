#ifndef _VK_FILLER_H_
#define _VK_FILLER_H_

#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_filler_s
{
    vk_widget_t         parent_klass;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
