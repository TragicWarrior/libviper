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


struct _viper_ctx_s
{
    int     screen_id;
    bool    managed;
};

struct _viper_event_s
{
    struct list_head        list;
    char                   	*event;
    ViperFunc               func;
    void                	*arg;
};

struct _viper_wnd_s
{
    WINDOW                  *user_window;
    WINDOW                  *window_frame;

    vctx_t                  *ctx;

    const char              *title;
    struct list_head        list;

    int                     min_width;
    int                     min_height;
    int                     max_width;
    int                     max_height;

    uint32_t                window_state;
    struct list_head        event_list;

    ViperWkeyFunc           key_func;
    ViperFunc               border_agent[2];

    void                    *userptr;
    void                    *classid;
};

struct _viper_s
{
    int                     cur_scr_id;
    WINDOW                  *screen[MAX_SCREENS];

    struct list_head        managed_list[MAX_SCREENS];
    struct list_head        unmanaged_list[MAX_SCREENS];

    WINDOW                  *console_mouse;

    WINDOW                  *wallpaper[MAX_SCREENS];
    ViperBkgdFunc           wallpaper_agent[MAX_SCREENS];

    void                    *wallpaper_arg;
    ViperFunc               border_agent[2];

    ViperKmioHook           kmio_dispatch_hook[2];

    vwnd_t                  *redraw_catalyst;

    int8_t                  xterm;
    uid_t                   user;
};


#endif
