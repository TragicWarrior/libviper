#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_label.h"
#include "vk_marquee.h"

static int
_vk_marquee_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_marquee_dtor(vk_object_t *object);

static int
_vk_marquee_update(vk_label_t *label);


require_klass(VK_LABEL_KLASS);

declare_klass(VK_MARQUEE_KLASS)
{
    .size = KLASS_SIZE(vk_marquee_t),
    .name = KLASS_NAME(vk_marquee_t),
    .ctor = _vk_marquee_ctor,
    .dtor = _vk_marquee_dtor,
};


inline vk_marquee_t*
vk_marquee_create(int width)
{
    vk_marquee_t    *marquee;

    if(width < 1) return NULL;

    marquee = (vk_marquee_t*)vk_object_create(VK_MARQUEE_KLASS, width, 1);

    return marquee;
}

inline int
vk_marquee_set_text(vk_marquee_t *marquee, const char *text)
{
    vk_label_t  *label;

    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    label = VK_LABEL(marquee);

    if(label->text != NULL)
    {
        free(label->text);
        label->text = NULL;
    }

    if(text != NULL)
        label->text = strdup(text);

    marquee->scroll_offset = 0;
    marquee->tick_count = 0;
    marquee->paused = true;
    marquee->stopped = false;

    if(marquee->direction == VK_SCROLL_RIGHT && label->text != NULL)
    {
        int text_len = strlen(label->text);
        int width = VK_WIDGET(marquee)->width;

        if(text_len > width)
            marquee->scroll_offset = text_len - width;
    }

    return 0;
}

inline const char*
vk_marquee_get_text(vk_marquee_t *marquee)
{
    if(marquee == NULL) return NULL;

    if(!vk_object_assert(marquee, vk_marquee_t)) return NULL;

    return VK_LABEL(marquee)->text;
}

inline int
vk_marquee_set_direction(vk_marquee_t *marquee, int direction)
{
    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    if(direction != VK_SCROLL_LEFT && direction != VK_SCROLL_RIGHT
        && direction != VK_SCROLL_LOOP)
        return -1;

    marquee->direction = direction;

    return 0;
}

inline int
vk_marquee_set_speed(vk_marquee_t *marquee, int interval)
{
    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    if(interval < 1) return -1;

    marquee->scroll_interval = interval;

    return 0;
}

inline int
vk_marquee_set_pause(vk_marquee_t *marquee, int duration)
{
    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    if(duration < 0) return -1;

    marquee->pause_duration = duration;

    return 0;
}

inline int
vk_marquee_set_repeat(vk_marquee_t *marquee, bool repeat)
{
    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    marquee->repeat = repeat;

    return 0;
}

inline int
vk_marquee_run(vk_marquee_t *marquee)
{
    if(marquee == NULL) return -1;

    if(!vk_object_assert(marquee, vk_marquee_t)) return -1;

    return VK_LABEL(marquee)->_update(VK_LABEL(marquee));
}

inline void
vk_marquee_destroy(vk_marquee_t *marquee)
{
    if(marquee == NULL) return;

    if(!vk_object_assert(marquee, vk_marquee_t)) return;

    marquee->dtor(VK_OBJECT(marquee));
}


static int
_vk_marquee_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_marquee_t    *marquee;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_LABEL_KLASS->ctor(object, argp);

    va_end(args);

    marquee = VK_MARQUEE(object);

    marquee->direction = VK_SCROLL_LEFT;
    marquee->scroll_interval = 1;
    marquee->pause_duration = 10;
    marquee->repeat = true;

    marquee->scroll_offset = 0;
    marquee->tick_count = 0;
    marquee->paused = true;
    marquee->stopped = false;

    marquee->ctor = _vk_marquee_ctor;
    marquee->dtor = _vk_marquee_dtor;

    VK_LABEL(marquee)->_update = _vk_marquee_update;

    return 0;
}

