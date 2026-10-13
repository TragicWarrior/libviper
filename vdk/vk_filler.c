#include <string.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_filler.h"
#include "vk_event.h"

static int
_vk_filler_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_filler_dtor(vk_object_t *object);

static int
_vk_filler_redraw(vk_object_t *object);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_FILLER_KLASS)
{
    .size = KLASS_SIZE(vk_filler_t),
    .name = KLASS_NAME(vk_filler_t),
    .ctor = _vk_filler_ctor,
    .dtor = _vk_filler_dtor,
    .parent = &VK_WIDGET_KLASS,
    .redraw = _vk_filler_redraw,
};

inline vk_filler_t*
vk_filler_create(void)
{
    vk_filler_t *filler;

    filler = (vk_filler_t*)vk_object_create(VK_FILLER_KLASS, 1, 1);

    if(filler != NULL)
        vk_widget_set_expand(VK_WIDGET(filler));

    return filler;
}

inline int
vk_filler_destroy(vk_filler_t *filler)
{
    if(filler == NULL) return -1;

    /* strictly this type: an object of any other, a derived one
       included, is refused, and the caller is told */
    if(!vk_object_assert(filler, vk_filler_t)) return -1;

    /* first of all, and once: tell anyone listening that it is going.
       refused when one of its own destroy handlers is asking */
    if(_vk_object_announce_destroy(VK_OBJECT(filler)) != 0) return -1;

    filler->dtor(VK_OBJECT(filler));

    return 0;
}

static int
_vk_filler_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_filler_t *filler;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    filler = VK_FILLER(object);

    filler->ctor = _vk_filler_ctor;
    filler->dtor = _vk_filler_dtor;



    return 0;
}

static int
_vk_filler_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_filler_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

/* a filler paints itself as a blank in its own colours */
static int
_vk_filler_redraw(vk_object_t *object)
{
    vk_widget_t *widget = VK_WIDGET(object);

    if(widget->fg >= 0 && widget->bg >= 0)
    {
        vk_widget_fill_pair(widget, L' ', widget->attrs,
            vdk_color_pair(widget->fg, widget->bg));
    }

    return 0;
}
