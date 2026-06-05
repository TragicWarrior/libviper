#ifndef _VK_CALENDAR_H_
#define _VK_CALENDAR_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_calendar_s
{
    vk_widget_t         parent_klass;

    int                 month;              // 0-11
    int                 year;

    int                 today_day;          // day of month for "today"
    int                 today_month;
    int                 today_year;

    short               highlight_fg;
    short               highlight_bg;
    attr_t              highlight_attrs;

    short               dimmed_fg;
    short               dimmed_bg;
    attr_t              dimmed_attrs;

    short               header_fg;
    short               header_bg;
    attr_t              header_attrs;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_update)          (vk_calendar_t *);
};

#endif
