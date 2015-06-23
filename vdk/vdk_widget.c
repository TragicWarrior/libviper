#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// #include "priv/klass_object.h"
#include "priv/klass_widget.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_surface.h"
#include "vdk_widget.h"

static int
_vdk_widget_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_widget_dtor(vdk_object_t *object);

static vdk_object_t _VDK_Widget_Klass =
{
    .klass_name = "VDK_WIDGET",
    .size = sizeof(vdk_widget_t),
    .ctor = _vdk_widget_ctor,
    .dtor = _vdk_widget_dtor,
};

const void  *VDK_WIDGET_KLASS = &_VDK_Widget_Klass;

// create a new widget from scratch
vdk_widget_t*
vdk_widget_create(int width,int height)
{
    vdk_widget_t    *widget;

    if(height == 0 || width == 0) return NULL;

    widget = (vdk_widget_t*)vdk_object_create(VDK_WIDGET_KLASS,width,height);

    return widget;
}


int
vdk_widget_set_context(vdk_widget_t *widget,vdk_context_t *context)
{
    int retval;

    if(widget == NULL) return -1;
    if(context == NULL) return -1;

    retval = vdk_context_set_target(VDK_CONTEXT(widget),context);

    return retval;
}

vdk_context_t*
vdk_widget_get_context(vdk_widget_t *widget)
{
    vdk_context_t   *context;

    if(widget == NULL) return NULL;

    context = vdk_context_get_target(VDK_CONTEXT(widget));

    return context;
}

void
vdk_widget_set_colors(vdk_widget_t *widget,short color_pair)
{
    if(widget == NULL) return;
    if(color_pair < 0) return;

    wbkgdset(*(WINDOW**)widget,COLOR_PAIR(color_pair));
    wcolor_set(*(WINDOW**)widget,color_pair,NULL);
    pair_content(color_pair,&widget->fg,&widget->bg);

    return;
}

void
vdk_widget_clear(vdk_widget_t *widget)
{
    if(widget == NULL) return;

    werase(*(WINDOW**)widget);

    return;
}

void
vdk_widget_fill(vdk_widget_t *widget,chtype ch)
{
    vdk_raster_t    *raster;
    long            i;

    if(widget == NULL) return;

    raster = VDK_RASTER(widget);

    i = raster->width * raster->height;

    while(i)
    {
        waddch(*(WINDOW**)widget,ch);
        i--;
    }

    wmove(*(WINDOW**)widget,0,0);

    return;
}

void
vdk_wiget_draw(vdk_widget_t *widget)
{
    if(widget == NULL) return;

    vdk_surface_blit(VDK_SURFACE(widget));

    return;
}

void
vdk_widget_destroy(vdk_widget_t *widget)
{
    if(widget == NULL) return;

    _vdk_widget_dtor(VDK_OBJECT(widget));

    return;
}

static int
_vdk_widget_ctor(vdk_object_t *object,va_list *argp,...)
{
    va_list     args;
    int         width;
    int         height;
    int         retval = 0;
    int         type;

    if(argp == NULL)
    {
        va_start(args,argp);
        argp = &args;
    }

    width = va_arg(*argp,int);
    height = va_arg(*argp,int);

    if(argp == &args) va_end(args);

    if(object == NULL) return -1;
    if(height == 0 || width == 0) return -1;

    type = VDK_RASTER_PAD;

    VDK_OBJECT(VDK_SURFACE_KLASS)->ctor(object,NULL,type,width,height);

    return retval;
}

static int
_vdk_widget_dtor(vdk_object_t *object)
{
    int             retval;

    if(object == NULL) return -1;

    // call base class destructor
    retval = VDK_OBJECT(VDK_SURFACE_KLASS)->dtor(object);

    return retval;
}
