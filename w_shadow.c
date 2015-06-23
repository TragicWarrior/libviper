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

WINDOW* window_create_shadow(WINDOW *window,WINDOW *window_below)
{
	/* WINDOW  *window_below; */
   extern WINDOW  *SCREEN_WINDOW;
	WINDOW 	      *shadow_window;
	gint	         width,height;
	gint	         beg_x,beg_y;
	gint	         x,y;
#ifdef _VIPER_WIDE
	cchar_t	      wcval;
	wchar_t	      wch;
	attr_t	      attrs;
	short	         color_pair;
#else
	chtype	      char_attr;
#endif

	if(window==NULL) return window;

	getmaxyx(window,height,width);
	getbegyx(window,beg_y,beg_x);

	if(window_below==NULL) window_below=SCREEN_WINDOW;
	shadow_window=newwin(height,width,beg_y+1,beg_x+1);

	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++)
		{
#ifdef _VIPER_WIDE
			mvwin_wch(window_below,beg_y+y+1,beg_x+x+1,&wcval);
			getcchar(&wcval,&wch,&attrs,&color_pair,NULL);
			setcchar(&wcval,&wch,attrs,
					viper_color_pair(COLOR_WHITE,COLOR_BLACK),NULL);
			mvwadd_wch(shadow_window,y,x,&wcval);
#else
			char_attr=mvwinch(window_below,beg_y+y+1,beg_x+x+1);
			mvwaddch(shadow_window,y,x,char_attr & A_CHARTEXT);
			if((char_attr & A_ALTCHARSET)==A_ALTCHARSET)
			 	mvwchgat(shadow_window,y,x,1,A_NORMAL | A_ALTCHARSET,
					viper_color_pair(COLOR_WHITE,COLOR_BLACK),NULL);
			else mvwchgat(shadow_window,y,x,1,A_NORMAL,
				viper_color_pair(COLOR_WHITE,COLOR_BLACK),NULL);
#endif
		}
	}

	return shadow_window;
}
