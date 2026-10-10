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

inline vk_object_t*
vk_object_construct(const void *klass, ...)
{
    vk_object_t     *object;
    va_list         argp;

    if(klass == NULL) return NULL;

    object = calloc(1, VK_OBJECT(klass)->size);

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

inline int
vk_object_destroy(vk_object_t *object)
{
    struct vk_event_handler *handler;
    struct list_head        *pos;
    struct list_head        *n;

    if(!vk_object_assert(object, vk_object_t))
    {
        object->dtor(object);
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
    -- a container destroying its children, for one.  NULL is a no-op.
*/
inline void
vk_object_dispose(vk_object_t *object)
{
    if(object == NULL) return;

    /* a plain object has no destructor of its own to run first */
    if(vk_object_assert(object, vk_object_t))
    {
        vk_object_destroy(object);
        return;
    }

    object->dtor(object);
}
