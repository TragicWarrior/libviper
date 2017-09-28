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
#include <math.h>

#include "viper.h"

int
is_cursor_at(WINDOW *window, uint16_t mask)
{
    extern WINDOW   *SCREEN_WINDOW;
    int             width, height;
    int             cur_x, cur_y;
    uint16_t        hitmap = 0;

    if(window == NULL) window = SCREEN_WINDOW;

    getyx(window, cur_y, cur_x);

    if(cur_x == 0) hitmap |= CURS_LEFT;
    if(cur_y == 0) hitmap |= CURS_TOP;

    getmaxyx(window, height, width);

    if(cur_x == width - 1) hitmap |= CURS_RIGHT;
    if(cur_y == height - 1) hitmap |= CURS_BOTTOM;

    /* no edges detected        */
    if(hitmap == 0) return -1;

    /*    edge-only detection    */
    if(mask == CURS_EDGE && hitmap > 0) return 0;

    if((hitmap & CURS_LEFT) && mask == CURS_LEFT) return cur_y;
    if((hitmap & CURS_RIGHT) && mask == CURS_RIGHT) return cur_y;
    if((hitmap & CURS_TOP) && mask == CURS_TOP) return cur_x;
    if((hitmap & CURS_BOTTOM) && mask == CURS_BOTTOM) return cur_x;

    /*    corner cases    */
    if(hitmap == mask) return 0;

    return -1;
}



void
window_get_center(WINDOW *window, int *x, int *y)
{
    extern WINDOW   *SCREEN_WINDOW;
    int             width, height;
    int             beg_x, beg_y;

    if(window == NULL) window = SCREEN_WINDOW;

    /* get the bottom right coords of the window */
    getmaxyx(window, height, width);
    /* get the top left coords of the window */
    getbegyx(window, beg_y, beg_x);

    if(x != NULL) *x = (width - beg_x) / 2;
    if(y != NULL) *y = (height - beg_y) / 2;

    return;
}

int
window_check_width(WINDOW *window)
{
    extern WINDOW   *SCREEN_WINDOW;
    int             screen_width, screen_height;
    int             beg_x, beg_y;
    int             height, width;

    if(window == NULL) return -1;

    getmaxyx(SCREEN_WINDOW, screen_height, screen_width);

    getbegyx(window, beg_y, beg_x);
    getmaxyx(window, height, width);
    width += beg_x;

    // squelch compiler warnings
    (void)screen_height;
    (void)height;
    (void)beg_y;

    if(width > screen_width) return width - screen_width;
    return 0;
}

int
window_check_height(WINDOW *window)
{
    extern WINDOW   *SCREEN_WINDOW;
    int             screen_width, screen_height;
    int             beg_x, beg_y;
    int             width, height;

    if(window == NULL) return -1;

    getmaxyx(SCREEN_WINDOW, screen_height, screen_width);

    getbegyx(window, beg_y, beg_x);
    getmaxyx(window, height, width);
    height += beg_y;

    // squelch compiler warnings
    (void)screen_width;
    (void)width;
    (void)beg_x;

    if(height > screen_height) return height - screen_height;
    return 0;
}

void
window_get_size_scaled(WINDOW *refrence, int *width, int *height,
    float hscale, float vscale)
{
    extern WINDOW   *SCREEN_WINDOW;
    int             max_width, max_height;

    if(refrence == NULL) refrence = SCREEN_WINDOW;

    getmaxyx(refrence, max_height, max_width);
    /*
        when casting a float to an int, the fractional part will always get
        discarded.  to insure that a fullscreen window can be crated, we simply
        increasing the max_height and max_width by 1.
    */
    max_height++;
    max_width++;
    if(width != NULL) *width = max_width * hscale;
    if(height != NULL) *height = max_height * vscale;

    return;
}
