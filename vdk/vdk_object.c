#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "priv/klass_object.h"

#include "vdk_object.h"

static int
_vdk_object_dtor(vdk_object_t *object);

static int
_vdk_raster_init_screen(vdk_raster_t *raster,
    const char *term,FILE *f_in,FILE *f_out);

static int
_vdk_raster_init_window(vdk_raster_t *raster,int width,int height);

static int
_vdk_raster_init_pad(vdk_raster_t *raster,int width,int height);


/*
    this is our static template which will be passed into the object
    allocator and also to the constructor.
*/
static _vdk_object_t _VDK_Object_Klass =
{
    .klass_name = "VDK_OBJECT",
    .size = sizeof(vdk_object_t),
    .dtor = _vdk_object_dtor,
};

const void  *VDK_OBJECT_KLASS = &_VDK_Object_Klass;

vdk_object_t*
vdk_object_create(const void *klass,...)
{
    vdk_object_t    *object;
    va_list         argp;

    if(klass == NULL) return NULL;

    object = calloc(1,VDK_OBJECT(klass)->size);

    // copy template to newly alloced object
    object->klass_name = VDK_OBJECT(klass)->klass_name;
    object->size = VDK_OBJECT(klass)->size;
    object->ctor = VDK_OBJECT(klass)->ctor;
    object->dtor = VDK_OBJECT(klass)->dtor;

    object->move = VDK_OBJECT(klass)->move;
    object->resize = VDK_OBJECT(klass)->resize;

    // if the object has a contrcutor, run it
    if(object->ctor != NULL)
    {
        va_start(argp,klass);

        // pass the pointer to the variable argument list structure
        object->ctor(object,&argp);

        va_end(argp);
    }

    return object;
}

int
vdk_object_rasterize(vdk_object_t *object,int type,
    const char *term,FILE *f_in,FILE *f_out,int width,int height)
{
    vdk_raster_t    raster;
    int             retval = 0;

    if(object == NULL) return -1;
    if(type == VDK_RASTER_NULL) return 0;

    raster.type = type;
    raster.term = term;
    raster.f_in = f_in;
    raster.f_out = f_out;
    raster.width = width;
    raster.height = height;

    memcpy(&object->raster,&raster,sizeof(vdk_raster_t));

    switch(type)
    {
        case VDK_RASTER_SCR:
        {
            retval = _vdk_raster_init_screen(&object->raster,term,f_in,f_out);
            break;
        }

        case VDK_RASTER_WND:
        {
            retval = _vdk_raster_init_window(&object->raster,width,height);
            break;
        }

        case VDK_RASTER_PAD:
        {
            retval = _vdk_raster_init_pad(&object->raster,width,height);
            break;
        }
    }

    return retval;
}

inline int
vdk_object_get_raster_type(vdk_object_t *object)
{
    if(object == NULL) return -1;

    return object->raster.type;
}

int
vdk_object_destroy(vdk_object_t *object)
{
    /*
        call the objects destructor.  if it was installed correctly, object
        tear down should cascade from the topmost derivative until it hits
        the bottom--which is _vdk_object_dtor().
    */

    if(object->dtor != NULL)
    {
        object->dtor(object);
    }

    return 0;
}

int
_vdk_raster_init_screen(vdk_raster_t *raster,
    const char *term,FILE *f_in,FILE *f_out)
{
    SCREEN      *screen;

    if(raster == NULL) return -1;

    if(f_in == NULL) f_in = stdin;
    if(f_out == NULL) f_out = stdout;

    // if term == NULL then newterm() uses $TERM
    screen = newterm(term,f_out,f_in);

    if(screen == NULL) return -1;

    raster->primitive.screen = screen;

    return 0;
}

int
_vdk_raster_init_window(vdk_raster_t *raster,int width,int height)
{
    WINDOW      *window;

    if(raster == NULL) return -1;
    if(width < 0 || height < 0) return -1;

    window = newwin(height,width,0,0);

    if(window == NULL) return -1;

    raster->primitive.window = window;

    return 0;
}

int
_vdk_raster_init_pad(vdk_raster_t *raster,int width,int height)
{
    WINDOW      *pad;

    if(raster == NULL) return -1;
    if(width < 0 || height < 0) return -1;

    pad = newpad(height,width);

    if(pad == NULL) return -1;

    raster->primitive.pad = pad;

    return 0;
}


static int
_vdk_object_dtor(vdk_object_t *object)
{
    if(object == NULL) return -1;

    free(object);

    return 0;
}
