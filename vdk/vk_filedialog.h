#ifndef _VK_FILEDIALOG_H_
#define _VK_FILEDIALOG_H_

#include <stdarg.h>
#include <stdbool.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_box.h"
#include "vk_input.h"
#include "vk_listbox.h"
#include "vk_scroller.h"
#include "vk_button.h"

struct _vk_filedialog_s
{
    vk_box_t            parent_klass;

    int                 style;
    bool                multiselect;
    char                *path;

    vk_input_t          *path_input;
    vk_listbox_t        *file_list;
    vk_scroller_t       *scroller;
    vk_box_t            *button_bar;
    vk_button_t         *btn_ok;
    vk_button_t         *btn_cancel;

    int                 (*ctor)(vk_object_t *, va_list *, ...);
    int                 (*dtor)(vk_object_t *);
};

#endif
