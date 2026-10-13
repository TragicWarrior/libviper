#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vdk_private.h"

// base klass methods
static int
_vk_container_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_container_dtor(vk_object_t *object);

// super klass methods
static int
_vk_container_add_widget(vk_container_t *container, vk_widget_t *widget);

static int
_vk_container_remove_widget(vk_container_t *container, vk_widget_t *widget);

static int
_vk_container_rotate(vk_container_t *container, int vector);

static int
_vk_container_vacate(vk_container_t *container);

static int
_vk_container_redraw(vk_object_t *object);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_CONTAINER_KLASS)
{
    .size = KLASS_SIZE(vk_container_t),
    .name = KLASS_NAME(vk_container_t),
    .ctor = _vk_container_ctor,
    .dtor = _vk_container_dtor,
    .parent = &VK_WIDGET_KLASS,
    .redraw = _vk_container_redraw,
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

    /* first of all, and once: tell anyone listening that it is going */
    _vk_object_announce_destroy(VK_OBJECT(container));

    container->dtor(VK_OBJECT(container));

    /* destroyed: this used to report -1, the same as a refusal */
    return 0;
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

    return 0;
}

static int
_vk_container_dtor(vk_object_t *object)
{
    vk_container_t      *container;
    struct list_head    *pos;
    struct list_head    *tmp;
    vk_widget_t         *child;

    if(object == NULL) return -1;

    container = VK_CONTAINER(object);

    /*
        A container owns what it holds: every widget still in it is
        destroyed with it, each through the destructor of its own type.
        A child that is itself a container does the same for its own,
        so a whole tree goes with one call.  It is done here, as an
        ordinary step of the container's teardown and while the
        container is still intact -- not in answer to an event.

        Each child is taken off the list before it is destroyed.  A
        child that should outlive the container has to be taken out
        first -- vk_container_remove_widget, vk_container_vacate, or
        the subclass's own call (vk_box_set_widget / vk_frame_set_child
        with NULL).
    */
    list_for_each_safe(pos, tmp, &container->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);

        list_del(&child->list);
        vk_object_dispose(VK_OBJECT(child));
    }

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

/*
    The redraw of a container: everything it holds repaints itself.

    Each child is repainted as whatever it is, through its own redraw,
    so the container needs to know nothing about the types it holds.  A
    child that is itself a container does the same for its own, which
    takes the repaint to the bottom of the tree.

    This paints nothing of the container itself.  A type built on a
    container calls this first and then paints its own canvas and
    copies the children in, so what it copies is current.
*/
static int
_vk_container_redraw(vk_object_t *object)
{
    vk_container_t      *container;
    struct list_head    *pos;
    vk_widget_t         *child;

    if(object == NULL) return -1;

    container = VK_CONTAINER(object);

    /* this repaint follows a resize or a recreate, which every child
       has just been through as well: they are current already */
    if(VK_WIDGET(object)->state & VDK_STATE_CHILDREN_CURRENT) return 0;

    list_for_each(pos, &container->widget_list)
    {
        child = list_entry(pos, vk_widget_t, list);

        /* a child that follows the container's colours or attributes
           is given them as they are now, then paints with them */
        vdk_inherit_apply(VK_WIDGET(container), child);

        vk_widget_redraw(child);
    }

    return 0;
}
