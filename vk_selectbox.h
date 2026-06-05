#ifndef _VK_SELECTBOX_H_
#define _VK_SELECTBOX_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"

struct _vk_selectbox_s
{
    vk_listbox_t        parent_klass;

    int                 mode;
    int                 style;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_selectbox_t *);
};

#endif
