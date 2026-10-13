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

/*
    The two pieces of every klass that are the same but for a name.

    VK_CHAIN_CTOR(object, argp, PARENT_KLASS)
        the first statement of a constructor: refuse a NULL object or a
        NULL argument list, then run the parent's constructor, which
        takes the arguments that are its own off the front of the list.
        What is left on the list afterwards belongs to this type, read
        in order with  va_arg(*argp, type).

        A constructor is always handed its argument list, by
        vk_object_construct or by the constructor of a type built on
        it.  It is never called with the arguments themselves, so it
        has no va_start or va_end of its own.

    VK_DEFINE_DESTROY(vk_gauge_destroy, vk_gauge_t)
        the whole of a type's public destroy call: refuse NULL and any
        object that is not exactly this type, announce the destruction
        (VK_EVENT_ON_DESTROY, once, and not from one of the object's
        own destroy handlers), run the destructor.  Returns 0 when the
        object was destroyed and -1 when it was not.
*/
#define         VK_CHAIN_CTOR(object, argp, parent_klass) \
                    do \
                    { \
                        if((object) == NULL || (argp) == NULL) return -1; \
                        (parent_klass)->ctor((object), (argp)); \
                    } \
                    while(0)

#define         VK_DEFINE_DESTROY(func, type) \
                    inline int \
                    func(type *object) \
                    { \
                        if(object == NULL) return -1; \
                        if(!vk_object_assert(object, type)) return -1; \
                        if(_vk_object_announce_destroy(VK_OBJECT(object)) != 0) \
                            return -1; \
                        object->dtor(VK_OBJECT(object)); \
                        return 0; \
                    }

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

    /* where the object is in being destroyed (VK_DESTROY_*): so that
       VK_EVENT_ON_DESTROY is emitted only once however the teardown is
       entered, and so that a destroy asked for by one of the handlers
       of that very event can be told apart and refused */
    unsigned char       destroying;
};

/* vk_object_t.destroying */
#define VK_DESTROY_NONE         0       /* alive */
#define VK_DESTROY_ANNOUNCING   1       /* its destroy handlers are running */
#define VK_DESTROY_ANNOUNCED    2       /* they have run; being taken apart */

/*
    Tell whoever is listening that `object` is about to be destroyed:
    VK_EVENT_ON_DESTROY, once per object, before anything of it has been
    taken apart -- a handler sees the whole object, and what it holds.
    Every destroy call makes this its first step; calling it again for
    the same object (as the destructor chain does, layer by layer) does
    nothing.

    Returns 0 when the destroy may go ahead.  Returns -1 when it may
    not: the object's destroy handlers are running at this moment, and
    one of them has asked for the object to be destroyed.  It already
    is being; the call that started it carries on when the handler
    returns.  Going ahead here would free the object underneath both.
*/
int             _vk_object_announce_destroy(vk_object_t *object);

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
