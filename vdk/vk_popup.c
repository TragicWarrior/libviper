#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"
#include "vk_window.h"
#include "vk_box.h"
#include "vk_button.h"
#include "vk_filler.h"
#include "vk_popup.h"

static int
_vk_popup_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_popup_dtor(vk_object_t *object);

static int
_popup_button_pressed(vk_widget_t *widget, void *anything);

require_klass(VK_WINDOW_KLASS);

declare_klass(VK_POPUP_KLASS)
{
    .size = KLASS_SIZE(vk_popup_t),
    .name = KLASS_NAME(vk_popup_t),
    .ctor = _vk_popup_ctor,
    .dtor = _vk_popup_dtor,
};

static void
_popup_init_button(vk_popup_t *popup, int idx, const char *text)
{
    vk_button_t *btn;

    btn = vk_button_create(text);

    if(popup->style == VK_BUTTON_BASIC)
        vk_button_set_relief_style(btn, VK_BUTTON_BASIC);
    else if(popup->style == VK_FRAME_ASCII)
        vk_button_set_relief_style(btn, VK_FRAME_ASCII);

    popup->btn_ctx[idx].popup = popup;
    popup->btn_ctx[idx].index = idx;
    vk_button_set_on_press(btn, _popup_button_pressed,
        &popup->btn_ctx[idx]);

    popup->buttons[idx] = btn;
}

inline vk_popup_t*
vk_popup_create(int width, int height, int style, ...)
{
    vk_popup_t      *popup;
    va_list         ap;
    const char      *labels[VK_POPUP_MAX_BUTTONS];
    int             btn_count = 0;
    const char      *label;
    int             interior_w, interior_h;
    int             btn_h;
    int             i;

    if(width < 5 || height < 3) return NULL;

    if(style != VK_FRAME_SINGLE && style != VK_FRAME_ASCII
        && style != VK_BUTTON_BASIC)
        style = VK_FRAME_SINGLE;

    va_start(ap, style);
    while((label = va_arg(ap, const char *)) != NULL)
    {
        if(btn_count < VK_POPUP_MAX_BUTTONS)
            labels[btn_count++] = label;
    }
    va_end(ap);

    popup = (vk_popup_t *)vk_object_create(VK_POPUP_KLASS,
        width, height, style);
    if(popup == NULL) return NULL;

    interior_w = width - 2;
    interior_h = height - 2;
    btn_h = (style == VK_BUTTON_BASIC) ? 1 : 3;

    popup->layout = vk_box_create(interior_w, interior_h,
        VK_BOX_VERTICAL, 2);
    vk_box_set_homogeneous(popup->layout, false);

    popup->default_client = vk_filler_create();
    vk_widget_set_expand(VK_WIDGET(popup->default_client));
    vk_box_set_widget(popup->layout, 0, VK_WIDGET(popup->default_client));

    if(btn_count > 0)
    {
        popup->button_bar = vk_box_create(interior_w, btn_h,
            VK_BOX_HORIZONTAL, btn_count);

        for(i = 0; i < btn_count; i++)
        {
            _popup_init_button(popup, i, labels[i]);
            vk_box_set_widget(popup->button_bar, i,
                VK_WIDGET(popup->buttons[i]));
        }

        popup->button_count = btn_count;

        vk_box_set_widget(popup->layout, 1,
            VK_WIDGET(popup->button_bar));
    }

    vk_window_set_child(VK_WINDOW(popup), VK_WIDGET(popup->layout));

    popup->result = -1;

    return popup;
}

inline int
vk_popup_set_client(vk_popup_t *popup, vk_widget_t *widget)
{
    if(popup == NULL) return -1;

    if(widget != NULL)
    {
        vk_widget_set_expand(widget);
        vk_box_set_widget(popup->layout, 0, widget);
        popup->client = widget;
    }
    else
    {
        if(popup->default_client != NULL)
        {
            vk_box_set_widget(popup->layout, 0,
                VK_WIDGET(popup->default_client));
        }
        popup->client = NULL;
    }

    return 0;
}

