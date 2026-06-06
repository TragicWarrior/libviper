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
    int retval;

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

    if(object == NULL) return -1;

    list_for_each(pos, &object->event_handlers)
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

