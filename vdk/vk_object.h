#ifndef _VK_OBJECT_H_
#define _VK_OBJECT_H_

#include "vdk.h"
#include "stdarg.h"
#include "list.h"

#define KLASS_SIZE(klass_size)  sizeof(klass_size)
#define KLASS_NAME(klass_name)  #klass_name

#define         declare_klass(klass) \
                    vk_object_t *klass = &(vk_object_t)

#define         require_klass(klass) \
                    extern vk_object_t *klass

#define         vk_object_create(klass, ...) \
                    vk_object_construct(klass, __VA_ARGS__)

vk_object_t*    vk_object_construct(const void *klass, ...);

#define         vk_object_assert(klass, type) \
                    _assert_klass((char *)VK_OBJECT(klass)->name, \
                    (char *) #type, VK_OBJECT(klass)->size, \
                    sizeof(type))

#define         vk_object_demote(object, type) \
                    _demote_klass(VK_OBJECT(object), \
                    (char *) #type, sizeof(type))

struct _vk_object_s
{
    size_t              size;
    const char          *name;
    struct list_head    event_handlers;
    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);
    int                 (*kmio)         (vk_object_t *, int32_t);

    /*
        paint the object's contents onto its own canvas.  A klass that
        names none takes its parent's (vk_object_construct works that
        out through `parent`), so every widget has one.  A klass reaches
        its parent's with  PARENT_KLASS->redraw(object)  -- first, and
        then paints its own on top.
    */
    int                 (*redraw)       (vk_object_t *);

    /* the klass this one is built on: the address of that klass's
       descriptor pointer (&VK_WIDGET_KLASS).  NULL for vk_object_t */
    vk_object_t         **parent;

    /* set once VK_EVENT_ON_DESTROY has been emitted for this object,
       so that it is emitted only once however the teardown is entered */
    bool                destroying;
};

/*
    Tell whoever is listening that `object` is about to be destroyed:
    VK_EVENT_ON_DESTROY, once per object, before anything of it has been
    taken apart -- a handler sees the whole object, and what it holds.
    Every destroy call makes this its first step; calling it again for
    the same object (as the destructor chain does, layer by layer) does
    nothing.
*/
void            _vk_object_announce_destroy(vk_object_t *object);

static inline int
_assert_klass(char *kn1, char *kn2, int ks1, int ks2)
{
    if((strcmp(kn1, kn2) != 0) || (ks1 != ks2)) return 0;

    return 1;
}

static inline void
_demote_klass(vk_object_t *object, char *kn, int ks)
{
    object->name = kn;
    object->size = ks;

    return;
}

#endif