inline vk_widget_t*
vk_popup_get_client(vk_popup_t *popup)
{
    if(popup == NULL) return NULL;

    return popup->client;
}

inline int
vk_popup_add_button(vk_popup_t *popup, const char *text)
{
    int     idx;
    int     interior_w;
    int     btn_h;
    int     ww, wh;
    int     i;

    if(popup == NULL || text == NULL) return -1;
    if(popup->button_count >= VK_POPUP_MAX_BUTTONS) return -1;

    idx = popup->button_count;
    _popup_init_button(popup, idx, text);

    if(popup->button_bar == NULL)
    {
        vk_widget_get_metrics(VK_WIDGET(popup), &ww, &wh);
        interior_w = ww - 2;
        btn_h = (popup->style == VK_BUTTON_BASIC) ? 1 : 3;

        popup->button_bar = vk_box_create(interior_w, btn_h,
            VK_BOX_HORIZONTAL, VK_POPUP_MAX_BUTTONS);

        vk_box_set_widget(popup->button_bar, 0,
            VK_WIDGET(popup->buttons[idx]));
        popup->button_count = 1;

        vk_box_set_widget(popup->layout, 1,
            VK_WIDGET(popup->button_bar));

        return 0;
    }

    if(idx < vk_box_get_slot_count(popup->button_bar))
    {
        vk_box_set_widget(popup->button_bar, idx,
            VK_WIDGET(popup->buttons[idx]));
        popup->button_count++;
        return idx;
    }

    {
        vk_box_t        *old_bar = popup->button_bar;
        vk_box_t        *new_bar;
        int             new_count = idx + 1;

        vk_widget_get_metrics(VK_WIDGET(popup), &ww, &wh);
        interior_w = ww - 2;
        btn_h = (popup->style == VK_BUTTON_BASIC) ? 1 : 3;

        new_bar = vk_box_create(interior_w, btn_h,
            VK_BOX_HORIZONTAL, new_count);

        for(i = 0; i < idx; i++)
        {
            vk_box_set_widget(old_bar, i, NULL);
            vk_box_set_widget(new_bar, i,
                VK_WIDGET(popup->buttons[i]));
        }

        vk_box_set_widget(new_bar, idx,
            VK_WIDGET(popup->buttons[idx]));

        vk_box_set_widget(popup->layout, 1, VK_WIDGET(new_bar));

        vk_box_destroy(old_bar);

        popup->button_bar = new_bar;
        popup->button_count = new_count;
    }

    return idx;
}

inline vk_button_t*
vk_popup_get_button(vk_popup_t *popup, int index)
{
    if(popup == NULL) return NULL;
    if(index < 0 || index >= popup->button_count) return NULL;

    return popup->buttons[index];
}

inline int
vk_popup_get_button_count(vk_popup_t *popup)
{
    if(popup == NULL) return 0;

    return popup->button_count;
}

inline vk_box_t*
vk_popup_get_button_bar(vk_popup_t *popup)
{
    if(popup == NULL) return NULL;

    return popup->button_bar;
}

inline int
vk_popup_get_result(vk_popup_t *popup)
{
    if(popup == NULL) return -1;

    return popup->result;
}

inline int
vk_popup_close(vk_popup_t *popup, int result)
{
    if(popup == NULL) return -1;

    popup->result = result;
    vk_object_emit(VK_OBJECT(popup), VK_EVENT_ON_CLOSE);

    return 0;
}

inline int
vk_popup_set_colors(vk_popup_t *popup, short fg, short bg)
{
    if(popup == NULL) return -1;

    if(popup->layout != NULL)
    {
        vk_widget_set_colors(VK_WIDGET(popup->layout), fg, bg);
        vk_widget_fill(VK_WIDGET(popup->layout),
            ' ' | COLOR_PAIR(vdk_color_pair(fg, bg)));
    }

    if(popup->default_client != NULL)
        vk_widget_set_colors(VK_WIDGET(popup->default_client), fg, bg);

    return 0;
}

