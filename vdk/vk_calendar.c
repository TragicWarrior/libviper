#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_calendar.h"

#define VK_CALENDAR_WIDTH   22
#define VK_CALENDAR_HEIGHT  8

static int
_vk_calendar_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_calendar_dtor(vk_object_t *object);

static int
_vk_calendar_update(vk_calendar_t *calendar);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_CALENDAR_KLASS)
{
    .size = KLASS_SIZE(vk_calendar_t),
    .name = KLASS_NAME(vk_calendar_t),
    .ctor = _vk_calendar_ctor,
    .dtor = _vk_calendar_dtor,
};

static int
days_in_month(int month, int year)
{
    static const int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    if(month < 0 || month > 11) return 30;

    int days = dim[month];

    if(month == 1)
    {
        if((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            days = 29;
    }

    return days;
}

static int
day_of_week(int day, int month, int year)
{
    struct tm t = {0};

    t.tm_year = year - 1900;
    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_isdst = -1;

    mktime(&t);

    return t.tm_wday;
}

inline vk_calendar_t*
vk_calendar_create(void)
{
    vk_calendar_t   *calendar;
    time_t          now;
    struct tm       *local;

    calendar = (vk_calendar_t *)vk_object_create(VK_CALENDAR_KLASS,
        VK_CALENDAR_WIDTH, VK_CALENDAR_HEIGHT);

    now = time(NULL);
    local = localtime(&now);

    calendar->month = local->tm_mon;
    calendar->year = local->tm_year + 1900;
    calendar->today_day = local->tm_mday;
    calendar->today_month = local->tm_mon;
    calendar->today_year = local->tm_year + 1900;

    return calendar;
}

inline int
vk_calendar_set_month(vk_calendar_t *calendar, int month, int year)
{
    if(calendar == NULL) return -1;
    if(month < 0 || month > 11) return -1;
    if(year < 1) return -1;

    calendar->month = month;
    calendar->year = year;

    return 0;
}

inline int
vk_calendar_get_month(vk_calendar_t *calendar, int *month, int *year)
{
    if(calendar == NULL) return -1;

    if(month != NULL) *month = calendar->month;
    if(year != NULL) *year = calendar->year;

    return 0;
}

inline int
vk_calendar_prev_month(vk_calendar_t *calendar)
{
    if(calendar == NULL) return -1;

    calendar->month--;
    if(calendar->month < 0)
    {
        calendar->month = 11;
        calendar->year--;
    }

    return 0;
}

inline int
vk_calendar_next_month(vk_calendar_t *calendar)
{
    if(calendar == NULL) return -1;

    calendar->month++;
    if(calendar->month > 11)
    {
        calendar->month = 0;
        calendar->year++;
    }

    return 0;
}

inline int
vk_calendar_set_highlight(vk_calendar_t *calendar, short fg, short bg)
{
    if(calendar == NULL) return -1;

    calendar->highlight_fg = fg;
    calendar->highlight_bg = bg;

    return 0;
}

inline int
vk_calendar_set_highlight_attrs(vk_calendar_t *calendar, attr_t attrs)
{
    if(calendar == NULL) return -1;

    calendar->highlight_attrs = attrs;

    return 0;
}

inline int
vk_calendar_set_dimmed(vk_calendar_t *calendar, short fg, short bg)
{
    if(calendar == NULL) return -1;

    calendar->dimmed_fg = fg;
    calendar->dimmed_bg = bg;

    return 0;
}

inline int
vk_calendar_set_dimmed_attrs(vk_calendar_t *calendar, attr_t attrs)
{
    if(calendar == NULL) return -1;

    calendar->dimmed_attrs = attrs;

    return 0;
}

inline int
vk_calendar_set_header_colors(vk_calendar_t *calendar, short fg, short bg)
{
    if(calendar == NULL) return -1;

    calendar->header_fg = fg;
    calendar->header_bg = bg;

    return 0;
}

inline int
vk_calendar_set_header_attrs(vk_calendar_t *calendar, attr_t attrs)
{
    if(calendar == NULL) return -1;

    calendar->header_attrs = attrs;

    return 0;
}

inline int
vk_calendar_update(vk_calendar_t *calendar)
{
    if(calendar == NULL) return -1;

    return calendar->_update(calendar);
}

inline void
vk_calendar_destroy(vk_calendar_t *calendar)
{
    if(calendar == NULL) return;

    if(!vk_object_assert(calendar, vk_calendar_t)) return;

    calendar->dtor(VK_OBJECT(calendar));
}

static int
_vk_calendar_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_calendar_t   *calendar;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    calendar = VK_CALENDAR(object);

    calendar->highlight_fg = -1;
    calendar->highlight_bg = -1;
    calendar->dimmed_fg = -1;
    calendar->dimmed_bg = -1;
    calendar->header_fg = -1;
    calendar->header_bg = -1;

    calendar->ctor = _vk_calendar_ctor;
    calendar->dtor = _vk_calendar_dtor;
    calendar->_update = _vk_calendar_update;

    return 0;
}

static int
_vk_calendar_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_calendar_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_calendar_update(vk_calendar_t *calendar)
{
    vk_widget_t     *widget;
    WINDOW          *canvas;
    int             normal_colors;
    int             header_colors;
    int             highlight_colors;
    int             dimmed_colors;
    int             first_dow;
    int             dim;
    int             prev_dim;
    int             row, col;
    int             day;
    char            buf[32];
    static const char *month_names[] = {
        "January", "February", "March", "April",
        "May", "June", "July", "August",
        "September", "October", "November", "December"
    };

    if(calendar == NULL) return -1;

    widget = VK_WIDGET(calendar);
    canvas = widget->canvas;

    normal_colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg));

    if(calendar->header_fg == -1 || calendar->header_bg == -1)
        header_colors = normal_colors | calendar->header_attrs;
    else
        header_colors = COLOR_PAIR(vdk_color_pair(
            calendar->header_fg, calendar->header_bg))
            | calendar->header_attrs;

    if(calendar->highlight_fg == -1 || calendar->highlight_bg == -1)
        highlight_colors = normal_colors | A_REVERSE
            | calendar->highlight_attrs;
    else
        highlight_colors = COLOR_PAIR(vdk_color_pair(
            calendar->highlight_fg, calendar->highlight_bg))
            | calendar->highlight_attrs;

    if(calendar->dimmed_fg == -1 || calendar->dimmed_bg == -1)
        dimmed_colors = normal_colors | A_DIM | calendar->dimmed_attrs;
    else
        dimmed_colors = COLOR_PAIR(vdk_color_pair(
            calendar->dimmed_fg, calendar->dimmed_bg))
            | calendar->dimmed_attrs;

    widget->_erase(widget);
    vk_widget_fill(widget, ' ' | normal_colors);

    /* row 0: < Month Year > */
    snprintf(buf, sizeof(buf), "%s %d", month_names[calendar->month],
        calendar->year);

    {
        int label_len = strlen(buf);
        int center = (VK_CALENDAR_WIDTH - label_len) / 2;
        if(center < 2) center = 2;

        wattron(canvas, header_colors);
        mvwprintw(canvas, 0, 0, "<");
        mvwprintw(canvas, 0, center, "%s", buf);
        mvwprintw(canvas, 0, VK_CALENDAR_WIDTH - 1, ">");
        wattroff(canvas, header_colors);
    }

    /* row 1: day-of-week headers */
    {
        static const char *dow_labels[] = {
            "Su","Mo","Tu","We","Th","Fr","Sa"
        };

        wattron(canvas, header_colors);
        for(col = 0; col < 7; col++)
        {
            mvwprintw(canvas, 1, col * 3 + 1, "%s", dow_labels[col]);
        }
        wattroff(canvas, header_colors);
    }

    /* rows 2-7: day grid */
    first_dow = day_of_week(1, calendar->month, calendar->year);
    dim = days_in_month(calendar->month, calendar->year);

    {
        int prev_month = calendar->month - 1;
        int prev_year = calendar->year;
        if(prev_month < 0) { prev_month = 11; prev_year--; }
        prev_dim = days_in_month(prev_month, prev_year);
    }

    day = 1 - first_dow;

    for(row = 0; row < 6; row++)
    {
        for(col = 0; col < 7; col++)
        {
            int print_day;
            int colors;
            int x = col * 3 + 1;
            int y = row + 2;

            if(day < 1)
            {
                print_day = prev_dim + day;
                colors = dimmed_colors;
            }
            else if(day > dim)
            {
                print_day = day - dim;
                colors = dimmed_colors;
            }
            else
            {
                print_day = day;

                if(day == calendar->today_day &&
                   calendar->month == calendar->today_month &&
                   calendar->year == calendar->today_year)
                {
                    colors = highlight_colors;
                }
                else
                {
                    colors = normal_colors;
                }
            }

            wattron(canvas, colors);
            mvwprintw(canvas, y, x, "%2d", print_day);
            wattroff(canvas, colors);

            day++;
        }
    }

    return 0;
}
