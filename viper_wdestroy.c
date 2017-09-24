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

#include "viper.h"
#include "viper_private.h"
#include "list.h"

int
viper_window_destroy(WINDOW *window)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;
    VIPER_EVENT     *viper_event;

    if(list_empty(&viper->wnd_list)) return ERR;

    viper_wnd = viper_get_viper_wnd(window);

    if(viper_wnd != NULL)
    {
        // execute "window-destroy" event if it exists
        viper_event = viper_get_viper_event(window, "window-destroy");
        if(viper_event != NULL)
            viper_event->func(viper_wnd->window, viper_event->arg);
        viper_event_del(viper_wnd->window, "*");

        // destroy viper_wnd and associated windows
        if(viper_wnd->user_window != viper_wnd->window)
        {
            delwin(viper_wnd->user_window);
            delwin(viper_wnd->window);
        }
        else delwin(viper_wnd->window);

        list_del(&viper_wnd->list);
        free(viper_wnd);

        // viper_deck_cycle(VECTOR_BOTTOM_TO_TOP);
        viper_wnd = list_first_entry(&viper->wnd_list, VIPER_WND, list);
        viper_window_focus(viper_wnd->window);
        viper_screen_redraw(REDRAW_ALL);
    }

    return 0;
}
