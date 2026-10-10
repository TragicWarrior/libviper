#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"

// base klass methods
static int
_vk_container_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_container_dtor(vk_object_t *object);

static int
_vk_container_on_destroy(vk_object_t *object, int event, void *anything);

// super klass methods
static int
_vk_container_add_widget(vk_container_t *container, vk_widget_t *widget);

static int
_vk_container_remove_widget(vk_container_t *container, vk_widget_t *widget);

static int
_vk_container_rotate(vk_container_t *container, int vector);

static int
_vk_container_vacate(vk_container_t *container);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_CONTAINER_KLASS)
{
    .size = KLASS_SIZE(vk_container_t),
    .name = KLASS_NAME(vk_container_t),
    .ctor = _vk_container_ctor,
    .dtor = _vk_container_dtor,
};

// create a new widget from scratch
inline vk_container_t*
vk_container_create(int width, int height)
{
    vk_container_t  *container;

    if(height == 0 || width == 0) return NULL;

    container = (vk_container_t*)vk_object_create(VK_CONTAINER_KLASS,
        width, height);

    return container;
}

inline int
vk_container_add_widget(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    container->add_widget(container, widget);

    return 0;
}

inline int
vk_container_remove_widget(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    container->remove_widget(container, widget);

    return 0;
}

inline int
vk_container_vacate(vk_container_t *container)
{
    if(container == NULL) return -1;

    container->vacate(container);

    return 0;
}

inline int
vk_container_destroy(vk_container_t *container)
{
    if(container == NULL) return -1;

    if(!vk_object_assert(container, vk_container_t)) return -1;

    container->dtor(VK_OBJECT(container));

    return -1;
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
    VK_WIDGET_KLASS->ctor(object, argp);

    // install our derived klass methods
    container = VK_CONTAINER(object);
    container->add_widget = _vk_container_add_widget;
    container->remove_widget = _vk_container_remove_widget;
    container->rotate = _vk_container_rotate;
    container->vacate = _vk_container_vacate;

    /* the destructor a derived class hands off to (vk_container_destroy)
       once it has taken its own layer down */
    container->ctor = _vk_container_ctor;
    container->dtor = _vk_container_dtor;

    INIT_LIST_HEAD(&container->widget_list);

    /* every widget that can host others takes part in the destroy
       cascade.  Registered here, in the base class, so that boxes,
       frames, windows, grids and everything built on them inherit it,
       the same way each of them registers for its own resize. */
    vk_object_register_event(object, VK_EVENT_ON_DESTROY,
        _vk_container_on_destroy, NULL);

    return 0;
}

/*
    A container is being destroyed (VK_EVENT_ON_DESTROY, emitted on it
    by the widget layer of its teardown).  It owns what it holds: every
    widget still in it is destroyed, each through the destructor of its
    own type, and a child that is itself a container gets this same
    event and does the same for its children.  Each child is taken off
    the list before it is destroyed.

    A child that should outlive the container has to be taken out first
    -- vk_container_remove_widget, vk_container_vacate, or the
    subclass's own call (vk_box_set_widget / vk_frame_set_child with
    NULL).
*/
static int
_vk_container_on_destroy(vk_object_t *object, int event, void *anything)
{
    vk_container_t      *container = VK_CONTAINER(object);
    struct list_head    *pos;
    struct list_head    *tmp;
    vk_widget_t         *child;

    (void)event;
    (void)anything;

    list_for_each_safe(pos, tmp, &container->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);

        list_del(&child->list);
        vk_object_dispose(VK_OBJECT(child));
    }

    return 0;
}

static int
_vk_container_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    /* the children are not destroyed here but in answer to the destroy
       event the widget layer emits next: see _vk_container_on_destroy */

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_container_add_widget(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    list_add(&widget->list, &container->widget_list);

    return 0;
}

static int
_vk_container_remove_widget(vk_container_t *container, vk_widget_t *widget)
{
    if(container == NULL) return -1;
    if(widget == NULL) return -1;

    list_del(&widget->list);

    return 0;
}

static int
_vk_container_vacate(vk_container_t *container)
{
    struct list_head    *tmp;
    struct list_head    *pos;
    vk_widget_t         *widget;

    if(container == NULL) return -1;

    // iterate over the widget list and remove them all
    list_for_each_safe(pos, tmp, &container->widget_list)
    {
        widget = list_entry(pos, vk_widget_t, list);

        list_del(&widget->list);
    }

    return 0;
}

static int
_vk_container_rotate(vk_container_t *container, int vector)
{
    if(container == NULL) return -1;

    if(vector == VK_VECTOR_LEFT)
    {
        list_rotate_left(&container->widget_list);
        return 0;
    }

    if(vector == VK_VECTOR_RIGHT)
    {
        list_rotate_right(&container->widget_list);
        return 0;
    }

    return 0;
}

