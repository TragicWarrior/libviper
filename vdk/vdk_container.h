#ifndef _VDK_CONTAINER_H_
#define _VDK_CONTAINER_H_

#include <curses.h>

#include <inttypes.h>

#include "vdk.h"
#include "vdk_widget.h"

declare_klass(VDK_CONTAINER_KLASS);


typedef struct _vdk_container_s     vdk_container_t;
#define	VDK_CONTAINER(x)            ((vdk_container_t*)x)

vdk_container_t*    vdk_container_create(int width,int height);

int                 vdk_container_attach_widget(vdk_container_t *container,
                        vdk_widget_t *widget);

vdk_widget_t*       vdk_container_remove_widget(vdk_container_t *container);

vdk_widget_t*       vdk_container_fetch_widget(vdk_container_t *container);

void                vdk_contianer_destroy(vdk_container_t *container);


#endif


