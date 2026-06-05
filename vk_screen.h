#ifndef _VK_SCREEN_H_
#define _VK_SCREEN_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#include <ncursesw/curses.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_desktop_s
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

    vk_desktop_t        **desktops;
    int                 desktop_count;
    int                 active_desktop;

    int                 width;
    int                 height;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);
};

#endif
