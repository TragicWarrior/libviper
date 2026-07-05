#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_progress.h"
#include "vk_meter.h"

static int      _vk_meter_ctor(vk_object_t *object, va_list *argp, ...);
static int      _vk_meter_dtor(vk_object_t *object);
static void     _vk_meter_fill_color(vk_progress_t *progress,
                    short *fg, short *bg);

require_klass(VK_PROGRESS_KLASS);

declare_klass(VK_METER_KLASS)
{
    .size = KLASS_SIZE(vk_meter_t),
    .name = KLASS_NAME(vk_meter_t),
    .ctor = _vk_meter_ctor,
    .dtor = _vk_meter_dtor,
};

vk_meter_t*
vk_meter_create(int orientation, int length, int thickness)
{
    vk_meter_t  *meter;
    int         w, h;

    if(length < 1) length = 1;
    if(thickness < 1) thickness = 1;

    if(orientation == VK_PROGRESS_VERTICAL)
    {
        w = thickness;
        h = length;
    }
    else
    {
        w = length;
        h = thickness;
    }

    meter = (vk_meter_t *)vk_object_create(VK_METER_KLASS, w, h,
        orientation, thickness);

    return meter;
}

static int
_vk_meter_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_meter_t  *meter;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* the progress base consumes width, height, orientation, thickness */
    VK_PROGRESS_KLASS->ctor(object, argp);

    meter = VK_METER(object);

    meter->threshold_count = 0;

    va_end(args);

    meter->ctor = _vk_meter_ctor;
    meter->dtor = _vk_meter_dtor;

    /* override just the fill-colour selection; geometry stays vk_progress's */
    VK_PROGRESS(meter)->_fill_color = _vk_meter_fill_color;

    return 0;
}

static int
_vk_meter_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_meter_t)) return -1;

    vk_object_demote(object, vk_progress_t);
    VK_PROGRESS(object)->dtor(object);

    return 0;
}

/*
    Colour lookup: the highest threshold whose `at` <= the current value wins.
    A value below every threshold takes the lowest threshold's colour.  With no
    thresholds set, fall back to the base fill colour.
*/
static void
_vk_meter_fill_color(vk_progress_t *progress, short *fg, short *bg)
{
    vk_meter_t  *meter = VK_METER(progress);
    int         i, best;

    if(meter->threshold_count == 0)
    {
        *fg = progress->fill_fg;
        *bg = progress->fill_bg;
        return;
    }

    /* thresholds are kept sorted ascending by `at`; the first (from the top)
       whose `at` <= value is the winner.  If none match, best stays 0 -- the
       lowest band, which also covers values below thresholds[0].at. */
    best = 0;
    for(i = meter->threshold_count - 1; i >= 0; i--)
    {
        if(meter->thresholds[i].at <= progress->value)
        {
            best = i;
            break;
        }
    }

    *fg = meter->thresholds[best].fg;
    *bg = meter->thresholds[best].bg;
}

int
vk_meter_add_threshold(vk_meter_t *meter, double at, short fg, short bg)
{
    int i;

    if(meter == NULL) return -1;
    if(meter->threshold_count >= VK_METER_MAX_THRESHOLDS) return -1;

    /* insertion sort by `at`, ascending */
    i = meter->threshold_count;
    while(i > 0 && meter->thresholds[i - 1].at > at)
    {
        meter->thresholds[i] = meter->thresholds[i - 1];
        i--;
    }

    meter->thresholds[i].at = at;
    meter->thresholds[i].fg = fg;
    meter->thresholds[i].bg = bg;
    meter->threshold_count++;

    return 0;
}

int
vk_meter_clear_thresholds(vk_meter_t *meter)
{
    if(meter == NULL) return -1;

    meter->threshold_count = 0;

    return 0;
}

void
vk_meter_destroy(vk_meter_t *meter)
{
    if(meter == NULL) return;

    vk_object_destroy(VK_OBJECT(meter));
}
