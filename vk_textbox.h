#ifndef _VK_TEXTBOX_H_
#define _VK_TEXTBOX_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_textbox_s
{
    vk_widget_t         parent_klass;

    char                *text;

    char                **lines;
    int                 line_count;
    int                 lines_alloc;

    int                 scroll_top;

    bool                word_wrap;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_textbox_t *);
};

#endif
