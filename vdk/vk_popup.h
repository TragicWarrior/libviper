#ifndef _VK_POPUP_H_
#define _VK_POPUP_H_

#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_window.h"
#include "vk_box.h"
#include "vk_button.h"
#include "vk_filler.h"

#define VK_POPUP_MAX_BUTTONS    8

struct _vk_popup_s
{
    vk_window_t     parent_klass;

    int             style;
    int             result;

    vk_box_t        *layout;
    vk_widget_t     *client;
    vk_filler_t     *default_client;
    vk_box_t        *button_bar;
    vk_button_t     *buttons[VK_POPUP_MAX_BUTTONS];
    int             button_count;

    struct
    {
        vk_popup_t  *popup;
        int         index;
    } btn_ctx[VK_POPUP_MAX_BUTTONS];

    int             (*ctor)(vk_object_t *, va_list *, ...);
    int             (*dtor)(vk_object_t *);
};

#endif
