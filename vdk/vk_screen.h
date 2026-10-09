#ifndef _VK_SCREEN_H_
#define _VK_SCREEN_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <termios.h>

#include <ncursesw/curses.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_surface_s
{
    WINDOW              *canvas;
    vk_widget_t         **widgets;
    int                 widget_count;
    int                 widget_alloc;
    cchar_t             bkgd;       /* wbkgrndset value; survives teleport */
};

struct _vk_screen_s
{
    vk_object_t         parent_klass;

    SCREEN              *term;
    FILE                *fd_in;
    FILE                *fd_out;

    vk_surface_t        **surfaces;
    int                 surface_count;
    int                 active_surface;

    int                 width;
    int                 height;

    VkSurfaceBkgdFunc   wallpaper_func;
    VkSurfaceBkgdFunc   overlay_func;

    /* the modes of the terminal the screen moved onto, as they were
       before it arrived; put back when it leaves */
    struct termios      saved_termios;
    bool                has_saved_termios;

    /* true while the screen is on no terminal at all (vk_screen_detach) */
    bool                detached;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
