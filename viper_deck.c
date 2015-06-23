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

#include <string.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_screen.h"
#include "viper_states.h"

WINDOW* viper_deck_hit_test(gint x,gint y)
{
	extern VIPER	*viper;
	VIPER_WND	   *viper_wnd;
	GSList		   *node;

	if(viper->wnd_count==0) return NULL;

	node=viper->wnd_list;
	while(node!=NULL)
	{
		viper_wnd=(VIPER_WND*)node->data;
		if(wenclose(viper_wnd->window,y,x)==TRUE) break;
		node=node->next;
	}

	if(node==NULL) return NULL;
	return viper_wnd->user_window;
}

WINDOW* viper_window_get_top(guint32 state_mask)
{
   extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	GSList		   *node;

	if(viper->wnd_count==0) return NULL;
	if(state_mask==0)
	{
		viper_wnd=(VIPER_WND*)viper->wnd_list->data;
		return viper_wnd->user_window;
	}

	node=viper->wnd_list;
	while(node!=NULL)
	{
		viper_wnd=(VIPER_WND*)node->data;
		if(viper_wnd->window_state & state_mask) break;
		node=node->next;
	}

	if(node==NULL) return NULL;
	return viper_wnd->user_window;
}

void viper_window_set_top(WINDOW *window)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd=NULL;

	if(viper->wnd_count==0) return;
	viper_wnd=viper_get_viper_wnd(window);
	if(viper_wnd==NULL) return;

	/*	only allow those windows which can catch focus to be placed on top	*/
	if(is_viper_window_allowed_focus(window)==FALSE) return;

	/*	is this window already on top?  if so, nothing to be done	*/
	if(viper_wnd==viper->wnd_list->data) return;
	/*	place eminent window on top... "no questions asked"	*/
	if(viper_wnd->window_state & STATE_EMINENT)
	{
		viper->wnd_list=g_slist_remove(viper->wnd_list,viper_wnd);
		viper->wnd_list=g_slist_prepend(viper->wnd_list,(gpointer)viper_wnd);
		viper_window_focus(window);
		return;
	}

	/* if there aren't any eminent windows in the deck, fulfill request	*/
	if(viper_window_get_top(STATE_EMINENT)==NULL)
	{
		viper->wnd_list=g_slist_remove(viper->wnd_list,viper_wnd);
		viper->wnd_list=g_slist_prepend(viper->wnd_list,(gpointer)viper_wnd);
		viper_window_focus(window);
	}
	return;
}

void viper_deck_cycle(gint vector)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	VIPER_WND	   *first_wnd=NULL;
	GSList		   *node=NULL;

	if(viper->wnd_count==0) return;

	/* honor window eminency flag */
	viper_wnd=viper->wnd_list->data;
	if(viper_wnd->window_state & STATE_EMINENT) return;

	/* check for a valid cycle vector */
	if(vector!=0 && viper->wnd_count>1)
	{
		do
		{
			if(vector>0) node=viper->wnd_list;
			if(vector<0) node=g_slist_last(viper->wnd_list);
			viper_wnd=(VIPER_WND*)node->data;
			if(viper_wnd==first_wnd) break;
			if(first_wnd==NULL) first_wnd=viper_wnd;

			viper->wnd_list=g_slist_delete_link(viper->wnd_list,node);
			if(vector>0) viper->wnd_list=g_slist_append(viper->wnd_list,
				(gpointer)viper_wnd);
			if(vector<0) viper->wnd_list=g_slist_prepend(viper->wnd_list,
				(gpointer)viper_wnd);
		}
		while(is_viper_window_allowed_focus(viper_wnd->window)==FALSE);
	}

	viper_window_focus(TOPMOST_WINDOW);
	return;
}


gchar** viper_deck_get_wndlist(void)
{
	extern VIPER   *viper;
	gchar				**titles;

	if(viper->wnd_count==0) return NULL;

	titles=(gchar**)g_malloc0(sizeof(gchar*)*256);

	viper_window_for_each(viper_enum_window_titles,(gpointer)titles,1);
	return titles;
}

inline gboolean viper_deck_check_occlusion(VIPER_WND *top_wnd,VIPER_WND *bottom_wnd)
{
	gint	bx,by,bw,bh;
	gint	tx,ty,tw,th;
	
	getbegyx(bottom_wnd->window,by,bx);
	getmaxyx(bottom_wnd->window,bh,bw);
	getbegyx(top_wnd->window,ty,tx);
	getmaxyx(top_wnd->window,th,tw);
	
	/* factor in shadow effects */
	if(top_wnd->window_state & STATE_SHADOWED)
	{
		th++;
		tw++;
	}
	
	if(by>ty+th+1) return FALSE;
	if(bx>tx+tw+1) return FALSE;
	if(bx+bw+1<tx) return FALSE;
	if(by+bh+1<ty) return FALSE;
	
	return TRUE;
}
