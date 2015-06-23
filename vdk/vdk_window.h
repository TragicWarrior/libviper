#ifndef _VDK_WINDOW_H_
#define _VDK_WINDOW_H_

#include <curses.h>

#include <inttypes.h>

#include "vdk.h"

declare_klass(VDK_WINDOW_KLASS);

#define VDK_WINDOW_FRAMED           (1 << 0)
#define VDK_WINDOW_SHADOWED         (1 << 1)


typedef struct _vdk_window_s        vdk_window_t;
#define	VDK_WINDOW(x)               ((vdk_window_t*)x)

vdk_window_t*   vdk_window_create(int width,int height);

int             vdk_window_decorate(vdk_window_t *window,uint8_t flags);

void            vdk_window_add_frame(vdk_window_t *window,int style);

void            vdk_window_del_frame(vdk_window_t *window);

void            vdk_window_set_frame_colors(vdk_window_t *window,
                    short color_pair);

void            vdk_window_set_title(vdk_window_t *window,const char *title);

char*           vdk_window_get_title(vdk_window_t *window);

void            vdk_window_destroy(vdk_window_t *window);


#endif


