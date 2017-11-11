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
#include "list.h"

int
viper_mvwin_rel(vwnd_t *vwnd, int vector_x, int vector_y)
{
    int             retval = 0;
    int             x, y;

    if(vector_x == 0 && vector_y == 0) return 1;

    if(vwnd != NULL)
    {
        retval = window_move_rel(vwnd->window_frame, vector_x, vector_y);

        /* this is a hack until mvwin is fixed */
        if((vwnd->ctx->managed == TRUE) && retval != ERR)
        {
            getbegyx(vwnd->window_frame, y, x);
            vwnd->user_window->_begy = y + vwnd->user_window->_pary;
            vwnd->user_window->_begx = x + vwnd->user_window->_parx;
        }
        /* end of hack */

        viper_event_run(vwnd, "window-move");

        viper_screen_redraw(vwnd->ctx->screen_id, REDRAW_ALL);
    }

    return retval;
}

int
viper_mvwin_abs(vwnd_t *vwnd, int x, int y)
{
    int             retval = 0;
    int             beg_x, beg_y;

    getbegyx(vwnd->window_frame, beg_y, beg_x);
    if(x == WPOS_UNCHANGED) x = beg_x;
    if(y == WPOS_UNCHANGED) y = beg_y;

    if(vwnd != NULL)
    {
        retval = mvwin(vwnd->window_frame, y, x);

        /* this is a hack until mvwin is fixed */
        if((vwnd->ctx->managed == TRUE) && retval != ERR)
        {
            getbegyx(vwnd->window_frame, y, x);
            vwnd->user_window->_begy = y + vwnd->user_window->_pary;
            vwnd->user_window->_begx = x + vwnd->user_window->_parx;
        }
        /* end of hack */

        viper_event_run(vwnd, "window-move");

        viper_screen_redraw(vwnd->ctx->screen_id, REDRAW_ALL);
    }

    return retval;
}
