#ifndef _VDK_CONTEXT_H_
#define _VDK_CONTEXT_H_

#include <stdlib.h>
#include <inttypes.h>

#include "vdk.h"

declare_klass(VDK_CONTEXT_KLASS);

typedef struct _vdk_context_s       vdk_context_t;
#define	VDK_CONTEXT(x)              ((vdk_context_t *)x)


vdk_context_t*  vdk_context_create(void);

int             vdk_context_get_height(vdk_context_t *context);

int             vdk_context_get_width(vdk_context_t *context);

vdk_context_t*  vdk_context_get_target(vdk_context_t *context);

int             vdk_context_set_target(vdk_context_t *context,
                    vdk_context_t *target);

void            vdk_context_blit(vdk_context_t *context);

int             vdk_context_destroy(vdk_context_t *context);

#endif


