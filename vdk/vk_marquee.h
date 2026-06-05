#ifndef _VK_MARQUEE_H_
#define _VK_MARQUEE_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_label.h"

struct _vk_marquee_s
{
    vk_label_t          parent_klass;

    int                 direction;
    int                 scroll_interval;
    int                 pause_duration;
    bool                repeat;

    int                 scroll_offset;
    int                 tick_count;
    bool                paused;
    bool                stopped;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
