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

#ifdef _VIPER_WIDE
void
window_fill(WINDOW *window, cchar_t *ch, short color, attr_t attr)
#else
void
window_fill(WINDOW *window, chtype ch, short color, attr_t attr)
#endif
{
    int     width, height;
    int     i = 0;

    getmaxyx(window, height, width);
    wmove(window, 0, 0);

    i = width * height;
    while(i)
    {
#ifdef _VIPER_WIDE
        wadd_wch(window, ch);
#else
        waddch(window, ch);
#endif
        i--;
    }

    for(i = 0;i < height;i++) mvwchgat(window, i, 0, -1, attr, color, NULL);

    return;
}
