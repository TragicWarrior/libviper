#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_label.h"

static int
_vk_label_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_label_dtor(vk_object_t *object);

static int
_vk_label_update(vk_label_t *label);

static int
_vk_label_recreate(vk_widget_t *widget);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_LABEL_KLASS)
{
    .size = KLASS_SIZE(vk_label_t),
    .name = KLASS_NAME(vk_label_t),
    .ctor = _vk_label_ctor,
    .dtor = _vk_label_dtor,
};

inline vk_label_t*
vk_label_create(int width)
{
    vk_label_t  *label;

    if(width < 1) return NULL;

    label = (vk_label_t*)vk_object_create(VK_LABEL_KLASS, width, 1);

    return label;
}

inline int
vk_label_set_text(vk_label_t *label, const char *text)
{
    if(label == NULL) return -1;

    if(label->text != NULL)
    {
        free(label->text);
        label->text = NULL;
    }

    if(text != NULL)
        label->text = strdup(text);

    return 0;
}

inline const char*
vk_label_get_text(vk_label_t *label)
{
    if(label == NULL) return NULL;

    return label->text;
}

inline int
vk_label_set_justify(vk_label_t *label, int justify)
{
    if(label == NULL) return -1;

    if(justify < VK_JUSTIFY_LEFT || justify > VK_JUSTIFY_CENTER)
        return -1;

    label->justify = justify;

    return 0;
}

inline int
vk_label_update(vk_label_t *label)
{
    if(label == NULL) return -1;

    return label->_update(label);
}

inline void
vk_label_destroy(vk_label_t *label)
{
    if(label == NULL) return;

    if(!vk_object_assert(label, vk_label_t)) return;

    label->dtor(VK_OBJECT(label));
}

static int
_vk_label_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_label_t  *label;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    label = VK_LABEL(object);

    label->text = NULL;
    label->justify = VK_JUSTIFY_LEFT;

    label->ctor = _vk_label_ctor;
    label->dtor = _vk_label_dtor;
    label->_update = _vk_label_update;

    VK_WIDGET(label)->_recreate = _vk_label_recreate;

    return 0;
}

/*
    After a teleport recreate the canvas is empty -- re-render the
    label text so the new SCREEN shows the same content the old one
    did, instead of a blank slot.
*/
static int
_vk_label_recreate(vk_widget_t *widget)
{
    if(widget == NULL) return -1;

    if(widget->composer != widget->canvas)
        delwin(widget->composer);

    widget->canvas = newwin(widget->height, widget->width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

    if(widget->canvas == NULL) return -1;

    return _vk_label_update(VK_LABEL(widget));
}

static int
_vk_label_dtor(vk_object_t *object)
{
    vk_label_t  *label;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_label_t)) return -1;

    label = VK_LABEL(object);

    if(label->text != NULL)
    {
        free(label->text);
        label->text = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_label_update(vk_label_t *label)
{
    vk_widget_t *widget;
    int         text_len;
    int         x;
    int         colors;

    if(label == NULL) return -1;

    widget = VK_WIDGET(label);
    widget->_erase(widget);

    colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg)) | widget->attrs;
    vk_widget_fill(VK_WIDGET(label), ' ' | colors);

    if(label->text == NULL) return 0;

    text_len = strlen(label->text);

    switch(label->justify)
    {
        case VK_JUSTIFY_RIGHT:
            x = widget->width - text_len;
            if(x < 0) x = 0;
            break;

        case VK_JUSTIFY_CENTER:
            x = (widget->width - text_len) / 2;
            if(x < 0) x = 0;
            break;

        default:
            x = 0;
            break;
    }

    wattron(widget->canvas, colors);
    mvwprintw(widget->canvas, 0, x, "%s", label->text);
    wattroff(widget->canvas, colors);

    return 0;
}
