#ifndef _VK_WINDOW_H_
#define _VK_WINDOW_H_

#include <inttypes.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"

struct _vk_window_s
{
    vk_frame_t          parent_klass;

    char                *title;
    int                 title_justify;

    VkWindowDecorateFunc    on_decorate;
    void                    *decorate_data;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_draw_title)      (vk_window_t *);
};

#endif
