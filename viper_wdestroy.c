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
#include "private.h"
#include "viper_wdestroy.h"
#include "viper_events.h"
#include "list.h"


// this fuction is only to be called internally
int
viper_window_destroy(vwnd_t *vwnd)
{
    viper_event_t       *viper_event;

    if(vwnd != NULL)
    {
        // execute "window-destroy" event if it exists
        viper_event = viper_get_viper_event(vwnd, "window-destroy");
        if(viper_event != NULL)
            viper_event->func(vwnd, viper_event->arg);
        viper_event_del(vwnd, "*");

        // destroy viper_wnd and associated windows
        if(vwnd->user_window != vwnd->window_frame)
        {
            delwin(vwnd->user_window);
            delwin(vwnd->window_frame);
        }
        else delwin(vwnd->window_frame);

        free(vwnd->ctx);
        free(vwnd);
    }

    return 0;
}
