#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "priv/klass_window.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_surface.h"
#include "vdk_widget.h"
#include "vdk_container.h"

static int
_vdk_window_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_window_dtor(vdk_object_t *object);

// template instance
static vdk_object_t _VDK_Window_Klass =
{
    .klass_name = "VDK_WINDOW",
    .size = sizeof(vdk_window_t),
    .ctor = _vdk_window_ctor,
    .dtor = _vdk_window_dtor,
//    .blit = _vdk_window_blit,
};

const void  *VDK_WINDOW_KLASS = &_VDK_Window_Klass;

// create a new container from scratch
vdk_window_t*
vdk_window_create(int width,int height)
{
    vdk_window_t    *window;

    if(height == 0 || width == 0) return NULL;

    window = (vdk_window_t*)vdk_object_create(VDK_WINDOW_KLASS,width,height);

    return window;
}

static int
_vdk_window_ctor(vdk_object_t *object,va_list *argp,...)
{
    va_list     args;
    int         width;
    int         height;
    int         retval = 0;

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

    VDK_OBJECT(VDK_CONTAINER_KLASS)->ctor(object,NULL,width,height);

    return retval;
}

static int
_vdk_window_dtor(vdk_object_t *object)
{
    vdk_widget_t    *widget;
    int             retval;

    if(object == NULL) return -1;

    widget = VDK_CONTAINER(object)->child;

    // propogate tear-down to child object first
    if(widget != NULL)
    {
        vdk_widget_destroy(widget);
        VDK_CONTAINER(object)->child = NULL;
    }

    // call base class destructor
    retval = VDK_OBJECT(VDK_SURFACE_KLASS)->dtor(object);

    return retval;
}
