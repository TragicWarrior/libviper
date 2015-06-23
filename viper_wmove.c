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
#include "viper_private.h"

gint viper_mvwin_rel(WINDOW *window,gint vector_x,gint vector_y)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	gint		      retval=0;
	gint		      x,y;

	if(window==NULL) return ERR;
	if(vector_x==0 && vector_y==0) return 1;

	if(viper->wnd_count==0) return ERR;
	viper_wnd=viper_get_viper_wnd(window);

	if(viper_wnd!=NULL)
	{
		retval=window_move_rel(viper_wnd->window,vector_x,vector_y);
		/* this is a hack until mvwin is fixed */
		if((viper_wnd->window_state & STATE_MANAGED) && retval!=ERR)
		{
			getbegyx(viper_wnd->window,y,x);
			viper_wnd->user_window->_begy=y+viper_wnd->user_window->_pary;
			viper_wnd->user_window->_begx=x+viper_wnd->user_window->_parx;
		}
		/* end of hack */
		viper_event_run(window,"window-move");
		viper_screen_redraw(REDRAW_ALL);
	}

	return retval;
}

gint viper_mvwin_abs(WINDOW *window,gint x,gint y)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	gint			   retval=0;
   gint           beg_x,beg_y;

	if(window==NULL) return ERR;

   /* commenting out to allow negative values.  example -1 = unchanged  */

	if(viper->wnd_count==0) return ERR;
	viper_wnd=viper_get_viper_wnd(window);

   getbegyx(viper_wnd->window,beg_y,beg_x);
   if(x==WPOS_UNCHANGED) x=beg_x;
   if(y==WPOS_UNCHANGED) y=beg_y;

	if(viper_wnd!=NULL)
	{
		retval=mvwin(viper_wnd->window,y,x);
		/* this is a hack until mvwin is fixed */
		if((viper_wnd->window_state & STATE_MANAGED) && retval!=ERR)
		{
			getbegyx(viper_wnd->window,y,x);
			viper_wnd->user_window->_begy=y+viper_wnd->user_window->_pary;
			viper_wnd->user_window->_begx=x+viper_wnd->user_window->_parx;
		}
		/* end of hack */
		viper_event_run(window,"window-move");
		viper_screen_redraw(REDRAW_ALL);
	}

	return retval;
}
