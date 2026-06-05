#ifndef _VK_DECK_H_
#define _VK_DECK_H_

#include <inttypes.h>
#include <stdarg.h>

#include "list.h"

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

struct _vk_deck_s
{
    vk_widget_t         parent_klass;

    struct list_head    widget_list;
    bool                shadows;

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);

    int                 (*_update)      (vk_deck_t *);
};

#endif
