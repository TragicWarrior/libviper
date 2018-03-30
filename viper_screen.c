/*************************************************************************
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 2007      Bryan Christ <bryan.christ@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "viper.h"
#include "private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_screen.h"


WINDOW*
viper_screen_get_wallpaper(int screen_id)
{
    extern VIPER    *viper;
    viper_screen_t  *viper_screen;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    viper_screen = &viper->viper_screen[screen_id];

    return viper_screen->wallpaper;
}

void
viper_screen_set_wallpaper(int screen_id, WINDOW *wallpaper, ViperBkgdFunc agent)
{
    extern VIPER   *viper;
    viper_screen_t  *viper_screen;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    viper_screen = &viper->viper_screen[screen_id];

    viper_screen->wallpaper = wallpaper;
    viper_screen->wallpaper_agent = agent;

    return;
}

WINDOW*
viper_get_screen_window(int screen_id)
{
    extern VIPER    *viper;
    viper_screen_t  *viper_screen;

    if(screen_id == -1)
        screen_id = viper->cur_scr_id;

    viper_screen = &viper->viper_screen[screen_id];

    return viper_screen->screen;
}

int
viper_get_active_screen(void)
{
    extern VIPER    *viper;

    return viper->cur_scr_id;
}

void
viper_screen_reset(int screen_id)
{
    extern VIPER    *viper;
    viper_screen_t  *viper_screen;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    // don't erase and inactive screen
    if(screen_id != CURRENT_SCREEN_ID) return;

    viper_screen = &viper->viper_screen[screen_id];

    if(viper_screen->wallpaper != NULL)
        overwrite(viper_screen->wallpaper, viper_screen->screen);
    else
        werase(viper_screen->screen);

    return;
}

void
viper_screen_redraw(int screen_id, uint32_t update_mask)
{
    extern VIPER    *viper;
    viper_screen_t  *viper_screen;
    uint32_t        state_mask = 0;
    ViperBkgdFunc   wallpaper_agent = NULL;

    // set to current screen if -1;
    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    // don't update any inactive screens
    if(screen_id != CURRENT_SCREEN_ID) return;

    viper_screen = &viper->viper_screen[screen_id];

    /*
        redrawing the background window is a fairly simple operation.  the
        user defined function "bg_func" is called which draws the picture on
        WINDOW bg_window.
    */
    if(update_mask & REDRAW_BACKGROUND)
    {
        if(viper_screen->wallpaper != NULL)
        {
            wallpaper_agent = viper_screen->wallpaper_agent;

            if(wallpaper_agent != NULL) wallpaper_agent(screen_id);
        }
    }

    /*
        the process of redrawing the workspace    begins with copying the user
        defined background (WINDOW bg_window) to the screen.  the convenience
        function for the copy is viper_screen_reset()
    */
    if(update_mask & REDRAW_WORKSPACE) viper_screen_reset(screen_id);


    // blit all the windows matching the state_mask to WINDOW screen_window
    // managed windows get blitted first
    if(update_mask & REDRAW_WINDOWS)
    {
        state_mask |= STATE_VISIBLE;

        // destroy dead windows (windows that have been closed)
        viper_prune_zombie_list();

        // first redraw managed windows
        viper_window_for_each(screen_id, TRUE, VECTOR_BOTTOM_TO_TOP,
            viper_callback_blit_window, (void*)&state_mask);

        // redraw unmanaged windows
        viper_window_for_each(screen_id, FALSE, VECTOR_BOTTOM_TO_TOP,
            viper_callback_blit_window, (void*)&state_mask);
    }


    // draw the mouse on top
    if((update_mask & REDRAW_MOUSE) && (viper->console_mouse != NULL))
        overwrite(viper->console_mouse, viper_screen->screen);

    // copy WINDOW screen_window to the _real_ window
    touchwin(viper_screen->screen);
    wnoutrefresh(viper_screen->screen);
    doupdate();

    return;
}