inline int
vk_popup_set_button_colors(vk_popup_t *popup, short fg, short bg)
{
    int i;

    if(popup == NULL) return -1;

    for(i = 0; i < popup->button_count; i++)
        vk_widget_set_colors(VK_WIDGET(popup->buttons[i]), fg, bg);

    if(popup->button_bar != NULL)
    {
        vk_widget_set_colors(VK_WIDGET(popup->button_bar), fg, bg);
        vk_widget_fill(VK_WIDGET(popup->button_bar),
            ' ' | COLOR_PAIR(vdk_color_pair(fg, bg)));
    }

    return 0;
}

inline int
vk_popup_set_button_attrs(vk_popup_t *popup, attr_t attrs)
{
    int i;

    if(popup == NULL) return -1;

    for(i = 0; i < popup->button_count; i++)
        vk_widget_set_attrs(VK_WIDGET(popup->buttons[i]), attrs);

    return 0;
}

inline int
vk_popup_update(vk_popup_t *popup)
{
    int i;

    if(popup == NULL) return -1;

    for(i = 0; i < popup->button_count; i++)
        vk_button_update(popup->buttons[i]);

    if(popup->button_bar != NULL)
        vk_box_update(popup->button_bar);

    vk_box_update(popup->layout);
    vk_window_update(VK_WINDOW(popup));

    return 0;
}

inline void
vk_popup_destroy(vk_popup_t *popup)
{
    if(popup == NULL) return;

    if(!vk_object_assert(popup, vk_popup_t)) return;

    popup->dtor(VK_OBJECT(popup));
}

static int
_vk_popup_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_popup_t  *popup;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WINDOW_KLASS->ctor(object, argp);

    popup = VK_POPUP(object);

    popup->style = va_arg(*argp, int);

    va_end(args);

    popup->result = -1;
    popup->layout = NULL;
    popup->client = NULL;
    popup->default_client = NULL;
    popup->button_bar = NULL;
    popup->button_count = 0;
    memset(popup->buttons, 0, sizeof(popup->buttons));

    popup->ctor = _vk_popup_ctor;
    popup->dtor = _vk_popup_dtor;

    return 0;
}

static int
_vk_popup_dtor(vk_object_t *object)
{
    vk_popup_t      *popup;
    vk_container_t  *container;
    vk_box_t        *layout;
    vk_box_t        *button_bar;
    vk_filler_t     *default_client;
    vk_button_t     *buttons[VK_POPUP_MAX_BUTTONS];
    int             btn_count;
    int             i;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_popup_t)) return -1;

    popup = VK_POPUP(object);
    container = VK_CONTAINER(object);

    layout = popup->layout;
    button_bar = popup->button_bar;
    default_client = popup->default_client;
    btn_count = popup->button_count;
    memcpy(buttons, popup->buttons, sizeof(popup->buttons));

    if(button_bar != NULL)
    {
        for(i = 0; i < btn_count; i++)
        {
            VK_CONTAINER(button_bar)->remove_widget(
                VK_CONTAINER(button_bar), VK_WIDGET(buttons[i]));
            button_bar->slot_widgets[i] = NULL;
        }
    }

    {
        vk_container_t *lc = VK_CONTAINER(layout);
        int j;

        for(j = 0; j < layout->slots; j++)
        {
            if(layout->slot_widgets[j] != NULL)
            {
                lc->remove_widget(lc, layout->slot_widgets[j]);
                layout->slot_widgets[j] = NULL;
            }
        }
    }

    VK_FRAME(object)->child = NULL;
    container->remove_widget(container, VK_WIDGET(layout));

    vk_object_demote(object, vk_window_t);
    vk_window_destroy(VK_WINDOW(object));

    for(i = 0; i < btn_count; i++)
        vk_button_destroy(buttons[i]);

    if(button_bar != NULL)
        vk_box_destroy(button_bar);

    if(default_client != NULL)
        vk_filler_destroy(default_client);

    vk_box_destroy(layout);

    return 0;
}

static int
_popup_button_pressed(vk_widget_t *widget, void *anything)
{
    struct { vk_popup_t *popup; int index; } *ctx = anything;

    (void)widget;

    vk_popup_close(ctx->popup, ctx->index);

    return 0;
}
