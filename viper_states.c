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
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_states.h"


void viper_window_set_state(WINDOW *window,guint32 state)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;

	if(window==NULL) return;

	if(viper->wnd_count==0) return;
	viper_wnd=viper_get_viper_wnd(window);

   /* this is really the worst way to handle bitmask changes, but for the
      time being it will remain. */

	if(state & STATE_UNSET)
	{
 		if(state & STATE_EMINENT)
			viper_wnd->window_state &= ~STATE_EMINENT;

      if(state & STATE_NORESIZE)
         viper_wnd->window_state &= ~STATE_NORESIZE;

  		if(state & STATE_SHADOWED)
			viper_wnd->window_state &= ~STATE_SHADOWED;

		if(state & STATE_VISIBLE)
		{
			viper_wnd->window_state &= ~STATE_VISIBLE;
			viper_deck_cycle(VECTOR_BOTTOM_TO_TOP);
		}

		viper_screen_redraw(REDRAW_ALL);
		return;
	}

	/*	in order to set window focus we need to clear all windows of
		the focus state flag.  then we set the flag as we would any other
		flag.  finally, we redraw the deck */
	if(state & STATE_FOCUS)
	{
		viper_window_for_each(viper_callback_change_focus,(gpointer)window,
			VECTOR_TOP_TO_BOTTOM);
		viper_screen_redraw(REDRAW_ALL);
	}

	if(state & STATE_EMINENT)
	{
		viper_window_for_each(viper_callback_change_eminency,(gpointer)window,
			VECTOR_TOP_TO_BOTTOM);
		viper_window_set_top(window);
		viper_window_redraw(window);
	}

   viper_wnd->window_state |= state;

	return;
}

guint32 viper_window_get_state(WINDOW *window)
{
	extern VIPER   *viper;
   VIPER_WND      *viper_wnd;

   if(window==NULL) return 0;

   if(viper->wnd_count==0) return 0;
   viper_wnd=viper_get_viper_wnd(window);

	return viper_wnd->window_state;
}


void viper_window_show(WINDOW *window)
{
	VIPER_WND	*viper_wnd;
	
	if(window==NULL) return;
	viper_wnd=viper_get_viper_wnd(window);
	if(viper_wnd==NULL) return;
		
	if((viper_wnd->window_state & STATE_VISIBLE)==FALSE)
		viper_window_unhide(window);
	
	viper_window_redraw(window);
}

void viper_window_redraw(WINDOW *window)
{
   extern WINDOW  *SCREEN_WINDOW;
	extern VIPER   *viper;
	VIPER_WND	   *redraw_wnd;
	VIPER_WND	   *next_wnd;
	GSList		   *list_copy;
	GSList		   *node;
		
	if(window==NULL) return;
/*	viper=viper_get_instance(); */
	
	/*	make sure the window is not a subwin, if so, traverse to top and
		change reference.  not sure about the portablility.  relies on a
		consistent implementation of struct win_st	*/
	while(window->_parent!=NULL) window=window->_parent;
	redraw_wnd=viper_get_viper_wnd(window);

	list_copy=g_slist_copy(viper->wnd_list);
	list_copy=g_slist_reverse(list_copy);
	node=list_copy;
	while(node->data!=redraw_wnd) node=node->next;
	
	/*	redrawing a window is somewhat complex.  if you just redraw the
		window, and the window is not on the top of the deck, the operation
		will occlude other windows and the rendered z-order will be wrong.
		all windows which are on top and overlap, need to be redrawn.  of
		course redrawing those windows could occlude other windows.  in order
		to deal with this problem, this function is recursive.  by setting
		the viper->redraw_catalyst, we tag the original window so that only after
		the parent instance of this function completes do we copy the virtual
		screen out to the terminal.	*/
	if(viper->redraw_catalyst==NULL) viper->redraw_catalyst=window;
	viper_callback_blit_window(redraw_wnd->window,NULL);
	node=node->next;
	
	while(node!=NULL)
	{
		next_wnd=(VIPER_WND*)node->data;
		if(viper_deck_check_occlusion(next_wnd,redraw_wnd)==TRUE)
			viper_window_redraw(next_wnd->window);
		node=node->next;
	}
		
	g_slist_free(list_copy);
		
	/*	only copy the virtual screen to the terminal *IF* this is the orignal
		instance of this recursive function.	*/
	if(window==viper->redraw_catalyst)
	{
		if(viper->console_mouse!=NULL) 
         overwrite(viper->console_mouse,SCREEN_WINDOW);
		wnoutrefresh(SCREEN_WINDOW);
		doupdate();
		viper->redraw_catalyst=NULL;
	}
	return;
}
	
gboolean is_viper_window_allowed_focus(WINDOW *window)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	
	if(window==NULL) return FALSE;
		
/*	viper=viper_get_instance(); */
	if(viper->wnd_count==0) return FALSE;
	viper_wnd=viper_get_viper_wnd(window);
	
	/*	notice that an unmanaged window can be moved to the top of the deck
		if it has the emmient bit set.  this trump case allows for special
		cases like a screensaver	*/
	if(viper_wnd->window_state & STATE_EMINENT) return TRUE;
	
	if((viper_wnd->window_state & STATE_MANAGED) &&
		(viper_wnd->window_state & STATE_VISIBLE)) return TRUE;
		
	return FALSE;
}
