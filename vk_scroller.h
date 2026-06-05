#ifndef _VK_SCROLLER_H_
#define _VK_SCROLLER_H_

#include <inttypes.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"

struct _vk_scroller_s
{
    vk_frame_t          parent_klass;

    VkScrollInfoFunc    scroll_info_func;

    int                 content_height;
    int                 content_width;
    int                 scroll_y;
    int                 scroll_x;

    int                 scrollbar_flags;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_draw_scrollbar)  (vk_scroller_t *);
};

#endif
