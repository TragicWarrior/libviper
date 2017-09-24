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

#include <math.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_events.h"
#include "viper_wdecorate.h"
#include "list.h"

WINDOW*
viper_window_create(char *title, float x, float y,
    float width, float height, gboolean managed)
{
    extern VIPER    *viper;
    extern WINDOW   *SCREEN_WINDOW;
    VIPER_WND       *viper_wnd;
    int             screen_width, screen_height;
    int             tmp;

    /* initialize window */
    viper_wnd = (VIPER_WND*)g_malloc0(sizeof(VIPER_WND));
    viper_wnd->title = title;
    viper_wnd->window_state |= STATE_VISIBLE;
    list_add(&viper_wnd->list, &viper->wnd_list); 

    /* fetch the dimentions of the active screen. */
    getmaxyx(SCREEN_WINDOW, screen_height, screen_width);

    /* handle special cases for width.  */
    if(width == WSIZE_FULLSCREEN)
    {
        width = screen_width;
        viper_wnd->min_width = WSIZE_FULLSCREEN;
    }
    if(width > 0 && width < 1)
    {
        window_get_size_scaled(SCREEN_WINDOW, &tmp, NULL, width, 0);
        width = tmp;
    }

    /* handle special cases for height. */
    if(height == WSIZE_FULLSCREEN)
    {
        height = screen_height;
        viper_wnd->min_height = WSIZE_FULLSCREEN;
    }
    if(height > 0 && height < 1)
    {
        window_get_size_scaled(SCREEN_WINDOW, NULL, &tmp, 0, height);
        height = tmp;
    }

    /* calculate the absolute coordinates from a decimal specification.  */
    if(x > 0 && x < 1) x = (screen_width - width - 2) * x;
    if(y > 0 && y < 1) y = (screen_height - height - 2) * y;

    if(managed == TRUE)
    {
        if((width + 1) > screen_width) width -= 2;
        if((height + 1) > screen_height) height -= 2;
        viper_wnd->window = window_create(NULL, x, y,width + 2,height + 2);
        viper_wnd->window_state |= STATE_MANAGED;
        viper_wnd->window_state |= STATE_SHADOWED;
    }
    else
    {
        viper_wnd->window = window_create(NULL, x, y, width, height);
        viper_wnd->window_state |= STATE_UNMANAGED;
    }

    /* block window resizing by default.   */
    viper_wnd->window_state |= STATE_NORESIZE;

    /* set minimum limits if not already specified. */
    if(viper_wnd->min_width == 0) viper_wnd->min_width = width;
    if(viper_wnd->min_height == 0) viper_wnd->min_height = height;

    if(managed == TRUE)
    {
        viper_wnd->window_state |= STATE_MANAGED;
        wbkgdset(viper_wnd->window, VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));
        viper_wnd->user_window = window_create(viper_wnd->window, 1, 1,
            width, height);
        wbkgdset(viper_wnd->user_window, VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));
        werase(viper_wnd->user_window);

        viper_wnd->border_agent[0] = viper->border_agent[0];
        viper_wnd->border_agent[1] = viper->border_agent[1];
    }
    else viper_wnd->user_window = viper_wnd->window;

    INIT_LIST_HEAD(&viper_wnd->event_list);

    viper_event_set(viper_wnd->window, "window-close",
        viper_event_default_WINDOW_CLOSE, NULL);
    viper_event_set(viper_wnd->window, "term-resized",
        viper_event_default_TERM_RESIZE, NULL);

    return viper_wnd->user_window;
}

gint
viper_window_set_limits(WINDOW *window, gint min_width, gint min_height,
    gint max_width, gint max_height)
{
    VIPER_WND       *viper_wnd;

    if(window == NULL) return ERR;

    viper_wnd = viper_get_viper_wnd(window);

    if(viper_wnd == NULL) return ERR;

    if(min_width != 0 && min_width != WSIZE_UNCHANGED)
        viper_wnd->min_width = min_width;
    if(min_height != 0 && min_height != WSIZE_UNCHANGED)
        viper_wnd->min_height=min_height;
    if(max_width != 0 && max_width != WSIZE_UNCHANGED)
        viper_wnd->max_width = max_width;
    if(max_height != 0 && max_height != WSIZE_UNCHANGED)
        viper_wnd->max_height = max_height;

    return 0;
}
