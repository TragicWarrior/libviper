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

#include "viper_color.h"
#include "viper.h"
#include "macros.h"

short   viper_color_table[] =
            {   COLOR_BLACK, COLOR_RED, COLOR_GREEN,
                COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA,
                COLOR_CYAN, COLOR_WHITE };

int     viper_color_count;

void
viper_color_init(void)
{
    extern short        viper_color_table[];
    extern int          viper_color_count;
    short				fg,bg;
	int					i;
	int					max_colors;
	struct color_mtx	*matrix;
	int					hard_pair = -1;

	start_color();
    viper_color_count = ARRAY_SZ(viper_color_table);

	/*	in order for fast color indexing to work properly, libviper must assume
		that COLOR_BLACK is always 0 and COLOR_WHITE is always 7.  if this is
		not true, we have to fall back to safe color.	*/

    // calculate the size of the matrix
    max_colors = viper_color_count * viper_color_count;

	matrix  = (struct color_mtx*)calloc(1,
        max_colors * sizeof(struct color_mtx));

	for(i = 0;i < max_colors; i++)
	{
		fg = i / viper_color_count;
		bg = viper_color_count - (i % viper_color_count) - 1;

		matrix[i].bg = fg;
		matrix[i].fg = bg;

		/*
            according to ncurses documentation, color pair 0 is assumed to
            be WHITE foreground on BLACK background.  when we discover this
            pair, we need to make sure it gets swapped into index 0 and
            whatever is in index 0 gets put into this location.
        */
		if(matrix[i].fg == COLOR_WHITE && matrix[i].bg == COLOR_BLACK)
            hard_pair = i;
	}

	/*	if hard_pair is no longer -1 then we found the "hard pair" during our
		enumeration process	and we need to do the swap.	*/
	if(hard_pair != -1)
	{
		fg = matrix[0].fg;
		bg = matrix[0].bg;
		matrix[hard_pair].fg = fg;
		matrix[hard_pair].bg = bg;
	}

	for(i = 1; i < max_colors; i++) init_pair(i, matrix[i].fg, matrix[i].bg);
	free(matrix);

	return;
}

short
viper_color_pair(short fg, short bg)
{
    extern int          viper_color_count;
	short               color_pair;
	short               fg_color, bg_color;
	int                 i;
	extern uint32_t     viper_global_flags;

	if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

	/*	use fast color indexing when possible.	*/
	if(viper_global_flags & VIPER_FASTCOLOR)
	{
		color_pair = (bg * viper_color_count) + (viper_color_count - fg -1);
		return color_pair;
	}

	/* safe color indexing (slower)	*/
	for(i = 1; i < COLOR_PAIRS; i++)
	{
		pair_content(i, &fg_color, &bg_color);
		if(fg_color == fg && bg_color == bg) break;
	}

	return i;
}

int
viper_pair_content(short pair, short *fg, short *bg)
{
    extern uint32_t     viper_global_flags;
    extern int          viper_color_count;
    int                 retval = 0;

    if(!(viper_global_flags & VIPER_FASTCOLOR))
    {
        retval = pair_content(pair, fg, bg);
        return retval;
    }

    // this works if VIPER_FASTCOLOR was specified
    if(pair == 0)
    {
        *fg = COLOR_WHITE;
        *bg = COLOR_BLACK;
    }

    *bg = (int)(pair / viper_color_count);
    *fg = (viper_color_count - pair) - (pair % viper_color_count);

    return 0;
}
