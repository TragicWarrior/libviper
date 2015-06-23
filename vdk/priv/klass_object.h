#ifndef _KLASS_OBJECT_H_
#define _KLASS_OBJECT_H_

#include <stdarg.h>
#include <inttypes.h>

#ifdef _USE_VDK_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#ifndef vdk_raster_t
#define vdk_raster_t    _vdk_raster_t
#endif

#ifndef vdk_object_t
#define vdk_object_t    _vdk_object_t
#endif

typedef struct  _vdk_raster_s   _vdk_raster_t;
typedef struct  _vdk_object_s   _vdk_object_t;

typedef union
{
    SCREEN              *screen;
    WINDOW              *window;
    WINDOW              *pad;
}
_vdk_primitive_u;

struct _vdk_raster_s
{
    _vdk_primitive_u    primitive;  /*  this is first so that the the user
                                        can always cast to a WINDOW or
                                        SCREEN pointer.     */
    int                 type;

    const char          *term;
    FILE                *f_in;
    FILE                *f_out;

    int                 width;
    int                 height;

    int                 abs_x;      // x position on the screen
    int                 abs_y;      // y position on the screen

    int                 rel_x;      // x position relative to context
    int                 rel_y;      // y position relative to context
};

struct _vdk_object_s
{
    _vdk_raster_t       raster;
    size_t              size;
    const char          *klass_name;
    int                 (*ctor)         (_vdk_object_t *,va_list *,...);
    int                 (*dtor)         (_vdk_object_t *);

    int                 (*resize)       (_vdk_object_t *,int,int);
    int                 (*move)         (_vdk_object_t *,int,int);
};


#endif
