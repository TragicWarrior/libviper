#ifndef _VK_SCREEN_H_
#define _VK_SCREEN_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <termios.h>

#include <ncursesw/curses.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_surface_s
{
    WINDOW              *canvas;
    vk_widget_t         **widgets;
    int                 widget_count;
    int                 widget_alloc;
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

    pid_t               evicted_pid;
    struct termios      saved_termios;
    bool                has_saved_termios;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
