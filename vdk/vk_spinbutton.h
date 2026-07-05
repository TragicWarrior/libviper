#ifndef _VK_SPINBUTTON_H_
#define _VK_SPINBUTTON_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

/*
    A numeric spin button: a relief value field with increment / decrement
    arrows at its right edge.  The value is a double so the step can be any
    magnitude; it is clamped to [min, max].  Manual text editing is opt-in
    (off by default) -- when on, the field accepts a typed number that is
    parsed and clamped on commit.

    Keyboard is self-contained via the widget's kmio (Up / Down step the
    value; when editable, digits / '.' / '-' / Backspace / arrows / Home /
    End / Enter / Esc drive the inline editor).  Mouse is parent-assisted:
    libviper leaf widgets do not know their absolute screen position, so
    the containing dialog translates a click into widget-local coordinates
    and calls vk_spinbutton_click().
*/

#define VK_SPINBUTTON_EDIT_CAP      64

struct _vk_spinbutton_s
{
    vk_widget_t         parent_klass;

    double              value;
    double              min;
    double              max;
    double              step;
    int                 precision;      /* fractional digits shown */
    int                 border_style;   /* border glyph style (SINGLE/ASCII) */
    int                 field_relief;   /* 3D dir of the value field */
    int                 button_relief;  /* 3D dir of the arrow controls */
    bool                editable;       /* manual text edit allowed */

    bool                editing;        /* inline editor is active */
    char                edit_buf[VK_SPINBUTTON_EDIT_CAP];
    int                 edit_len;
    int                 edit_cursor;
    int                 edit_scroll;

    VkWidgetFunc        on_change;      /* fired when the value changes */
    void                *on_change_arg;

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);

    int                 (*_update)      (vk_spinbutton_t *);
};

#endif
