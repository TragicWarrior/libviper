#ifndef _KLASS_SURFACE_H_
#define _KLASS_SURFACE_H_

#include <inttypes.h>

#include "klass_context.h"

#ifndef vdk_surface_t
#define vdk_surface_t   _vdk_surface_t
#endif

typedef struct _vdk_surface_s   _vdk_surface_t;

struct _vdk_surface_s
{
    _vdk_context_t  parent_klass;
    uint32_t        state;
};

#endif
