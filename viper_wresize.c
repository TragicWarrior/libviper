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
#include "viper_wdecorate.h"

gint
viper_wresize(WINDOW *window, gint width, gint height, gint8 flags)
{
    extern WINDOW   *SCREEN_WINDOW;
    VIPER_WND       *viper_wnd;
    WINDOW          *copy_pad;
    gint            beg_x, beg_y;
    gint            max_x, max_y;

    /* saftey checks. */
    if(window == NULL) return ERR;
    if(width == 0 && height == 0) return ERR;
    if(width == 0) width = WSIZE_UNCHANGED;
    if(height == 0) height = WSIZE_UNCHANGED;

    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return ERR;
    if((viper_wnd->window_state & STATE_NORESIZE) && !(flags & 1)) return ERR;

    /* create a copy of the window contents before resizing. */
    getmaxyx(viper_wnd->window, max_y, max_x);
    getbegyx(viper_wnd->window, beg_y, beg_x);
    copy_pad = newwin(max_y - 1,max_x - 1, beg_y, beg_x);
    overwrite(viper_wnd->window, copy_pad);

    /* handle special values.  */
    getmaxyx(SCREEN_WINDOW, max_y, max_x);
    if(width == WSIZE_DEFAULT) width = viper_wnd->min_width;
    if(height == WSIZE_DEFAULT) height = viper_wnd->min_height;
    if(width == WSIZE_FULLSCREEN)
    {
        viper_mvwin_abs(viper_wnd->window, WPOS_UNCHANGED, 0);
        width = max_x;
    }
    if(height == WSIZE_FULLSCREEN)
    {
        viper_mvwin_abs(viper_wnd->window, 0, WPOS_UNCHANGED);
        width = max_y;
    }

    wresize(viper_wnd->window, height, width);
    wresize(viper_wnd->user_window, height - 2,width - 2);
    getmaxyx(viper_wnd->window, max_y, max_x);

    werase(viper_wnd->window);
    werase(viper_wnd->user_window);
    overwrite(copy_pad, viper_wnd->window);
    delwin(copy_pad);

    if(viper_wnd->window_state & STATE_MANAGED)
    {
        if(viper_wnd->window_state & STATE_FOCUS)
            viper_event_run(viper_wnd->window, "window-focus");
        else
            viper_event_run(viper_wnd->window, "window-unfocus");
    }
    viper_event_run(viper_wnd->window, "window-resized");

    viper_window_redraw(window);

    return 0;
}

/*
    if you're looking for viper_wresize_abs() you won't find it here.  it is
    a macro for viper_wresize()
*/

gint
viper_wresize_rel(WINDOW *window, gint vector_x, gint vector_y)
{
    VIPER_WND       *viper_wnd;
    gint            width, height;
    gint            max_x, max_y;

    if(window == NULL) return ERR;
    if(vector_x == 0 && vector_y == 0) return 0;

    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return ERR;
    if(viper_wnd->window_state & STATE_NORESIZE) return ERR;

    getmaxyx(viper_wnd->window, max_y, max_x);
    width = max_x + vector_x;
    height = max_y + vector_y;

    return viper_wresize_abs(window, width, height);
}
