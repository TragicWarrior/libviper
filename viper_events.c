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

#include <string.h>
#include <stdbool.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "list.h"

VIPER_EVENT*
viper_get_viper_event(WINDOW *window, char *event)
{
    extern VIPER    	*viper;
    VIPER_WND       	*viper_wnd;
    VIPER_EVENT     	*viper_event = NULL;
	struct list_head	*pos;
    int                 len;

    if(window == NULL || event == NULL) return NULL;
    if(list_empty(&viper->wnd_list)) return NULL;

    viper_wnd = viper_get_viper_wnd(window);

	// do a bunch of checks before iterating
	if(viper_wnd == NULL) return NULL;
	if(list_empty(&viper_wnd->event_list)) return NULL;

    len = strlen(event);

	list_for_each(pos, &viper_wnd->event_list)
	{
	    viper_event = list_entry(pos, VIPER_EVENT, list);
		if(memcmp(viper_event->event, event, len) == 0) break;

		// invalidate for the next iteration
		viper_event = NULL;
	}

    return viper_event;
}

int
viper_event_set(WINDOW *window, char *event, VIPER_FUNC func, void *arg)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd = NULL;
    VIPER_EVENT     *viper_event = NULL;

    if(window == NULL || event == NULL || func == NULL) return -1;
    if(list_empty(&viper->wnd_list)) return ERR;

    // does the event already exists?
    viper_event = viper_get_viper_event(window, event);

    if(viper_event == NULL)
    {
        viper_wnd = viper_get_viper_wnd(window);
        viper_event = (VIPER_EVENT*)calloc(1, sizeof(VIPER_EVENT));

        list_add(&viper_event->list, &viper_wnd->event_list);
    }

    viper_event->event = event;
    viper_event->func = func;
    viper_event->arg = arg;

    if(viper_wnd == NULL) return ERR;
    return 1;
}

int
viper_event_exec(WINDOW *window, char *event, void *anything)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    VIPER_EVENT         *viper_event;
    struct list_head    *pos;
    bool                broadcast = FALSE;


    if(window == NULL || event == NULL) return ERR;

    /*  if the user wants to send a broadcast event, the value of *window will
        be VIPER_EVENT_BROADCAST.   */

    if(memcmp(window, VIPER_EVENT_BROADCAST,
        strlen((char*)VIPER_EVENT_BROADCAST)) == 0)
    {
        broadcast = TRUE;
    }

    if(list_empty(&viper->wnd_list)) return ERR;

    if(broadcast == TRUE)
    {
        /*
            when doing a broadcast, the bottom most window in the stack Z-order
            will be signaled first.  the last window will be the top window.
        */

        list_for_each_prev(pos, &viper->wnd_list)
        {
            viper_wnd = list_entry(pos, VIPER_WND, list);

            viper_event = viper_get_viper_event(viper_wnd->user_window, event);

            if(viper_event != NULL)
            {
                // if anything is NULL, pass the default data with the event
                if(anything == NULL)
                    viper_event->func(viper_wnd->user_window, viper_event->arg);
                // otherwise, pass the override data
                else
                    viper_event->func(viper_wnd->user_window, anything);
            }
        }
    }
    else
    {
        viper_wnd = viper_get_viper_wnd(window);
        viper_event = viper_get_viper_event(viper_wnd->user_window, event);
        if(viper_event != NULL)
        {
            // if anything is NULL, pass the default data with the event
            if(anything == NULL)
                viper_event->func(viper_wnd->user_window, viper_event->arg);
            // otherwise, pass the override data
            else
                viper_event->func(viper_wnd->user_window, anything);
        }
    }

    return 1;
}


int
viper_event_del(WINDOW *window, char *event)
{
    extern VIPER    	*viper;
    VIPER_WND       	*viper_wnd;
    VIPER_EVENT     	*viper_event = NULL;

	struct list_head	*pos;
    struct list_head    *tmp;

    if(window == NULL || event == NULL) return -1;
    if(list_empty(&viper->wnd_list)) return ERR;

    viper_wnd = viper_get_viper_wnd(window);
    if(list_empty(&viper_wnd->event_list)) return ERR;

    if(event[0] == '*')
    {
		list_for_each_safe(pos, tmp, &viper_wnd->event_list)
		{
			viper_event = list_entry(pos, VIPER_EVENT, list);

			list_del(&viper_event->list);
			free(viper_event);
		}

		return 1;
    }

    viper_event = viper_get_viper_event(window, event);
    if(viper_event != NULL)
    {
		list_del(&viper_event->list);
		free(viper_event);

		return 1;
    }

    return ERR;
}


/*
    these events defined below get automatically added to certain windows
    during initialization
*/
int
viper_event_default_TERM_RESIZE(WINDOW *window, void *arg)
{
    VIPER_WND       *viper_wnd;
    int            	x, y;

    viper_wnd = viper_get_viper_wnd(window);

    x = window_check_height(viper_wnd->window);
    y = window_check_width(viper_wnd->window);

    if(x > 0 || y > 0)
        viper_wresize_abs(window, viper_wnd->min_width, viper_wnd->min_height);

    viper_screen_redraw(REDRAW_ALL);

    (void)arg;

    return 0;
}

int
viper_event_default_WINDOW_CLOSE(WINDOW *window, void *arg)
{

    // suppress compiler warnings
    (void)window;
    (void)arg;

    return VIPER_EVENT_WINDOW_DESIST;
}

/*
    by returning VIPER_EVENT_WINDOW_PERSIST we prevent the user from closing
    the msgbox using the mouse, hotkey, or viper_window_close() function.
*/
int
viper_event_default_MSGBOX_CLOSE(WINDOW *window, void *arg)
{
    beep();

    // suppress compiler warnings
    (void)window;
    (void)arg;

    return VIPER_EVENT_WINDOW_PERSIST;
}
