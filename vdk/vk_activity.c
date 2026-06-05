#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_activity.h"

static int
_vk_activity_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_activity_dtor(vk_object_t *object);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_ACTIVITY_KLASS)
{
    .size = KLASS_SIZE(vk_activity_t),
    .name = KLASS_NAME(vk_activity_t),
    .ctor = _vk_activity_ctor,
    .dtor = _vk_activity_dtor,
};

static const char *spinner_frames[] =
{
    "|", "/", "-", "\\",
};
static const int spinner_count = 4;

static const wchar_t dots_frames[] =
{
    0x280B, 0x2819, 0x2839, 0x2838,
    0x283C, 0x2834, 0x2826, 0x2827,
    0x2807, 0x280F,
};
static const int dots_count = 10;

static const wchar_t circles_frames[] =
{
    0x25D0, 0x25D3, 0x25D1, 0x25D2,
};
static const int circles_count = 4;

static const wchar_t bar_frames[] =
{
    0x2581, 0x2582, 0x2583, 0x2584,
    0x2585, 0x2586, 0x2587, 0x2588,
};
static const int bar_count = 8;

inline vk_activity_t*
vk_activity_create(void)
{
    vk_activity_t   *activity;

    activity = (vk_activity_t*)vk_object_create(VK_ACTIVITY_KLASS, 1, 1);

    return activity;
}

inline int
vk_activity_set_style(vk_activity_t *activity, int style)
{
    if(activity == NULL) return -1;

    if(style != VK_ACTIVITY_SPINNER && style != VK_ACTIVITY_DOTS
        && style != VK_ACTIVITY_CIRCLES && style != VK_ACTIVITY_BAR)
        return -1;

    activity->style = style;
    activity->frame = 0;
    activity->tick_count = 0;

    return 0;
}

inline int
vk_activity_get_style(vk_activity_t *activity)
{
    if(activity == NULL) return -1;

    return activity->style;
}

inline int
vk_activity_set_speed(vk_activity_t *activity, int interval)
{
    if(activity == NULL) return -1;

    if(interval < 1) return -1;

    activity->speed = interval;

    return 0;
}

inline int
vk_activity_start(vk_activity_t *activity)
{
    if(activity == NULL) return -1;

    activity->running = true;
    activity->frame = 0;
    activity->tick_count = 0;

    return 0;
}

inline int
vk_activity_stop(vk_activity_t *activity)
{
    if(activity == NULL) return -1;

    activity->running = false;

    return 0;
}

inline bool
vk_activity_is_running(vk_activity_t *activity)
{
    if(activity == NULL) return false;

    return activity->running;
}

inline int
vk_activity_run(vk_activity_t *activity)
{
    vk_widget_t     *widget;
    int             colors;
    int             frame_count;

    if(activity == NULL) return -1;

    widget = VK_WIDGET(activity);

    widget->_erase(widget);

    colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg)) | widget->attrs;

    if(!activity->running)
    {
        vk_widget_fill(widget, ' ' | colors);
        return 0;
    }

    activity->tick_count++;

    if(activity->tick_count >= activity->speed)
    {
        activity->tick_count = 0;

        switch(activity->style)
        {
            case VK_ACTIVITY_SPINNER:   frame_count = spinner_count;    break;
            case VK_ACTIVITY_DOTS:      frame_count = dots_count;       break;
            case VK_ACTIVITY_CIRCLES:   frame_count = circles_count;    break;
            case VK_ACTIVITY_BAR:       frame_count = bar_count;        break;
            default:                    frame_count = spinner_count;     break;
        }

        activity->frame = (activity->frame + 1) % frame_count;
    }

    wattron(widget->canvas, colors);

    if(activity->style == VK_ACTIVITY_SPINNER)
    {
        mvwprintw(widget->canvas, 0, 0, "%s",
            spinner_frames[activity->frame]);
    }
    else
    {
        const wchar_t   *frames;
        cchar_t         wch;
        wchar_t         wstr[2];

        switch(activity->style)
        {
            case VK_ACTIVITY_DOTS:      frames = dots_frames;       break;
            case VK_ACTIVITY_CIRCLES:   frames = circles_frames;    break;
            case VK_ACTIVITY_BAR:       frames = bar_frames;        break;
            default:                    frames = dots_frames;        break;
        }

        wstr[0] = frames[activity->frame];
        wstr[1] = 0;

        setcchar(&wch, wstr, colors, 0, NULL);
        mvwadd_wch(widget->canvas, 0, 0, &wch);
    }

    wattroff(widget->canvas, colors);

    return 0;
}

inline void
vk_activity_destroy(vk_activity_t *activity)
{
    if(activity == NULL) return;

    if(!vk_object_assert(activity, vk_activity_t)) return;

    activity->dtor(VK_OBJECT(activity));
}

static int
_vk_activity_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_activity_t   *activity;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    activity = VK_ACTIVITY(object);

    activity->style = VK_ACTIVITY_SPINNER;
    activity->speed = 1;
    activity->frame = 0;
    activity->tick_count = 0;
    activity->running = false;

    activity->ctor = _vk_activity_ctor;
    activity->dtor = _vk_activity_dtor;

    return 0;
}

static int
_vk_activity_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_activity_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}
