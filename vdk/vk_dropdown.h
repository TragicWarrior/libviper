#ifndef _VK_DROPDOWN_H_
#define _VK_DROPDOWN_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"

struct _vk_dropdown_s
{
    vk_listbox_t        parent_klass;

    bool                expanded;
    int                 collapsed_height;
    int                 max_visible;
    int                 relief_style;

    vk_window_t         *popup;
    vk_listbox_t        *popup_listbox;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_dropdown_t *);
};

#endif
