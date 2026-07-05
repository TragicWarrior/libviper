#ifndef _VK_METER_H_
#define _VK_METER_H_

#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_progress.h"

#define VK_METER_MAX_THRESHOLDS     16

struct _vk_meter_threshold_s
{
    double      at;
    short       fg;
    short       bg;
};

/*
    vk_meter derives from vk_progress and adds value-dependent fill colours.
    It overrides only vk_progress's _fill_color virtual -- all geometry (fill
    fraction, orientation, thickness, relief, trough, sub-cell 1/8 fill) is
    inherited.  The colour thresholds and the fill fraction domain are
    independent: set the fill range with vk_progress_set_range() and the colour
    breakpoints with vk_meter_add_threshold().
*/
struct _vk_meter_s
{
    vk_progress_t       parent_klass;

    struct _vk_meter_threshold_s    thresholds[VK_METER_MAX_THRESHOLDS];
    int                             threshold_count;

    int                 (*ctor)     (vk_object_t *, va_list *, ...);
    int                 (*dtor)     (vk_object_t *);
};

#endif