static int
_vk_marquee_dtor(vk_object_t *object)
{
    vk_label_t  *label;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_marquee_t)) return -1;

    label = VK_LABEL(object);

    if(label->text != NULL)
    {
        free(label->text);
        label->text = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_marquee_update(vk_label_t *label)
{
    vk_marquee_t    *marquee;
    vk_widget_t     *widget;
    int             text_len;
    int             colors;
    int             x;

    if(label == NULL) return -1;

    marquee = VK_MARQUEE(label);
    widget = VK_WIDGET(label);

    widget->_erase(widget);

    colors = VIPER_COLORS(widget->fg, widget->bg) | widget->attrs;
    wbkgd(widget->canvas, ' ' | colors);

    if(label->text == NULL) return 0;

    text_len = strlen(label->text);

    if(text_len <= widget->width)
    {
        switch(label->justify)
        {
            case VK_JUSTIFY_RIGHT:
                x = widget->width - text_len;
                break;

            case VK_JUSTIFY_CENTER:
                x = (widget->width - text_len) / 2;
                break;

            default:
                x = 0;
                break;
        }

        wattron(widget->canvas, colors);
        mvwprintw(widget->canvas, 0, x, "%s", label->text);
        wattroff(widget->canvas, colors);

        return 0;
    }

    if(marquee->direction == VK_SCROLL_LOOP)
    {
        wchar_t     *wtext;
        cchar_t     wch;
        int         wlen;
        int         period;
        int         i;
        int         vpos;

        wtext = malloc((text_len + 1) * sizeof(wchar_t));
        wlen = mbstowcs(wtext, label->text, text_len + 1);

        if(wlen < 0) wlen = 0;

        period = wlen + 3;

        marquee->tick_count++;

        if(marquee->tick_count >= marquee->scroll_interval)
        {
            marquee->tick_count = 0;
            marquee->scroll_offset++;

            if(marquee->scroll_offset >= period)
                marquee->scroll_offset = 0;
        }

        wattron(widget->canvas, colors);

        for(i = 0; i < widget->width; i++)
        {
            vpos = (marquee->scroll_offset + i) % period;

            if(vpos < wlen)
            {
                setcchar(&wch, &wtext[vpos], colors, 0, NULL);
                mvwadd_wch(widget->canvas, 0, i, &wch);
            }
            else
            {
                mvwaddch(widget->canvas, 0, i, ' ');
            }
        }

        wattroff(widget->canvas, colors);
        free(wtext);

        return 0;
    }

    if(!marquee->stopped)
    {
        if(marquee->paused)
        {
            marquee->tick_count++;

            if(marquee->tick_count >= marquee->pause_duration)
            {
                marquee->paused = false;
                marquee->tick_count = 0;
            }
        }
        else
        {
            marquee->tick_count++;

            if(marquee->tick_count >= marquee->scroll_interval)
            {
                marquee->tick_count = 0;

                if(marquee->direction == VK_SCROLL_LEFT)
                {
                    marquee->scroll_offset++;

                    if(marquee->scroll_offset > text_len - widget->width)
                    {
                        if(marquee->repeat)
                        {
                            marquee->scroll_offset = 0;
                            marquee->paused = true;
                            marquee->tick_count = 0;
                        }
                        else
                        {
                            marquee->scroll_offset = text_len - widget->width;
                            marquee->stopped = true;
                        }
                    }
                }
                else
                {
                    marquee->scroll_offset--;

                    if(marquee->scroll_offset < 0)
                    {
                        if(marquee->repeat)
                        {
                            marquee->scroll_offset = text_len - widget->width;
                            marquee->paused = true;
                            marquee->tick_count = 0;
                        }
                        else
                        {
                            marquee->scroll_offset = 0;
                            marquee->stopped = true;
                        }
                    }
                }
            }
        }
    }

    wattron(widget->canvas, colors);
    mvwprintw(widget->canvas, 0, 0, "%.*s",
        widget->width, label->text + marquee->scroll_offset);
    wattroff(widget->canvas, colors);

    return 0;
}
