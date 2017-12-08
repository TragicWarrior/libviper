#include "list.h"

#include "viper.h"
#include "private.h"
#include "viper_wdestroy.h"

void
viper_window_close(vwnd_t *vwnd)
{
    extern viper_t  *viper;
    int             screen_id;
    int             managed;

    if(vwnd == NULL) return;
    screen_id = vwnd->ctx->screen_id;
    managed = vwnd->ctx->managed;

    viper_event_run(vwnd, "window-close");

    // move the window to the zombie list where destruction takes place
    list_move(&vwnd->list, &viper->zombie_list);
    
    if(managed == TRUE)
        viper_window_set_focus(TOPMOST_MANAGED);

    /*
        now redraw the screen after the window is moved to zombie list.
        this will also trigger an immediate pruning and a proper
        destruction of the window.
    */
    viper_screen_redraw(screen_id, REDRAW_ALL);

    return;
}


