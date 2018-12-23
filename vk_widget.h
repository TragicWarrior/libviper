#ifndef _VK_WIDGET_H_
#define _VK_WIDGET_H_

#include <inttypes.h>
#include <stdarg.h>

#include <ncursesw/ncurses.h>

#include "list.h"

#include "viper.h"
#include "vk_object.h"

struct _vk_widget_s
{
    vk_object_t         parent_klass;

    struct list_head    list;               // list struct to connect multiple
                                            // widgets (such as in a container)

    WINDOW              *canvas;            // where the widget is composed
    WINDOW              *surface;           // where the widget blits to

    int                 x;                  // x coord relative to surface
    int                 y;                  // y coord relative to surface

    int                 width;              // width of canvas
    int                 height;             // height of canvas

    short               fg;                 // foreground color for canvas
    short               bg;                 // background color for canvas


    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_draw)            (vk_widget_t *);
    int                 (*_move)            (vk_widget_t *, int, int);
    int                 (*_resize)          (vk_widget_t *, int, int);
    int                 (*_erase)           (vk_widget_t *);

};

#endif


