#ifndef _VK_PROGRESS_H_
#define _VK_PROGRESS_H_

#include <stdarg.h>
#include <wchar.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_progress_s
{
    vk_widget_t         parent_klass;

    int                 orientation;    /* VK_PROGRESS_HORIZONTAL | _VERTICAL */
    int                 style;          /* VK_PROGRESS_UNICODE | _ASCII        */
    int                 thickness;      /* cross-axis cells; default 1        */
    int                 relief;         /* 0 (none) | VK_RELIEF_SUNKEN         */
    int                 trough_style;   /* VK_TROUGH_NONE | _STIPPLE | _SOLID  */

    double              value;
    double              range_min;
    double              range_max;

    short               fill_fg;
    short               fill_bg;
    attr_t              fill_attrs;

    short               trough_fg;
    short               trough_bg;
    wchar_t             trough_ch;      /* glyph for VK_TROUGH_STIPPLE         */

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);

    /*
        virtual: the FILLED portion's fg/bg at the current value.  vk_progress
        returns fill_fg / fill_bg; a derived widget (vk_meter) overrides it to
        select value-dependent colours.  Returning fg and bg (not a packed
        pair) lets the draw build the sub-cell partial cell, whose unfilled
        half must blend with the trough.  All geometry / drawing is shared --
        only the fill colour is virtual.
    */
    void                (*_fill_color)  (vk_progress_t *, short *fg, short *bg);
};

#endif
