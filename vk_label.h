#ifndef _VK_LABEL_H_
#define _VK_LABEL_H_

#include <inttypes.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_label_s
{
    vk_widget_t         parent_klass;

    char                *text;
    int                 justify;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_label_t *);
};

#endif
