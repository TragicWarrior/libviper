#ifndef _VDK_LABEL_H_
#define _VDK_LABEL_H_

#include "vdk.h"

declare_klass(VDK_LABEL_KLASS);


typedef struct _vdk_label_s         vdk_label_t;
#define	VDK_LABEL(x)                ((vdk_label_t*)x)


vdk_label_t*    vdk_label_create(const char *text);

int             vdk_label_set_text(vdk_label_t *label,
                    const char *text,bool resize);

char*           vdk_label_get_text(vdk_label_t *label);

#endif


