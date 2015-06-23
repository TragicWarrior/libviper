#ifndef _VDK_OBJECT_H_
#define _VDK_OBJECT_H_

#include <stdio.h>
#include <inttypes.h>

#include "vdk.h"

declare_klass(VDK_OBJECT_KLASS);

typedef struct _vdk_object_s        vdk_object_t;
#define VDK_OBJECT(x)               ((vdk_object_t *)x)

#define VDK_RASTER(object)          ((vdk_raster_t*)object)

#define VDK_WINDOW_PTR(object)      ((WINDOW*)object)
#define VDK_SCREEN_PTR(object)      ((SCREEN*)object)

enum
{
    VDK_RASTER_NULL     =   0x0,
    VDK_RASTER_SCR,
    VDK_RASTER_WND,
    VDK_RASTER_PAD
};

//extern const void   *VDK_OBJECT_KLASS;

/*
    @param 'type'       Specifies the type of raster device:
                        VDK_RASTER_SCREEN, etc..

    @param 'term'       VDK_RASTER_SCREEN will initialize the SCREEN raster
                        based on $TERM if 'term' is not specified.  This is
                        almost alwasy the desired behavior.

    @param 'f_in'       Indicates the terminal input FILE handle.  If not
                        specified the default is stdin.

    @param 'f_out'      Indicates the termianl output FILE handle.  If not
                        specified the default is stdout.

    @param 'width'      Default width of a VDK_RASTER_WIDNOW or
                        VDK_RASTER_PAD.  Ignored for VDK_RASTER_SCREEN.

    @param 'height'     Default height of a VDK_RASTER_WIDNOW or
                        VDK_RASTER_PAD.  Ignored for VDK_RASTER_SCREEN.
*/

vdk_object_t*   vdk_object_create(const void *klass,...);

int             vdk_object_rasterize(vdk_object_t *object,int type,
                    const char *term,FILE *fin,FILE *fout,
                    int width,int height);

int             vdk_object_get_raster_type(vdk_object_t *object);

const char*     vdk_object_get_klass_name(vdk_object_t *object);

int             vdk_object_destroy(vdk_object_t *object);

#endif
