#include <string.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_filler.h"

static int
_vk_filler_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_filler_dtor(vk_object_t *object);


require_klass(VK_WIDGET_KLASS);

declare_klass(VK_FILLER_KLASS)
{
    .size = KLASS_SIZE(vk_filler_t),
    .name = KLASS_NAME(vk_filler_t),
    .ctor = _vk_filler_ctor,
    .dtor = _vk_filler_dtor,
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

inline void
vk_filler_destroy(vk_filler_t *filler)
{
    if(filler == NULL) return;

    if(!vk_object_assert(filler, vk_filler_t)) return;

    filler->dtor(VK_OBJECT(filler));
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
