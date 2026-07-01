#ifndef _VK_SCROLLER_H_
#define _VK_SCROLLER_H_

#include <inttypes.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_scroller_s
{
    vk_widget_t         parent_klass;

    vk_widget_t         *host;
    vk_widget_t         *scroll_source;

    VkScrollInfoFunc    scroll_info_func;

    int                 content_height;
    int                 content_width;
    int                 scroll_y;
    int                 scroll_x;

    int                 scrollbar_flags;

    int                 border_style;
    short               border_fg;
    short               border_bg;

    int                 always_visible;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_scroller_t *);
    int                 (*_draw_scrollbar)  (vk_scroller_t *);
};

#endif
