#ifndef _KLASS_SCREEN_H_
#define _KLASS_SCREEN_H_

#include <inttypes.h>

#include "klass_context.h"

typedef struct _vdk_screen_s        _vdk_screen_t;
typedef struct _vdk_color_mtx_s     _vdk_color_mtx_t;

struct _vdk_color_mtx_s
{
    int     fg;
    int     bg;
};


struct _vdk_screen_s
{
    _vdk_context_t          parent_klass;

    _vdk_color_mtx_t        *matrix;

    uint8_t                 state;

    short                   fg;             // default fg color
    short                   bg;             // default bg color
};


#endif


