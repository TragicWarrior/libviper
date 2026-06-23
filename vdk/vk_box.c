#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_box.h"
#include "vk_event.h"

static int
_vk_box_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_box_dtor(vk_object_t *object);

static int
_vk_box_on_resize(vk_object_t *object, int event, void *anything);

static int
_vk_box_recreate(vk_widget_t *widget);

static int
_vk_box_update(vk_box_t *box);

static int
_vk_box_kmio(vk_object_t *object, int32_t keystroke);

require_klass(VK_CONTAINER_KLASS);

declare_klass(VK_BOX_KLASS)
{
    .size = KLASS_SIZE(vk_box_t),
    .name = KLASS_NAME(vk_box_t),
    .ctor = _vk_box_ctor,
    .dtor = _vk_box_dtor,
};

inline vk_box_t*
vk_box_create(int width, int height, int orientation, int slots)
{
    vk_box_t    *box;

    if(height == 0 || width == 0) return NULL;
    if(slots < 1) slots = 1;

    box = (vk_box_t*)vk_object_create(VK_BOX_KLASS,
        width, height, orientation, slots);

    return box;
}

inline int
vk_box_set_homogeneous(vk_box_t *box, bool homogeneous)
{
    if(box == NULL) return -1;

    box->homogeneous = homogeneous;

    return 0;
}

inline int
vk_box_set_widget(vk_box_t *box, int slot, vk_widget_t *widget)
{
    vk_container_t  *container;

    if(box == NULL) return -1;

    if(slot < 0 || slot >= box->slots) return -1;

    container = VK_CONTAINER(box);

    if(box->slot_widgets[slot] != NULL)
    {
        container->remove_widget(container, box->slot_widgets[slot]);
    }

    box->slot_widgets[slot] = widget;

    if(widget != NULL)
    {
        vk_widget_t *bw = VK_WIDGET(box);

        container->add_widget(container, widget);
        vk_widget_set_surface(widget, bw->canvas);

        if(box->homogeneous && (widget->state & VK_STATE_EXPAND))
        {
            int pos = 0;
            int slot_size;
            int j;

            for(j = 0; j <= slot; j++)
            {
                if(box->orientation == VK_BOX_HORIZONTAL)
                {
                    slot_size = bw->width / box->slots;
                    if(j == box->slots - 1)
                        slot_size = bw->width - pos;
                }
                else
                {
                    slot_size = bw->height / box->slots;
                    if(j == box->slots - 1)
                        slot_size = bw->height - pos;
                }

                if(j < slot) pos += slot_size;
            }

            if(box->orientation == VK_BOX_HORIZONTAL)
                vk_widget_resize(widget, slot_size, bw->height);
            else
                vk_widget_resize(widget, bw->width, slot_size);
        }
    }

    return 0;
}

inline int
vk_box_get_slot_count(vk_box_t *box)
{
    if(box == NULL) return -1;

    return box->slots;
}

inline vk_widget_t*
vk_box_get_widget(vk_box_t *box, int slot)
{
    if(box == NULL) return NULL;

    if(slot < 0 || slot >= box->slots) return NULL;

    return box->slot_widgets[slot];
}

inline int
vk_box_set_subfocus(vk_box_t *box, int slot)
{
    int old_slot;

    if(box == NULL) return -1;

    if(slot < 0 || slot >= box->slots) return -1;
    if(slot == box->focused_slot) return 0;

    old_slot = box->focused_slot;
    box->focused_slot = slot;

    if(box->slot_widgets[old_slot] != NULL)
        vk_object_emit(VK_OBJECT(box->slot_widgets[old_slot]),
            VK_EVENT_ON_UNFOCUS);

    if(box->slot_widgets[slot] != NULL)
        vk_object_emit(VK_OBJECT(box->slot_widgets[slot]),
            VK_EVENT_ON_FOCUS);

    return 0;
}

inline int
vk_box_get_subfocus(vk_box_t *box)
{
    if(box == NULL) return -1;

    return box->focused_slot;
}

inline int
vk_box_update(vk_box_t *box)
{
    if(box == NULL) return -1;

    return box->_update(box);
}

