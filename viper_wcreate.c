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

#include <stdbool.h>
#include <math.h>

#include "viper.h"
#include "private.h"
#include "viper_events.h"
#include "viper_wdecorate.h"
#include "list.h"

vwnd_t*
viper_window_create(int screen_id, bool managed, char *title,
    float x, float y, float width, float height)
{
    extern VIPER    *viper;
    viper_screen_t  *viper_screen;
    vwnd_t          *vwnd;
    vctx_t          *vctx;
    int             screen_width, screen_height;
    int             tmp;

    // alloc objects
    vwnd = (vwnd_t*)calloc(1, sizeof(vwnd_t));
    vctx = (vctx_t*)calloc(1, sizeof(vctx_t));

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    viper_screen = &viper->viper_screen[screen_id];

    // configure and set the context;
    vctx->screen_id = screen_id;
    vctx->managed = managed;
    vwnd->ctx = vctx;

    vwnd->title = title;
    vwnd->window_state |= STATE_VISIBLE;

    if(managed == TRUE)
        list_add(&vwnd->list, &viper_screen->managed_list);
    else
        list_add(&vwnd->list, &viper_screen->unmanaged_list);

    /* fetch the dimentions of the active screen. */
    getmaxyx(viper_screen->screen, screen_height, screen_width);

    /* handle special cases for width.  */
    if(width == WSIZE_FULLSCREEN)
    {
        width = screen_width;
        vwnd->min_width = WSIZE_FULLSCREEN;
    }

    // width is a relative fraction
    if(width > 0 && width < 1)
    {
        window_get_size_scaled(CURRENT_SCREEN, &tmp, NULL, width, 0);
        width = tmp;
    }

    /* handle special cases for height. */
    if(height == WSIZE_FULLSCREEN)
    {
        height = screen_height;
        vwnd->min_height = WSIZE_FULLSCREEN;
    }

    // height is a relative fraction
    if(height > 0 && height < 1)
    {
        window_get_size_scaled(CURRENT_SCREEN, NULL, &tmp, 0, height);
        height = tmp;
    }

    /* calculate the absolute coordinates from a decimal specification.  */
    if(x > 0 && x < 1) x = (screen_width - width - 2) * x;
    if(y > 0 && y < 1) y = (screen_height - height - 2) * y;

    if(managed == TRUE)
    {
        if((width + 1) > screen_width) width -= 2;
        if((height + 1) > screen_height) height -= 2;
        vwnd->window_frame = window_create(NULL, x, y, width + 2, height + 2);
        vwnd->window_state |= STATE_SHADOWED;
    }
    else
    {
        vwnd->user_window = window_create(NULL, x, y, width, height);
    }

    /* set minimum limits if not already specified. */
    if(vwnd->min_width == 0) vwnd->min_width = width;
    if(vwnd->min_height == 0) vwnd->min_height = height;

    if(managed == TRUE)
    {
        wbkgdset(vwnd->window_frame, VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));
        vwnd->user_window = window_create(vwnd->window_frame, 1, 1, width, height);
        wbkgdset(vwnd->user_window, VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));
        werase(vwnd->user_window);

        vwnd->border_agent[0] = viper->border_agent[0];
        vwnd->border_agent[1] = viper->border_agent[1];
    }
    else
        vwnd->window_frame = vwnd->user_window;

    INIT_LIST_HEAD(&vwnd->event_list);

    viper_event_set(vwnd, "term-resized",
        viper_event_default_TERM_RESIZE, NULL);

    viper_window_set_top(vwnd);

    return vwnd;
}

int
viper_window_set_limits(vwnd_t *vwnd, int min_width, int min_height,
    int max_width, int max_height)
{
    if(vwnd == NULL) return ERR;

    if(min_width != 0 && min_width != WSIZE_UNCHANGED)
        vwnd->min_width = min_width;

    if(min_height != 0 && min_height != WSIZE_UNCHANGED)
        vwnd->min_height = min_height;

    if(max_width != 0 && max_width != WSIZE_UNCHANGED)
        vwnd->max_width = max_width;

    if(max_height != 0 && max_height != WSIZE_UNCHANGED)
        vwnd->max_height = max_height;

    return 0;
}
