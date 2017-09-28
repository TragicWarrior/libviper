#ifndef _VIPER_PRIVATE_H_
#define _VIPER_PRIVATE_H_

#include <inttypes.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#include "viper.h"
#include "list.h"

struct _viper_s
{
    struct list_head        wnd_list;
    WINDOW                  *console_mouse;
    WINDOW                  *wallpaper;
    VIPER_FUNC              wallpaper_agent;
    void                    *wallpaper_arg;
    VIPER_FUNC              border_agent[2];
    VIPER_KMIO_HOOK         kmio_dispatch_hook[2];
    int8_t                  xterm;
    uid_t                   user;
    WINDOW                  *redraw_catalyst;
};

struct _viper_event_s
{
    struct list_head        list;
    char                   	*event;
    VIPER_FUNC              func;
    void                	*arg;
};

struct _viper_wnd_s
{
    WINDOW                  *window;
    WINDOW                  *user_window;
    const char              *title;
    struct list_head        list;
    int                     min_width;
    int                     min_height;
    int                     max_width;
    int                     max_height;
    uint32_t                window_state;
    struct list_head        event_list;
    VIPER_WKEY_FUNC         key_func;
    VIPER_FUNC              border_agent[2];
    void                    *userptr;
    void                    *classid;
};


#endif
