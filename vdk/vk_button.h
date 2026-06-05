#ifndef _VK_BUTTON_H_
#define _VK_BUTTON_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_button_s
{
    vk_widget_t         parent_klass;

    char                *text;
    int                 relief_style;
    bool                pressed;

    short               pressed_fg;
    short               pressed_bg;

    VkWidgetFunc        on_press;
    void                *anything;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_button_t *);
};

#endif
