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

#include <stdlib.h>
#include <string.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_screen.h"


WINDOW*	viper_screen_get_wallpaper(void)
{
	extern VIPER   *viper;

	return viper->wallpaper;
}

void viper_screen_set_wallpaper(WINDOW *wallpaper,VIPER_FUNC agent,gpointer arg)
{
	extern VIPER   *viper;

	viper->wallpaper=wallpaper;
	viper->wallpaper_agent=agent;
	viper->wallpaper_arg=arg;

	return;
}

void viper_screen_reset(void)
{
   extern WINDOW  *SCREEN_WINDOW;
	extern VIPER   *viper;

	if(viper->wallpaper!=NULL) overwrite(viper->wallpaper,SCREEN_WINDOW);
	else werase(SCREEN_WINDOW);

	return;
}

inline void viper_screen_redraw(gint32 update_mask)
{
   extern WINDOW  *SCREEN_WINDOW;
	extern VIPER   *viper;
	guint32		   state_mask=0;

   /* redrawing the background window is a fairly simple operation.  the
      user defined function "bg_func" is called which draws the picture on
      WINDOW bg_window. */
	if(update_mask & REDRAW_BACKGROUND)
	{
		if(viper->wallpaper!=NULL && viper->wallpaper_agent!=NULL)
			viper->wallpaper_agent(viper->wallpaper,viper->wallpaper_arg);
	}

   /* the process of redrawing the workspace	begins with copying the user
      defined background (WINDOW bg_window) to the screen.  the convenience
      function for the copy is viper_screen_reset()   */
	if(update_mask & REDRAW_WORKSPACE) viper_screen_reset();

	if(update_mask & REDRAW_WINDOWS_UNMANAGED)
		state_mask |= (STATE_UNMANAGED | STATE_VISIBLE);

	if(update_mask & REDRAW_WINDOWS_MANAGED)
		state_mask |= (STATE_MANAGED | STATE_VISIBLE);

   /* blit all the windows matching the state_mask to WINDOW screen_window.   */
	if(state_mask!=0) viper_window_for_each(viper_callback_blit_window,
		(gpointer)&state_mask,VECTOR_BOTTOM_TO_TOP);

   /* draw the mouse on top.  */
	if((update_mask & REDRAW_MOUSE) && (viper->console_mouse!=NULL))
		overwrite(viper->console_mouse,SCREEN_WINDOW);

   /* copy WINDOW screen_window to the _real_ window  */
   touchwin(SCREEN_WINDOW);
	wnoutrefresh(SCREEN_WINDOW);
	doupdate();

	return;
}
