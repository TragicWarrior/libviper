#ifndef _KLASS_WIDGET_H_
#define _KLASS_WIDGET_H_

#include "klass_surface.h"

#ifndef vdk_widget_t
#define vdk_widget_t    _vdk_widget_t
#endif

typedef struct _vdk_widget_s    _vdk_widget_t;

struct _vdk_widget_s
{
    _vdk_surface_t  parent_klass;

    int             x;
    int             y;

    short           fg;             // curses foreground color
    short           bg;             // curese background color
};

#endif

