#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "priv/klass_container.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_surface.h"
#include "vdk_widget.h"
#include "vdk_container.h"

static int
_vdk_container_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_container_dtor(vdk_object_t *object);

// template instance
static vdk_object_t _VDK_Container_Klass =
{
    .klass_name = "VDK_CONTAINER",
    .size = sizeof(vdk_container_t),
    .ctor = _vdk_container_ctor,
    .dtor = _vdk_container_dtor,
};


const void  *VDK_CONTAINER_KLASS = &_VDK_Container_Klass;

// create a new container from scratch
vdk_container_t*
vdk_container_create(int width,int height)
{
    vdk_container_t    *container;

    if(height == 0 || width == 0) return NULL;

    container = (vdk_container_t*)vdk_object_create(VDK_CONTAINER_KLASS,
        width,height);

    return container;
}

int
vdk_container_attach_widget(vdk_container_t *container,vdk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    container->child = widget;

    return 0;
}

vdk_widget_t*
vdk_container_remove_widget(vdk_container_t *container)
{
    vdk_widget_t    *child = NULL;

    if(container == NULL) return NULL;

    child = container->child;

    container->child = NULL;

    return child;
}

vdk_widget_t*
vdk_container_fetch_widget(vdk_container_t *container)
{
    if(container == NULL) return NULL;

    return container->child;
}


static int
_vdk_container_ctor(vdk_object_t *object,va_list *argp,...)
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

    VDK_OBJECT(VDK_WIDGET_KLASS)->ctor(object,NULL,width,height);

    return retval;
}

static int
_vdk_container_dtor(vdk_object_t *object)
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
