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

#include <inttypes.h>

#include "viper.h"
#include "private.h"
#include "viper_wdecorate.h"

int
viper_wresize(vwnd_t *vwnd, int width, int height)
{
    WINDOW          *copy_pad;
    int             beg_x, beg_y;
    int             max_x, max_y;

    /* saftey checks. */
    if(vwnd == NULL) return ERR;
    if(width == 0 && height == 0) return ERR;
    if(width == 0) width = WSIZE_UNCHANGED;
    if(height == 0) height = WSIZE_UNCHANGED;

    if(vwnd->window_state & STATE_NORESIZE) return ERR;

    // don't allow resizing if the window is on an inactive screen
    if(vwnd->ctx->screen_id != CURRENT_SCREEN_ID) return ERR;

    /* create a copy of the window contents before resizing. */
    getmaxyx(vwnd->window_frame, max_y, max_x);
    getbegyx(vwnd->window_frame, beg_y, beg_x);
    copy_pad = newwin(max_y - 1,max_x - 1, beg_y, beg_x);
    overwrite(vwnd->window_frame, copy_pad);

    /* handle special values.  */
    getmaxyx(CURRENT_SCREEN, max_y, max_x);
    if(width == WSIZE_FULLSCREEN)
    {
        viper_mvwin_abs(vwnd, WPOS_UNCHANGED, 0);
        width = max_x;
    }
    if(height == WSIZE_FULLSCREEN)
    {
        viper_mvwin_abs(vwnd, 0, WPOS_UNCHANGED);
        height = max_y;
    }

    if(width == WSIZE_DEFAULT) width = vwnd->min_width;
    if(height == WSIZE_DEFAULT) height = vwnd->min_height;

    wresize(vwnd->window_frame, height, width);
    werase(vwnd->window_frame);

    if(vwnd->ctx->managed == TRUE)
    {
        wresize(vwnd->user_window, height - 2, width - 2);
        werase(vwnd->user_window);
    }

    getmaxyx(vwnd->window_frame, max_y, max_x);

    overwrite(copy_pad, vwnd->window_frame);
    delwin(copy_pad);

    if(vwnd->ctx->managed == TRUE)
    {
        if(vwnd->window_state & STATE_FOCUS)
            viper_event_run(vwnd, "window-focus");
        else
            viper_event_run(vwnd, "window-unfocus");
    }
    viper_event_run(vwnd, "window-resized");

    viper_window_redraw(vwnd);

    return 0;
}

/*
    if you're looking for viper_wresize_abs() you won't find it here.  it is
    a macro for viper_wresize()
*/

int
viper_wresize_rel(vwnd_t *vwnd, int vector_x, int vector_y)
{
    int             width, height;
    int             max_x, max_y;

    if(vwnd == NULL) return ERR;
    if(vector_x == 0 && vector_y == 0) return 0;

    if(vwnd->window_state & STATE_NORESIZE) return ERR;

    // don't allow resizing if the window is on an inactive screen
    if(vwnd->ctx->screen_id != CURRENT_SCREEN_ID) return ERR;

    getmaxyx(vwnd->window_frame, max_y, max_x);
    width = max_x + vector_x;
    height = max_y + vector_y;

    return viper_wresize_abs(vwnd, width, height);
}

