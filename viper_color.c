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

void
viper_color_init(void)
{
    gshort					fg,bg;
	gint					i;
	gint					max_colors;
	struct color_mtx		*matrix;
	gint					hard_pair = -1;
	extern guint32			viper_global_flags;

	start_color();

	/*	in order for fast color indexing to work properly, libviper must assume
		that COLOR_BLACK is always 0 and COLOR_WHITE is always 7.  if this is
		not true, we have to fall back to safe color.	*/
	if(COLOR_BLACK == 0 && COLOR_WHITE == 7)
	{
		viper_global_flags |= VIPER_FASTCOLOR;

		max_colors = COLORS * COLORS;
		if(max_colors > COLOR_PAIRS) max_colors = COLOR_PAIRS;

		for(i = 1;i < max_colors;i++)
		{
			bg = i / COLORS;
			fg = COLORS - (i % COLORS) - 1;
			init_pair(i, fg, bg);
		}

		return;
	}

	viper_global_flags &= ~VIPER_FASTCOLOR;
	matrix =(struct color_mtx*)g_malloc0(COLOR_PAIRS * sizeof(struct color_mtx));
	for(i = 0;i < COLOR_PAIRS;i++)
	{
		matrix[i].fg = i / COLORS;
		matrix[i].bg = i % COLORS;
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

	for(i = 1;i < COLOR_PAIRS;i++) init_pair(i, matrix[i].fg, matrix[i].bg);
	g_free(matrix);

	return;
}

inline gshort
viper_color_pair(gshort fg, gshort bg)
{
	gshort             color_pair;
	gshort             fg_color,bg_color;
	gint               i;
	extern guint32     viper_global_flags;

	if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

	/*	use fast color indexing when possible.	*/
	if(viper_global_flags & VIPER_FASTCOLOR)
	{
		color_pair = (bg*COLORS)+(COLORS-fg-1);
		return color_pair;
	}

	/* safe color indexing (slower)	*/
	for(i = 1;i < COLOR_PAIRS;i++)
	{
		pair_content(i, &fg_color, &bg_color);
		if(fg_color == fg && bg_color == bg) break;
	}

	return i;
}
