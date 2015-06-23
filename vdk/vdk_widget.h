#ifndef _VDK_WIDGET_H_
#define _VDK_WIDGET_H_

#include <curses.h>

#include <inttypes.h>

#include "vdk.h"

declare_klass(VDK_WIDGET_KLASS);


typedef struct _vdk_widget_s        vdk_widget_t;
#define	VDK_WIDGET(x)               ((vdk_widget_t*)x)

vdk_widget_t*   vdk_widget_create(int width,int height);

int             vdk_widget_set_context(vdk_widget_t *widget,
                    vdk_context_t *context);

vdk_context_t*  vdk_widget_get_context(vdk_widget_t *widget);

void            vdk_widget_set_colors(vdk_widget_t *widget,short color_pair);

short           vdk_widget_get_fg(vdk_widget_t *widget);

short           vdk_widget_get_bg(vdk_widget_t *widget);

void            vdk_widget_clear(vdk_widget_t *widget);

void            vdk_widget_fill(vdk_widget_t *widget,chtype ch);

void			vdk_widget_draw(vdk_widget_t *widget);

void            vdk_widget_destroy(vdk_widget_t *widget);


#endif


