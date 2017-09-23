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

    if(window == NULL || event == NULL) return NULL;
    if(viper->wnd_count == 0) return NULL;

    viper_wnd = viper_get_viper_wnd(window);

	// do a bunch of checks before iterating
	if(viper_wnd == NULL) return NULL;
	if(viper_wnd->events == NULL) return NULL;
	if(list_empty(&viper_wnd->events->list)) return NULL;

	list_for_each(pos, &viper_wnd->events->list)
	{
	    viper_event = list_entry(pos, VIPER_EVENT, list);
		if(memcmp(viper_event->event, event, strlen(event)) == 0) break;

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

    if(viper->wnd_count == 0) return ERR;

    viper_event = viper_get_viper_event(window, event);
    if(viper_event == NULL)
    {
        viper_wnd = viper_get_viper_wnd(window);
        viper_event = (VIPER_EVENT*)calloc(1, sizeof(VIPER_EVENT));

		if(viper_wnd->events == NULL)
		{
			viper_wnd->events = viper_event;
            INIT_LIST_HEAD(&viper_wnd->events->list);
			list_add(viper_event, &viper_wnd->events->list);
		}
    }

    viper_event->event = event;
    viper_event->func = func;
    viper_event->arg = arg;

    if(viper_wnd == NULL) return ERR;
    return 1;
}

int
viper_event_del(WINDOW *window, char *event)
{
    extern VIPER    	*viper;
    VIPER_WND       	*viper_wnd;
    VIPER_EVENT     	*viper_event = NULL;

	struct list_head	*pos;

    if(window == NULL || event == NULL) return -1;

    if(viper->wnd_count == 0) return ERR;

    viper_wnd = viper_get_viper_wnd(window);
    if(event[0] == '*')
    {
		list_for_each(pos, &viper_wnd->events->list)
		{
			viper_event = list_entry(pos, VIPER_EVENT, list);
			free(viper_event->event);
			if(viper_event->arg != NULL) free(viper_event->arg);

			list_del(&viper_event->list);
			free(viper_event);
		}

		return 1;
    }

    viper_event = viper_get_viper_event(window, event);
    if(viper_event != NULL)
    {
		free(viper_event->event);
		if(viper_event->arg != NULL) free(viper_event->arg);

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

    return 0;
}

int
viper_event_default_WINDOW_CLOSE(WINDOW *window,void *arg)
{
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
    return VIPER_EVENT_WINDOW_PERSIST;
}
