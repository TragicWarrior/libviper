#ifndef _KLASS_WINDOW_H_
#define _KLASS_WINDOW_H_

#include "klass_container.h"

#ifndef vdk_window_t
#define vdk_window_t                _vdk_window_t
#endif

typedef struct _vdk_window_s        _vdk_window_t;

struct _vdk_window_s
{
    _vdk_container_t    parent_klass;

    uint8_t             flags;
    char                *title;

    short               frame_fg;
    short               frame_bg;
};

#endif

