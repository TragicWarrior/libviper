#ifndef _VDK_SURFACE_H_
#define _VDK_SURFACE_H_

#include <stdlib.h>
#include <inttypes.h>

#include "vdk.h"

declare_klass(VDK_SURFACE_KLASS);

typedef struct _vdk_surface_s   vdk_surface_t;
#define	VDK_SURFACE(x)          ((vdk_surface_t *)x)

#define VDK_SURFACE_HIDDEN      (1 << 1)
#define VDK_SURFACE_FROZEN      (1 << 2)


vdk_surface_t*  vdk_surface_create(int type,int width,int height);

int             vdk_surface_blit(vdk_surface_t *surface);

int             vdk_surface_move(vdk_surface_t *surface,int x,int y);

int             vdk_surface_resize(vdk_surface_t *surface,int width,int height);

void            vdk_surface_destroy(vdk_surface_t *surface);


#endif


