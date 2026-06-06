#ifndef _VK_VIEWPORT_H_
#define _VK_VIEWPORT_H_

#include <stdarg.h>

#include <ncursesw/curses.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

/*
    A vk_viewport renders a window into a logical canvas.  The canvas
    itself isn't owned by the viewport -- the consumer supplies a
    `get_row` callback that fills a horizontal slice on demand.  This
    keeps the abstraction agnostic about how the data is stored
    (vterm cell matrix, in-memory buffer, computed on the fly, ...).

    The vk_viewport_src_t struct is defined in vdk.h since it is
    public POD that consumers construct directly.
*/

struct _vk_viewport_s
{
    vk_widget_t             parent_klass;

    vk_viewport_src_t       src;

    int                     scroll_row;
    int                     scroll_col;

    int                     (*ctor)             (vk_object_t *, va_list *, ...);
    int                     (*dtor)             (vk_object_t *);

    int                     (*_update)          (vk_viewport_t *);
};

#endif
