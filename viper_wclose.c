

#include "viper.h"

void
viper_window_close(vwnd_t *vwnd)
{
    if(vwnd == NULL) return;

    viper_event_run(vwnd, "window-close"));

    viper_window_destroy(vwnd);

    return;
}


