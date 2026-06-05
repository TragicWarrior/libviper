#ifndef _VK_FRAME_H_
#define _VK_FRAME_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"

struct _vk_frame_s
{
    vk_container_t      parent_klass;

    int                 border_style;
    short               border_fg;
    short               border_bg;

    vk_widget_t         *child;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_set_border_style)(vk_frame_t *, int);
    int                 (*_set_child)       (vk_frame_t *, vk_widget_t *);
    int                 (*_draw_border)     (vk_frame_t *);
    int                 (*_update)          (vk_frame_t *);
};

#endif