inline void
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
    box->homogeneous = true;

    box->ctor = _vk_box_ctor;
    box->dtor = _vk_box_dtor;
    box->_update = _vk_box_update;

    vk_object_register_event(VK_OBJECT(box),
        VK_EVENT_ON_RESIZE, _vk_box_on_resize, NULL);
    VK_WIDGET(box)->_recreate = _vk_box_recreate;

    object->kmio = _vk_box_kmio;

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
_vk_box_on_resize(vk_object_t *object, int event, void *anything)
{
    vk_widget_t     *widget = VK_WIDGET(object);
    vk_box_t        *box;
    vk_widget_t     *child;
    int             i;
    bool            horiz;

    (void)event;
    (void)anything;

    box = VK_BOX(widget);
    horiz = (box->orientation == VK_BOX_HORIZONTAL);

    if(box->homogeneous)
    {
        int pos = 0;

        for(i = 0; i < box->slots; i++)
        {
            int slot_size;

            if(horiz)
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

            if(child != NULL && (child->state & VK_STATE_EXPAND))
            {
                if(horiz)
                    vk_widget_resize(child, slot_size, widget->height);
                else
                    vk_widget_resize(child, widget->width, slot_size);
            }

            pos += slot_size;
        }
    }
    else
    {
        int natural_total = 0;
        int expand_count = 0;
        int leftover;
        int expand_size;
        int expand_remain;
        int expand_idx;
        int dimension;

        dimension = horiz ? widget->width : widget->height;

        for(i = 0; i < box->slots; i++)
        {
            child = box->slot_widgets[i];
            if(child == NULL) continue;
            if(!(child->state & VK_STATE_VISIBLE)) continue;

            if(child->state & VK_STATE_EXPAND)
                expand_count++;
            else
                natural_total += horiz ? child->width : child->height;
        }

        leftover = dimension - natural_total;
        if(leftover < 0) leftover = 0;

        expand_size = (expand_count > 0) ? leftover / expand_count : 0;
        expand_remain = (expand_count > 0)
            ? leftover - expand_size * expand_count : 0;
        expand_idx = 0;

        for(i = 0; i < box->slots; i++)
        {
            child = box->slot_widgets[i];
            if(child == NULL) continue;
            if(!(child->state & VK_STATE_VISIBLE)) continue;

            if(child->state & VK_STATE_EXPAND)
            {
                int sz = expand_size;
                expand_idx++;
                if(expand_idx == expand_count)
                    sz += expand_remain;

                if(horiz)
                    vk_widget_resize(child, sz, widget->height);
                else
                    vk_widget_resize(child, widget->width, sz);
            }
        }
    }

    return 0;
}

static int
_vk_box_recreate(vk_widget_t *widget)
{
    vk_box_t    *box;
    int         i;

    widget->canvas = newwin(widget->height, widget->width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

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
_vk_box_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_box_t    *box = VK_BOX(object);
    vk_widget_t *child;

    if(box->focused_slot < 0 || box->focused_slot >= box->slots)
        return -1;

    child = box->slot_widgets[box->focused_slot];
    if(child == NULL) return -1;

    return vk_object_push_keystroke(VK_OBJECT(child), keystroke);
}

static int
_vk_box_update(vk_box_t *box)
{
    vk_widget_t     *widget;
    vk_widget_t     *child;
    int             i;
    int             pos;
    bool            horiz;

    if(box == NULL) return -1;

    widget = VK_WIDGET(box);
    widget->_erase(widget);

    if(widget->fg >= 0 && widget->bg >= 0)
    {
        vk_widget_fill_pair(widget, L' ', widget->attrs,
            vdk_color_pair(widget->fg, widget->bg));
    }

    horiz = (box->orientation == VK_BOX_HORIZONTAL);

    if(box->homogeneous)
    {
        pos = 0;

        for(i = 0; i < box->slots; i++)
        {
            int slot_size;

            if(horiz)
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
                int xoff, yoff;

                child->surface = widget->canvas;

                if(horiz)
                {
                    xoff = (slot_size - child->width) / 2;
                    yoff = (widget->height - child->height) / 2;
                }
                else
                {
                    xoff = (widget->width - child->width) / 2;
                    yoff = (slot_size - child->height) / 2;
                }

                if(xoff < 0) xoff = 0;
                if(yoff < 0) yoff = 0;

                if(horiz)
                    vk_widget_move(child, pos + xoff, yoff);
                else
                    vk_widget_move(child, xoff, pos + yoff);

                vk_widget_draw(child);
            }

            pos += slot_size;
        }
    }
    else
    {
        int natural_total = 0;
        int expand_count = 0;
        int leftover;
        int expand_size;
        int expand_remain;
        int expand_idx;
        int dimension;

        dimension = horiz ? widget->width : widget->height;

        for(i = 0; i < box->slots; i++)
        {
            child = box->slot_widgets[i];
            if(child == NULL) continue;
            if(!(child->state & VK_STATE_VISIBLE)) continue;

            if(child->state & VK_STATE_EXPAND)
                expand_count++;
            else
                natural_total += horiz ? child->width : child->height;
        }

        leftover = dimension - natural_total;
        if(leftover < 0) leftover = 0;

        expand_size = (expand_count > 0) ? leftover / expand_count : 0;
        expand_remain = (expand_count > 0)
            ? leftover - expand_size * expand_count : 0;
        expand_idx = 0;

        pos = 0;

        for(i = 0; i < box->slots; i++)
        {
            int slot_size;

            child = box->slot_widgets[i];
            if(child == NULL) continue;
            if(!(child->state & VK_STATE_VISIBLE)) continue;

            if(child->state & VK_STATE_EXPAND)
            {
                slot_size = expand_size;
                expand_idx++;
                if(expand_idx == expand_count)
                    slot_size += expand_remain;

                if(horiz)
                    vk_widget_resize(child, slot_size, widget->height);
                else
                    vk_widget_resize(child, widget->width, slot_size);
            }
            else
            {
                slot_size = horiz ? child->width : child->height;
            }

            child->surface = widget->canvas;

            if(horiz)
                vk_widget_move(child, pos, 0);
            else
                vk_widget_move(child, 0, pos);

            vk_widget_draw(child);
            pos += slot_size;
        }
    }

    return 0;
}
