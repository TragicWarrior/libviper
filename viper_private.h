#ifndef _VIPER_PRIVATE_H_
#define _VIPER_PRIVATE_H_

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#include "viper.h"

struct _viper_s
{
    GStaticRecMutex         *lock;
    GSList                  *wnd_list;
    guint32                 wnd_count;
    WINDOW                  *console_mouse;
    WINDOW                  *wallpaper;
    VIPER_FUNC              wallpaper_agent;
    gpointer                wallpaper_arg;
    VIPER_FUNC              border_agent[2];
    VIPER_KMIO_HOOK         kmio_dispatch_hook[2];
    gint8                   xterm;
    uid_t                   user;
    WINDOW                  *redraw_catalyst;
};

struct _viper_wnd_s
{
    WINDOW                  *window;
    WINDOW                  *user_window;
    const gchar             *title;
    gint                    min_width;
    gint                    min_height;
    gint                    max_width;
    gint                    max_height;
    guint32                 window_state;
    GSList                  *event_list;
    VIPER_WKEY_FUNC         key_func;
    VIPER_FUNC              border_agent[2];
    gpointer                userptr;
    gpointer                classid;
};

struct _viper_event_s
{
    gchar                   *event;
    VIPER_FUNC              func;
    gpointer                arg;
};

#endif
