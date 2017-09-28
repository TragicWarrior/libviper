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

int
window_move_rel(WINDOW *window, int vector_x, int vector_y)
{
    int     curr_x, curr_y;

    if(window == NULL) return ERR;

    /* first, handle subwindows     */
    if(window->_parent != NULL)
    {
        /*
            unfortunately, ncurses (and others) cannot move nested subwindows.
            abort with ERR in these situations.
        */

        /* todo:  can the subwin_move_realign() help?   */
        if(window->_parent->_parent != NULL) return ERR;

        getparyx(window, curr_y, curr_x);
        curr_x += vector_x;
        curr_y += vector_y;

        return mvderwin(window, curr_y, curr_x);
    }

    /*    now handle reglar windows    */
    getbegyx(window, curr_y, curr_x);
    curr_x += vector_x;
    curr_y += vector_y;

    return mvwin(window, curr_y, curr_x);
}

void
subwin_move_realign(WINDOW *subwin)
{
    WINDOW  *target;
    int     x = 0, y = 0;

    /* make sure this is a subwin */
    if(subwin->_parent == NULL) return;

    target = subwin;
    while(subwin->_parent != NULL)
    {
        x += subwin->_parx;
        y += subwin->_pary;
        subwin = subwin->_parent;
    }
    x += subwin->_begx;
    y += subwin->_begy;

    subwin = target;
    subwin->_begy = y;
    subwin->_begx = x;

    return;
}
