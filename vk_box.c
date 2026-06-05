#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_box.h"

static int
_vk_box_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_box_dtor(vk_object_t *object);

static int
_vk_box_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_box_on_resize(vk_widget_t *widget);

static int
_vk_box_recreate(vk_widget_t *widget);

static int
_vk_box_update(vk_box_t *box);


require_klass(VK_CONTAINER_KLASS);

declare_klass(VK_BOX_KLASS)
{
    .size = KLASS_SIZE(vk_box_t),
    .name = KLASS_NAME(vk_box_t),
    .ctor = _vk_box_ctor,
    .dtor = _vk_box_dtor,
    .kmio = _vk_box_kmio,
};


vk_box_t*
vk_box_create(int width, int height, int orientation, int slots)
{
    vk_box_t    *box;

    if(height == 0 || width == 0) return NULL;
    if(slots < 1) slots = 1;

    box = (vk_box_t*)vk_object_create(VK_BOX_KLASS,
        width, height, orientation, slots);

    return box;
}

int
vk_box_set_widget(vk_box_t *box, int slot, vk_widget_t *widget)
{
    vk_container_t  *container;

    if(box == NULL) return -1;

    if(!vk_object_assert(box, vk_box_t)) return -1;

    if(slot < 0 || slot >= box->slots) return -1;

    container = VK_CONTAINER(box);

    if(box->slot_widgets[slot] != NULL)
    {
        container->remove_widget(container, box->slot_widgets[slot]);
    }

    box->slot_widgets[slot] = widget;

    if(widget != NULL)
    {
        container->add_widget(container, widget);
        vk_widget_set_surface(widget, VK_WIDGET(box)->canvas);
    }

    return 0;
}

vk_widget_t*
vk_box_get_widget(vk_box_t *box, int slot)
{
    if(box == NULL) return NULL;

    if(!vk_object_assert(box, vk_box_t)) return NULL;

    if(slot < 0 || slot >= box->slots) return NULL;

    return box->slot_widgets[slot];
}

int
vk_box_update(vk_box_t *box)
{
    if(box == NULL) return -1;

    if(!vk_object_assert(box, vk_box_t)) return -1;

    return box->_update(box);
}

void
vk_box_destroy(vk_box_t *box)
{
    if(box == NULL) return;

    if(!vk_object_assert(box, vk_box_t)) return;

    box->dtor(VK_OBJECT(box));
}


static int
_vk_box_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_box_t    *box;
    va_list     args;
    int         orientation;
    int         slots;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_CONTAINER_KLASS->ctor(object, argp);

    orientation = va_arg(*argp, int);
    slots = va_arg(*argp, int);

    va_end(args);

    if(slots < 1) slots = 1;

    box = VK_BOX(object);

    box->orientation = orientation;
    box->slots = slots;
    box->slot_widgets = calloc(slots, sizeof(vk_widget_t *));
    box->focused_slot = 0;

    box->ctor = _vk_box_ctor;
    box->dtor = _vk_box_dtor;
    box->_update = _vk_box_update;

    VK_WIDGET(box)->_on_resize = _vk_box_on_resize;
    VK_WIDGET(box)->_recreate = _vk_box_recreate;

    return 0;
}

static int
_vk_box_dtor(vk_object_t *object)
{
    vk_box_t        *box;
    vk_container_t  *container;
    int             i;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_box_t)) return -1;

    box = VK_BOX(object);
    container = VK_CONTAINER(object);

    for(i = 0; i < box->slots; i++)
    {
        if(box->slot_widgets[i] != NULL)
        {
            container->remove_widget(container, box->slot_widgets[i]);
            box->slot_widgets[i] = NULL;
        }
    }

    free(box->slot_widgets);
    box->slot_widgets = NULL;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_box_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_box_t    *box;

    box = VK_BOX(object);

    if(keystroke == KEY_TAB)
    {
        box->focused_slot = (box->focused_slot + 1) % box->slots;
        return 0;
    }

    if(box->slot_widgets[box->focused_slot] != NULL)
    {
        return vk_object_push_keystroke(
            VK_OBJECT(box->slot_widgets[box->focused_slot]), keystroke);
    }

    return 0;
}

static int
_vk_box_on_resize(vk_widget_t *widget)
{
    vk_box_t        *box;
    vk_widget_t     *child;
    int             i;
    int             slot_size;
    int             pos;

    box = VK_BOX(widget);
    pos = 0;

    for(i = 0; i < box->slots; i++)
    {
        if(box->orientation == VK_BOX_HORIZONTAL)
        {
            slot_size = widget->width / box->slots;
            if(i == box->slots - 1)
                slot_size = widget->width - pos;
        }
        else
        {
            slot_size = widget->height / box->slots;
            if(i == box->slots - 1)
                slot_size = widget->height - pos;
        }

        child = box->slot_widgets[i];

        if(child != NULL)
        {
            if(box->orientation == VK_BOX_HORIZONTAL)
                vk_widget_resize(child, slot_size, widget->height);
            else
                vk_widget_resize(child, widget->width, slot_size);
        }

        pos += slot_size;
    }

    return 0;
}

static int
_vk_box_recreate(vk_widget_t *widget)
{
    vk_box_t    *box;
    int         i;

    widget->canvas = newwin(widget->height, widget->width, 0, 0);

    box = VK_BOX(widget);

    for(i = 0; i < box->slots; i++)
    {
        if(box->slot_widgets[i] != NULL)
        {
            box->slot_widgets[i]->surface = widget->canvas;
            vk_widget_recreate(box->slot_widgets[i]);
        }
    }

    return 0;
}

static int
_vk_box_update(vk_box_t *box)
{
    vk_widget_t     *widget;
    vk_widget_t     *child;
    int             i;
    int             slot_size;
    int             pos;

    if(box == NULL) return -1;

    widget = VK_WIDGET(box);
    widget->_erase(widget);

    pos = 0;

    for(i = 0; i < box->slots; i++)
    {
        if(box->orientation == VK_BOX_HORIZONTAL)
        {
            slot_size = widget->width / box->slots;
            if(i == box->slots - 1)
                slot_size = widget->width - pos;
        }
        else
        {
            slot_size = widget->height / box->slots;
            if(i == box->slots - 1)
                slot_size = widget->height - pos;
        }

        child = box->slot_widgets[i];

        if(child != NULL)
        {
            child->surface = widget->canvas;

            if(box->orientation == VK_BOX_HORIZONTAL)
                vk_widget_move(child, pos, 0);
            else
                vk_widget_move(child, 0, pos);

            vk_widget_draw(child);
        }

        pos += slot_size;
    }

    return 0;
}
