#ifndef _VIPER_MSGBOX
#define _VIPER_MSGBOX

#include <inttypes.h>

/*    default kbd handlers    */
int    viper_kbd_default_MSGBOX_OK(int32_t keystroke, WINDOW *window);

/*    helpers    */
int    calc_msgbox_metrics(char **msg_array, int *width, int *height);

#endif
