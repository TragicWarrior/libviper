#ifndef _KLASS_CONTEXT_H_
#define _KLASS_CONTEXT_H_

#include <inttypes.h>

#include "klass_object.h"

#ifndef vdk_context_t
#define vdk_context_t   _vdk_context_t
#endif

typedef struct _vdk_context_s   _vdk_context_t;

struct _vdk_context_s
{
    _vdk_object_t   parent_klass;

    _vdk_context_t  *target;

    int             (*blit)         (_vdk_context_t *,_vdk_context_t *);
};

#endif
