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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "viper.h"
#include "list.h"

void
window_decorate(WINDOW *window, char *title, bool border)
{
    int             x,y;
    static char     *term = NULL;

    getmaxyx(window, y, x);

    if(term == NULL) term = getenv("TERM");

    if(border == TRUE)
#ifdef _VIPER_WIDE
        box_set(window, WACS_VLINE, WACS_HLINE);
#else
        box(window, ACS_VLINE, ACS_HLINE);
#endif

    if(title != NULL)
    {
        x = x / 2;
        x = x - (strlen(title) / 2);
        mvwprintw(window, 0, x, title);
    }

    touchwin(window);

    // squelch compiler warnings
    (void)y;

    return;
}

void
window_modify_border(WINDOW *window, int attrs, short colors)
{
    chtype      char_attr;
    int         width, height;
    int         x, y;

    if(window == NULL) return;

    getmaxyx(window, height, width);

    for(y = 0;y < height + 1;y++)
    {
        for(x = 0;x < width + 1;x++)
        {
            if((x % (width - 1) == 0) || (y % (height - 1) == 0))
            {
                char_attr = mvwinch(window, y, x);
                if((char_attr & A_ALTCHARSET) == A_ALTCHARSET)
                    mvwchgat(window, y, x, 1, attrs | A_ALTCHARSET, colors, NULL);
                else
                    mvwchgat(window, y, x, 1, attrs, colors, NULL);
            }
        }
    }

    return;
}
