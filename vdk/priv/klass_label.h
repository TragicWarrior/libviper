#ifndef _KLASS_LABEL_H_
#define _KLASS_LABEL_H_

#include "klass_widget.h"

#ifndef vdk_label_t
#define vdk_label_t     _vdk_label_t
#endif

typedef struct _vdk_label_s     _vdk_label_t;

struct _vdk_label_s
{
    _vdk_widget_t   parent_klass;

    char            *text;
};

#endif

