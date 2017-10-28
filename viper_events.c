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
#include "viper_events.h"
#include "viper_wdestroy.h"
#include "list.h"

VIPER_EVENT*
viper_get_viper_event(vwnd_t *vwnd, char *event)
{
    VIPER_EVENT     	*viper_event = NULL;
	struct list_head	*pos;
    int                 len;

    if(vwnd == NULL || event == NULL) return NULL;

	// do a check before iterating
	if(list_empty(&vwnd->event_list)) return NULL;

    len = strlen(event);

	list_for_each(pos, &vwnd->event_list)
	{
	    viper_event = list_entry(pos, VIPER_EVENT, list);
		if(memcmp(viper_event->event, event, len) == 0) break;

		// invalidate for the next iteration
		viper_event = NULL;
	}

    return viper_event;
}

int
viper_event_set(vwnd_t *vwnd, char *event, ViperFunc func, void *arg)
{
    VIPER_EVENT         *viper_event = NULL;

    if(vwnd == NULL || event == NULL || func == NULL) return -1;

    // does the event already exists?
    viper_event = viper_get_viper_event(vwnd, event);

    if(viper_event == NULL)
    {
        viper_event = (VIPER_EVENT*)calloc(1, sizeof(VIPER_EVENT));

        list_add(&viper_event->list, &vwnd->event_list);
    }

    viper_event->event = event;
    viper_event->func = func;
    viper_event->arg = arg;

    return 1;
}

int
viper_event_exec(vwnd_t *vwnd, char *event, void *anything)
{
    extern VIPER        *viper;
    VIPER_EVENT         *viper_event;
    struct list_head    *pos;
    struct list_head    *wnd_list;
    bool                broadcast = FALSE;
    int                 i;

    if(vwnd == NULL || event == NULL) return ERR;

    /*  if the user wants to send a broadcast event, the value of *window will
        be VIPER_EVENT_BROADCAST.   */

    if(memcmp(vwnd, VIPER_EVENT_BROADCAST,
        strlen((char*)VIPER_EVENT_BROADCAST)) == 0)
    {
        broadcast = TRUE;
    }

    if(broadcast == TRUE)
    {
        // iterate through all the screens and their 2 lists
        for(i = 0; i < (MAX_SCREENS *2); i++)
        {
            if(i < MAX_SCREENS)
                wnd_list = &viper->managed_list[i];
            else
                wnd_list = &viper->unmanaged_list[i % 2];

            /*
                when doing a broadcast, the bottom most window in the
                stack Z-order will be signaled first.  the last window
                will be the top window.
            */
            list_for_each_prev(pos, wnd_list)
            {
                vwnd = list_entry(pos, vwnd_t, list);

                viper_event = viper_get_viper_event(vwnd, event);

                if(viper_event != NULL)
                {
                    // if anything is NULL, pass the default data with the event
                    if(anything == NULL)
                        viper_event->func(vwnd, viper_event->arg);
                    // otherwise, pass the override data
                    else
                        viper_event->func(vwnd, anything);
                }
            }
        }
    }
    else
    {
        viper_event = viper_get_viper_event(vwnd, event);
        if(viper_event != NULL)
        {
            // if anything is NULL, pass the default data with the event
            if(anything == NULL)
                viper_event->func(vwnd, viper_event->arg);
            // otherwise, pass the override data
            else
                viper_event->func(vwnd, anything);
        }
    }

    return 1;
}


int
viper_event_del(vwnd_t *vwnd, char *event)
{
    VIPER_EVENT     	*viper_event = NULL;

	struct list_head	*pos;
    struct list_head    *tmp;

    if(vwnd == NULL || event == NULL) return -1;

    if(list_empty(&vwnd->event_list)) return ERR;

    if(event[0] == '*')
    {
		list_for_each_safe(pos, tmp, &vwnd->event_list)
		{
			viper_event = list_entry(pos, VIPER_EVENT, list);

			list_del(&viper_event->list);
			free(viper_event);
		}

		return 1;
    }

    viper_event = viper_get_viper_event(vwnd, event);
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
viper_event_default_TERM_RESIZE(vwnd_t *vwnd, void *arg)
{
    int            	x, y;

    x = window_check_height(WINDOW_FRAME(vwnd));
    y = window_check_width(WINDOW_FRAME(vwnd));

    if(x > 0 || y > 0)
        viper_wresize_abs(vwnd, vwnd->min_width, vwnd->min_height);

    viper_screen_redraw(vwnd->ctx->screen_id, REDRAW_ALL);

    (void)arg;

    return 0;
}

int
viper_event_default_WINDOW_CLOSE(vwnd_t *vwnd, void *arg)
{
    viper_window_destroy(vwnd);

    (void)arg;

    return 0;
}

int
viper_event_default_MSGBOX_CLOSE(vwnd_t *vwnd, void *arg)
{
    beep();

    (void)vwnd;
    (void)arg;

    return 0;
}
