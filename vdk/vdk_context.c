#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// private include MUST come first
#include "priv/klass_context.h"

#include "vdk_object.h"
#include "vdk_context.h"

static int
_vdk_context_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_context_dtor(vdk_object_t *object);

/*
    this is our static template which will be passed into the object
    allocator and also to the constructor.
*/
static _vdk_object_t _VDK_Context_Klass =
{
    .klass_name = "VDK_CONTEXT",
    .size = sizeof(vdk_context_t),
    .ctor = _vdk_context_ctor,
    .dtor = _vdk_context_dtor,
};

const void  *VDK_CONTEXT_KLASS = &_VDK_Context_Klass;

vdk_context_t*
vdk_context_create(void)
{
    vdk_context_t  *context;

    context = (vdk_context_t*)vdk_object_create(VDK_CONTEXT_KLASS);

    return context;
}

static int
_vdk_context_ctor(vdk_object_t *object,va_list *argp,...)
{
    if(object == NULL) return -1;

    return 0;
}


vdk_context_t*
vdk_context_get_target(vdk_context_t *context)
{
    if(context == NULL) return NULL;

    return context->target;
}

int
vdk_context_set_target(vdk_context_t *context,vdk_context_t *target)
{
    int raster_type;

    if(context == NULL) return -1;
    if(target == NULL) return -1;

    // make sure "target" can be blitted to
    raster_type = vdk_object_get_raster_type(VDK_OBJECT(target));
    if(raster_type == VDK_RASTER_NULL) return -1;

    context->target = target;

    return 0;
}

static int
_vdk_context_dtor(vdk_object_t *object)
{
    int     retval;

    if(object == NULL) return -1;

    // call the destructor of the base class
    retval = VDK_OBJECT(VDK_OBJECT_KLASS)->dtor(object);

    return retval;
}

int
vdk_context_destroy(vdk_context_t *context)
{
    vdk_object_t    *object = VDK_OBJECT(context);
    int             retval;

    if(context == NULL) return -1;

    retval = object->dtor(object);

    return retval;
}

