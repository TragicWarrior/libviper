#ifndef _VK_INPUT_H_
#define _VK_INPUT_H_

#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_input_s
{
    vk_widget_t         parent_klass;

    char                *text;
    int                 text_len;
    int                 capacity;
    int                 max_len;
    int                 cursor;
    int                 scroll;
    int                 relief_style;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_input_t *);
};

#endif
