#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// private include MUST come first
#include "priv/klass_surface.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_screen.h"
#include "vdk_surface.h"

static int
_vdk_surface_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_surface_dtor(vdk_object_t *object);

static int
_vdk_surface_move(vdk_object_t *object,int x,int y);

static int
_vdk_surface_blit(vdk_context_t *self,vdk_context_t *target);

static vdk_object_t _VDK_Surface_Klass =
{
    .klass_name = "VDK_SURFACE",
    .size = sizeof(vdk_surface_t),
    .ctor = _vdk_surface_ctor,
    .dtor = _vdk_surface_dtor,
    .move = _vdk_surface_move,
};

const void  *VDK_SURFACE_KLASS = &_VDK_Surface_Klass;

vdk_surface_t*
vdk_surface_create(int type,int width,int height)
{
    vdk_surface_t   *surface;

    if((width == 0) || (height == 0)) return NULL;

    // these raster types are not allowed
    if((type == VDK_RASTER_SCR) || (type == VDK_RASTER_NULL)) return NULL;

    surface = (vdk_surface_t*)vdk_object_create(VDK_SURFACE_KLASS,
        type,width,height);

    return surface;
}

int
vdk_surface_move(vdk_surface_t *surface,int x,int y)
{
    int retval;

    if(surface == NULL) return -1;
    if(x < 0) return -1;
    if(y < 0) return -1;

    retval = _vdk_surface_move(VDK_OBJECT(surface),x,y);

    return retval;
}

static int
_vdk_surface_ctor(vdk_object_t *object,va_list *argp,...)
{
    va_list     args;
    int         type;
    int         width;
    int         height;
    int         retval;

    if(argp == NULL)
    {
        va_start(args,argp);
        argp = &args;
    }

    type = va_arg(*argp,int);
    width = va_arg(*argp,int);
    height = va_arg(*argp,int);

    if(argp == &args) va_end(args);

    if(object == NULL) return -1;
    if(width == 0) return -1;
    if(height == 0) return -1;

    VDK_OBJECT(VDK_CONTEXT_KLASS)->ctor(object,NULL);

    // these raster types are not allowed
    if((type == VDK_RASTER_SCR) || (type == VDK_RASTER_NULL)) return -1;

    retval = vdk_object_rasterize(VDK_OBJECT(object),
        type,NULL,NULL,NULL,width,height);

    VDK_CONTEXT(object)->blit = _vdk_surface_blit;

    return retval;
}

static int
_vdk_surface_blit(vdk_context_t *self,vdk_context_t *target)
{
    int     surface_type;
    int     target_type;
    WINDOW  *output;

    int     max_x;
    int     max_y;

    int     top;
    int     bottom;
    int     left;
    int     right;

	if(self == NULL) return -1;
    if(target == NULL) return -1;

	// ignore operation when state is set to hidden
	if(VDK_SURFACE(self)->state & VDK_SURFACE_HIDDEN) return -1;

    surface_type = vdk_object_get_raster_type(VDK_OBJECT(self));
    target_type = vdk_object_get_raster_type(VDK_OBJECT(target));

    if(target_type != VDK_RASTER_NULL)
    {
        max_x = VDK_RASTER(target)->width - 1;
        max_y = VDK_RASTER(target)->height - 1;

        // return OK if the terminal has been resized to 0 lines or 0 columns
        if(max_x == 0 || max_y == 0) return 0;

        // calculate our target rectangle
        left = VDK_RASTER(self)->rel_x;
        top = VDK_RASTER(self)->rel_y;
        right = VDK_RASTER(self)->rel_x + VDK_RASTER(self)->width - 1;
        bottom = VDK_RASTER(self)->rel_y + VDK_RASTER(self)->height - 1;

        // make sure our target stays on the screen--clipping as needed.
        if(bottom > max_y) bottom = max_y;
        if(right > max_x) right = max_x;

        // make sure top-left wouldn't be off the screen
        if(VDK_RASTER(self)->rel_x > max_x) return -1;
        if(VDK_RASTER(self)->rel_y > max_y) return -1;

        if(target_type == VDK_RASTER_SCR)
            output = stdscr;
        else
            output = *(WINDOW**)target;

        copywin(*((WINDOW**)self),
            output,
            0,0,
            top,left,bottom,right,
            FALSE);

        return 0;
    }

	return 0;
}

int
vdk_surface_blit(vdk_surface_t *surface)
{
    vdk_context_t   *context;
    int             retval;

    if(surface == NULL) return -1;

    context = VDK_CONTEXT(surface);

    if(context->target == NULL) return -1;

    retval = _vdk_surface_blit(context,context->target);

    return retval;
}

void
vdk_surface_destroy(vdk_surface_t *surface)
{
    if(surface == NULL) return;

    _vdk_surface_dtor(VDK_OBJECT(surface));

	return;
}

static int
_vdk_surface_move(vdk_object_t *object,int x,int y)
{
    vdk_context_t   *context;
    int             raster_type;
    int             ctx_x = 0;
    int             ctx_y = 0;

    if(object == NULL) return -1;
    if(x < 0) return -1;
    if(y < 0) return -1;

    raster_type = vdk_object_get_raster_type(object);

    if(raster_type == VDK_RASTER_NULL || raster_type == VDK_RASTER_SCR)
        return -1;

    context = vdk_context_get_target(VDK_CONTEXT(object));
    raster_type = vdk_object_get_raster_type(VDK_OBJECT(context));

    if(raster_type != VDK_RASTER_NULL && raster_type != VDK_RASTER_SCR)
    {
        ctx_x = VDK_RASTER(context)->abs_x;
        ctx_y = VDK_RASTER(context)->abs_y;
    }

    VDK_RASTER(object)->rel_x = x;
    VDK_RASTER(object)->rel_y = y;
    VDK_RASTER(object)->abs_x = ctx_x + x;
    VDK_RASTER(object)->abs_y = ctx_y + y;

    /*
        if the raster type of the object we're moving is a VDK_RASTER_WND
        (which is a true curses WINDOW) then we'll try to do the physical
        move.

        according to the man pages this will error if the WINDOW would be
        relocated off the screen.
    */
    raster_type = vdk_object_get_raster_type(object);
    if(raster_type == VDK_RASTER_WND)
    {
        mvwin(*(WINDOW**)object,y,x);
    }

    return 0;
}

int
vdk_surface_resize(vdk_surface_t *surface,int width,int height)
{
    int     raster_type;
    int     retval;

	if(surface == NULL) return -1;

    raster_type = vdk_object_get_raster_type(VDK_OBJECT(surface));
    if(raster_type == VDK_RASTER_NULL) return -1;
    if(raster_type == VDK_RASTER_SCR) return -1;

    if(width < 1)
    {
        width = VDK_RASTER(surface)->width;
    }

    if(height < 1)
    {
        height = VDK_RASTER(surface)->height;
    }

    VDK_RASTER(surface)->width = width;
    VDK_RASTER(surface)->height = height;

	retval = wresize(*(WINDOW**)surface,height,width);

    return retval;
}

static int
_vdk_surface_dtor(vdk_object_t *self)
{
    if(self == NULL) return -1;

	// delwin(canvas->window);
	// free(canvas);

    return 0;
}
