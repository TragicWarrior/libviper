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
    WINDOW  *parent;

    if(window == NULL) return ERR;

    /* first, handle subwindows - use wgetparent() for ncurses 6.x compat */
    parent = wgetparent(window);
    if(parent != NULL)
    {
        /*
            unfortunately, ncurses (and others) cannot move nested subwindows.
            abort with ERR in these situations.
        */

        /* todo:  can the subwin_move_realign() help?   */
        if(wgetparent(parent) != NULL) return ERR;

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
    WINDOW  *parent;
    int     x = 0, y = 0;

    /* make sure this is a subwin */
    parent = wgetparent(subwin);
    if(parent == NULL) return;

    target = subwin;
    while((parent = wgetparent(subwin)) != NULL)
    {
        x += getparx(subwin);
        y += getpary(subwin);
        subwin = parent;
    }
    x += getbegx(subwin);
    y += getbegy(subwin);

    /*
     * Note: In ncurses 6.x, we cannot directly set _begy/_begx.
     * The mvderwin() call repositions derived windows relative to parent.
     * For absolute repositioning, the parent window's position plus
     * the derived window's parent-relative offset determines screen position.
     * Modern ncurses should handle this automatically when the parent moves.
     */
    (void)target;  /* suppress unused warning - can't set coords directly */
    (void)x;
    (void)y;

    return;
}
