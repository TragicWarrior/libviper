#ifndef _KLASS_CONTAINER_H_
#define _KLASS_CONTAINER_H_

#include "klass_widget.h"

#ifndef vdk_container_t
#define vdk_container_t             _vdk_container_t
#endif

typedef struct _vdk_container_s     _vdk_container_t;

struct _vdk_container_s
{
    _vdk_widget_t   parent_klass;

    _vdk_widget_t   *child;
};

#endif

