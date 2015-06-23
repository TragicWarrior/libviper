#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <curses.h>

// #include "priv/klass_object.h"
#include "priv/klass_label.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_surface.h"
#include "vdk_widget.h"
#include "vdk_label.h"

static int
_vdk_label_ctor(vdk_object_t *object,va_list *argp,...);

static int
_vdk_label_dtor(vdk_object_t *object);

static vdk_object_t _VDK_Label_Klass =
{
    .klass_name = "VDK_LABEL",
    .size = sizeof(vdk_label_t),
    .ctor = _vdk_label_ctor,
    .dtor = _vdk_label_dtor,
};

const void  *VDK_LABEL_KLASS = &_VDK_Label_Klass;

// create a new widget from scratch
vdk_label_t*
vdk_label_create(const char *text)
{
    vdk_label_t     *label;

    if(text == NULL) return NULL;
    if(text[0] == '\0') return NULL;

    label = (vdk_label_t*)vdk_object_create(VDK_LABEL_KLASS,text);

    return label;
}

int
vdk_label_set_text(vdk_label_t *label,const char *text,bool resize)
{
    int     width;

    if(label == NULL) return -1;

    if(text == NULL) return -1;
    if(text[0] == '\0') return -1;

    if(label->text != NULL)
    {
        free(label->text);
        label->text = NULL;
    }

    width = strlen(text);
    label->text = strdup(text);

    if(resize == TRUE)
    {
        vdk_surface_resize(VDK_SURFACE(label),width,-1);
    }

    wprintw(*(WINDOW**)label,"%s",text);
    wmove(*(WINDOW**)label,0,0);

    return 0;
}

void
vdk_label_destroy(vdk_label_t *label)
{
    if(label == NULL) return;

    _vdk_label_dtor(VDK_OBJECT(label));

    return;
}

static int
_vdk_label_ctor(vdk_object_t *object,va_list *argp,...)
{
    va_list     args;
    char        *text;
    int         width;
    int         retval = 0;

    if(argp == NULL)
    {
        va_start(args,argp);
        argp = &args;
    }

    text = va_arg(*argp,char*);

    if(argp == &args) va_end(args);

    if(text == NULL) return -1;
    if(text[0] == '\0') return -1;

    width = strlen(text);

    VDK_OBJECT(VDK_WIDGET_KLASS)->ctor(object,NULL,width,1);

    VDK_LABEL(object)->text = strdup(text);
    wprintw(*(WINDOW**)object,"%s",text);
    // waddchstr(*(WINDOW**)object,(const chtype*)text);
    wmove(*(WINDOW**)object,0,0);

    return retval;
}

static int
_vdk_label_dtor(vdk_object_t *object)
{
    int             retval;

    if(object == NULL) return -1;

    if(VDK_LABEL(object)->text != NULL)
    {
        free(VDK_LABEL(object)->text);
        VDK_LABEL(object)->text = NULL;
    }

    // call base class destructor
    retval = VDK_OBJECT(VDK_WIDGET_KLASS)->dtor(object);

    return retval;
}
