#ifndef _VK_MENU_H_
#define _VK_MENU_H_

#include <inttypes.h>
#include <stdarg.h>

#include "list.h"

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"

struct _vk_menu_s
{
    vk_listbox_t        parent_klass;

    int                 frame_style;

    int                 (*ctor)             (vk_object_t *, va_list *, ...);
    int                 (*dtor)             (vk_object_t *);

    int                 (*_set_frame)       (vk_menu_t *, int);

    int                 (*_add_separator)   (vk_menu_t *, int);

    int                 (*_update)          (vk_menu_t *);
    int                 (*_reset)           (vk_menu_t *);
};

#endif


