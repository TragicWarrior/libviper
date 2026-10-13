#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_event.h"

/*
    this is our static template which will be passed into the object
    allocator and also to the constructor.
*/
declare_klass(VK_OBJECT_KLASS)
{
    .name = KLASS_NAME(vk_object_t),
    .size = KLASS_SIZE(vk_object_t),
};
/*
    Fill in what a klass inherits.  A descriptor that names no redraw
    gets its nearest ancestor's, found by walking the parent links.

    The ancestors are settled first, every time, whether or not this
    klass has a redraw of its own: a klass that does may still call
    PARENT_KLASS->redraw, and that must be there even if no object of
    the parent klass itself was ever made.  The walk is as long as the
    klass is deep -- a handful of steps -- and changes nothing once the
    descriptors are complete.
*/
static void
_vk_klass_resolve(vk_object_t *klass)
{
    vk_object_t *parent;

    if(klass == NULL) return;
    if(klass->parent == NULL || *klass->parent == NULL) return;

    parent = *klass->parent;

    _vk_klass_resolve(parent);

    if(klass->redraw == NULL) klass->redraw = parent->redraw;
}


inline vk_object_t*
vk_object_construct(const void *klass, ...)
{
    vk_object_t     *object;
    va_list         argp;

    if(klass == NULL) return NULL;

    object = calloc(1, VK_OBJECT(klass)->size);

    /* a klass that names no redraw of its own takes its parent's.  it
       is settled in the descriptor, before the copy below, so the
       object and anything that later calls  KLASS->redraw  both get it */
    _vk_klass_resolve((vk_object_t *)klass);

    // copy template to newly alloced object
    memcpy(object, klass, sizeof(vk_object_t));
    INIT_LIST_HEAD(&object->event_handlers);

    if(object->ctor != NULL)
    {
        va_start(argp, klass);

        // pass the pointer to the variable argument list structure
        object->ctor(object, &argp);

        va_end(argp);
    }

    return object;
}

inline int
vk_object_set_kmio(vk_object_t *object, VkKmioFunc func)
{
    if(object == NULL) return -1;

    object->kmio = func;

    return 0;
}

inline int
vk_object_push_keystroke(vk_object_t *object, int32_t keystroke)
{
    int retval = -1;        /* default when the target widget has no kmio */

    if(object == NULL) return -1;

    if(object->kmio != NULL)
    {
        retval = object->kmio(object, keystroke);
    }

    return retval;
}

inline int
vk_object_register_event(vk_object_t *object, int event,
    VkEventFunc func, void *anything)
{
    struct vk_event_handler *handler;

    if(object == NULL || func == NULL) return -1;

    handler = malloc(sizeof(struct vk_event_handler));
    if(handler == NULL) return -1;

    handler->event = event;
    handler->func = func;
    handler->anything = anything;

    list_add_tail(&handler->list, &object->event_handlers);

    return 0;
}

inline int
vk_object_unregister_event(vk_object_t *object, int event,
    VkEventFunc func)
{
    struct vk_event_handler *handler;
    struct list_head        *pos;
    struct list_head        *n;

    if(object == NULL || func == NULL) return -1;

    list_for_each_safe(pos, n, &object->event_handlers)
    {
        handler = list_entry(pos, struct vk_event_handler, list);

        if(handler->event == event && handler->func == func)
        {
            list_del(pos);
            free(handler);
            return 0;
        }
    }

    return -1;
}

int
vk_object_emit(vk_object_t *object, int event)
{
    struct vk_event_handler *handler;
    struct list_head        *pos;
    struct list_head        *n;

    if(object == NULL) return -1;

    /*
        walk with the _safe iterator (as unregister_event and destroy already
        do): a handler may unregister itself -- or another handler -- during
        dispatch, freeing its list node; the non-safe walk would then
        dereference the freed node via pos->next.  (Destroying the emitting
        object inside its own handler remains unsupported -- that frees the
        whole list out from under the walk, so callers must defer it, as vwm
        does by emitting ON_CLOSE and destroying only after emit returns.)
    */
    list_for_each_safe(pos, n, &object->event_handlers)
    {
        handler = list_entry(pos, struct vk_event_handler, list);

        if(handler->event == event)
            handler->func(object, event, handler->anything);
    }

    return 0;
}

/* see vk_object.h */
int
_vk_object_announce_destroy(vk_object_t *object)
{
    if(object == NULL) return -1;

    /* announced already: the destructor chain coming back through, one
       layer at a time.  nothing to do, and it may carry on */
    if(object->destroying == VK_DESTROY_ANNOUNCED) return 0;

    /* its destroy handlers are running right now, and one of them has
       asked for the object to be destroyed.  refused: the destroy that
       called those handlers is waiting for them to return and will
       finish the job */
    if(object->destroying == VK_DESTROY_ANNOUNCING) return -1;

    object->destroying = VK_DESTROY_ANNOUNCING;

    vk_object_emit(object, VK_EVENT_ON_DESTROY);

    object->destroying = VK_DESTROY_ANNOUNCED;

    return 0;
}

inline int
vk_object_destroy(vk_object_t *object)
{
    struct vk_event_handler *handler;
    struct list_head        *pos;
    struct list_head        *n;

    if(object == NULL) return -1;

    /* first of all, and once: tell anyone listening that it is going.
       refused when one of its own destroy handlers is asking */
    if(_vk_object_announce_destroy(object) != 0) return -1;

    /*
        still a derived type: its own destructor does the whole job.
        the chain it starts ends back here -- the last destructor demotes
        the object to vk_object_t and calls this function again -- and
        that inner call frees the handlers and the object.  so there is
        nothing left for this call to do, and nothing left that it may
        touch: carrying on would walk a freed handler list and free the
        object a second time.
    */
    if(!vk_object_assert(object, vk_object_t))
    {
        return object->dtor(object);
    }

    list_for_each_safe(pos, n, &object->event_handlers)
    {
        handler = list_entry(pos, struct vk_event_handler, list);
        list_del(pos);
        free(handler);
    }

    free(object);

    return 0;
}

/*
    The name of the object's type, as written in the code: "vk_box_t",
    "vk_window_t".  It is the most derived type for as long as the
    object is alive (destructors step it down through the parent types
    as they run).  The string belongs to the library.  NULL for a NULL
    object.
*/
inline const char*
vk_object_get_klass_name(vk_object_t *object)
{
    if(object == NULL) return NULL;

    return object->name;
}

/*
    Destroy an object of any type, through the destructor of the type
    it actually is.  The typed calls (vk_button_destroy and the rest)
    each refuse an object that is not exactly their type; this is the
    one to use when all that is known is that the pointer is an object
    -- a container destroying its children, for one.

    Returns 0 when the object was destroyed, and -1 when it was not:
    for NULL, and for a call made from one of the object's own destroy
    handlers (the destroy that is running them finishes the job).
*/
inline int
vk_object_dispose(vk_object_t *object)
{
    if(object == NULL) return -1;

    /* first of all, and once: tell anyone listening that it is going.
       refused when one of its own destroy handlers is asking */
    if(_vk_object_announce_destroy(object) != 0) return -1;

    /* a plain object has no destructor of its own to run first */
    if(vk_object_assert(object, vk_object_t))
    {
        return vk_object_destroy(object);
    }

    object->dtor(object);

    return 0;
}
