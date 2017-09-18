#ifndef _VIPER_MSGBOX
#define _VIPER_MSGBOX

#include <glib.h>

/*    default kbd handlers    */
gint    viper_kbd_default_MSGBOX_OK(gint32 keystroke, WINDOW *window);

/*    helpers    */
gint    calc_msgbox_metrics(gchar **msg_array, gint *width, gint *height);

#endif
