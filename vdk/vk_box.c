#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_box.h"
#include "vk_event.h"
#include "vdk_private.h"

static int
_vk_box_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_box_dtor(vk_object_t *object);

static int
_vk_box_layout(vk_widget_t *widget);

static int
_vk_box_recreate(vk_widget_t *widget);

static int
_vk_box_redraw(vk_object_t *object);

static void
_vk_box_place(vk_box_t *box);

static int
_vk_box_kmio(vk_object_t *object, int32_t keystroke);

require_klass(VK_CONTAINER_KLASS);

declare_klass(VK_BOX_KLASS)
{
    .size = KLASS_SIZE(vk_box_t),
    .name = KLASS_NAME(vk_box_t),
    .ctor = _vk_box_ctor,
    .dtor = _vk_box_dtor,
    .parent = &VK_CONTAINER_KLASS,
    .redraw = _vk_box_redraw,
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
vk_box_set_widget(vk_box_t *box, int slot, vk_widget_t *widget, uint32_t flags)
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

        /* what the widget is to take from the box, now and from then on */
        vdk_inherit_attach(bw, widget, flags);

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
vk_box_redraw(vk_box_t *box)
{
    if(box == NULL) return -1;

    return vk_widget_redraw(VK_WIDGET(box));
}

/* destroy exactly this type; see VK_DEFINE_DESTROY */
VK_DEFINE_DESTROY(vk_box_destroy, vk_box_t)

static int
_vk_box_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_box_t    *box;
    int         orientation;
    int         slots;

    VK_CHAIN_CTOR(object, argp, VK_CONTAINER_KLASS);

    orientation = va_arg(*argp, int);
    slots = va_arg(*argp, int);

    if(slots < 1) slots = 1;

    box = VK_BOX(object);

    box->orientation = orientation;
    box->slots = slots;
    box->slot_widgets = calloc(slots, sizeof(vk_widget_t *));
    box->focused_slot = 0;
    box->homogeneous = true;

    box->ctor = _vk_box_ctor;
    box->dtor = _vk_box_dtor;

    VK_WIDGET(box)->_recreate = _vk_box_recreate;
    VK_WIDGET(box)->_layout = _vk_box_layout;

    object->kmio = _vk_box_kmio;

    return 0;
}

static int
_vk_box_dtor(vk_object_t *object)
{
    vk_box_t        *box;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_box_t)) return -1;

    box = VK_BOX(object);

    /* the slots are only the box's view of its children; the children
       themselves are on the container's list and are destroyed with it
       (see _vk_container_dtor) */
    free(box->slot_widgets);
    box->slot_widgets = NULL;

    vk_object_demote(object, vk_container_t);
    vk_container_destroy(VK_CONTAINER(object));

    return 0;
}

static int
_vk_box_layout(vk_widget_t *widget)
{
    vk_box_t        *box;
    vk_widget_t     *child;
    int             i;
    bool            horiz;

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

    /* and each child to its place, so that where things are can be
       read before anything is painted */
    _vk_box_place(box);

    return 0;
}

static int
_vk_box_recreate(vk_widget_t *widget)
{
    vk_box_t    *box;
    int         i;

    if(vdk_widget_reset_canvas(widget) < 0) return -1;

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

/*
    Put every child of the box in its place: its position in the box,
    and for a child that expands in a box packed to natural sizes, its
    size.  Nothing is painted.

    A homogeneous box gives each slot an equal share and centres the
    child in it.  Otherwise each child gets its natural size along the
    box, the ones that expand share what is left over, and a hidden
    child is left out.

    The box's redraw calls this and then copies the children in; its
    layout (vk_widget_layout, and after every resize) calls it too, so
    a program can read where things are without painting anything.
*/
static void
_vk_box_place(vk_box_t *box)
{
    vk_widget_t     *widget = VK_WIDGET(box);
    vk_widget_t     *child;
    int             i;
    int             pos;
    bool            horiz;

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

            pos += slot_size;
        }
    }
}

static int
_vk_box_redraw(vk_object_t *object)
{
    vk_box_t *box = (vk_box_t *)object;
    vk_widget_t     *widget;
    vk_widget_t     *child;
    int             i;

    if(box == NULL) return -1;

    /* what it holds repaints itself first, so that what is copied in
       below is current */
    VK_CONTAINER_KLASS->redraw(object);

    widget = VK_WIDGET(box);
    widget->_erase(widget);

    if(widget->fg >= 0 && widget->bg >= 0)
    {
        vk_widget_fill_pair(widget, L' ', widget->attrs,
            vdk_color_pair(widget->fg, widget->bg));
    }

    /* every child to its place and size, then copied in, slot by slot */
    _vk_box_place(box);

    for(i = 0; i < box->slots; i++)
    {
        child = box->slot_widgets[i];
        if(child == NULL) continue;

        /* a box that packs to natural sizes leaves a hidden child out
           of the arrangement altogether */
        if(!box->homogeneous && !(child->state & VK_STATE_VISIBLE)) continue;

        vk_widget_blit(child);
    }

    return 0;
}
