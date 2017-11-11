#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"

static int
_vk_container_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_container_dtor(vk_object_t *object);

static int
_vk_container_add(vk_container_t *container, vk_widget_t *widget);

static int
_vk_container_remove(vk_container_t *container, vk_widget_t *widget);

static vk_object_t VK_CONTAINER_KLASS =
{
    .size = KLASS_SIZE(vk_container_t),
    .name = KLASS_NAME(vk_container_t),
    .ctor = _vk_container_ctor,
    .dtor = _vk_container_dtor,
};

// create a new widget from scratch
vk_container_t*
vk_container_create(int width, int height)
{
    vk_container_t  *container;

    if(height == 0 || width == 0) return NULL;

    container = (vk_container_t*)vk_object_create(VK_CONTAINER_KLASS,
        width, height);

    return container;
}

void
vk_container_add(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return;
    if(widget == NULL) return;

    if(!vk_object_assert(container, vk_container_t)) return;

    container->add(container, widget);

    return;
}

void
vk_container_remove(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return;
    if(widget == NULL) return;

    if(!vk_object_assert(container, vk_container_t)) return;

    container->remove(container, widget);

    return;
}

void
vk_container_destroy(vk_container_t *container)
{
    if(container == NULL) return;

    if(!vk_object_assert(container, vk_container_t)) return;

    container->dtor(VK_OBJECT(container));

    return;
}


static int
_vk_container_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_container_t  *container;
    va_list         args;

    if(object == NULL) return -1;

    /*
        if argp is set then we're being called by a superclass.
        otherwise, we're being called directly.
    */
    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    // call the base klass constructor
    VK_WIDGET(object)->ctor(object, argp);

    // install our derived klass methods
    container = VK_CONTAINER(object);
    container->add = _vk_container_add;
    container->remove = _vk_container_remove;

    INIT_LIST_HEAD(&container->widget_list);

    return 0;
}


static int
_vk_container_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    // todo iterate over container list and destroy

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_container_add(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    list_add(&widget->list, &container->widget_list);

    return 0;
}

static int
_vk_container_remove(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    // list_del(&widget->list, &container->widget_list);

    return 0;
}


