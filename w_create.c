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

WINDOW*
window_create(WINDOW *parent, gint x, gint y, gint width, gint height)
{
    extern WINDOW   *SCREEN_WINDOW;
    WINDOW          *window = NULL;
    static gint     stagger_x = 3;
    static gint     stagger_y = 3;
    gint            max_x, max_y;

    if(x == WPOS_STAGGERED || y == WPOS_STAGGERED)
    {
        stagger_x += 2;
        stagger_y += 2;

        getmaxyx(SCREEN_WINDOW, max_y, max_x);
        if(stagger_x + width > max_x) stagger_x = 1;
        if(stagger_y + height > max_y) stagger_y = 1;
        y = stagger_y;
        x = stagger_x;
    }

    if(parent == NULL) window = newwin(height, width, y, x);
    else window = derwin(parent, height, width, y, x);

    return window;
}
