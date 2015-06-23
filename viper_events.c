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

VIPER_EVENT* viper_get_viper_event(WINDOW *window,gchar *event)
{
   extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	VIPER_EVENT	   *viper_event=NULL;
	GSList		   *node=NULL;
	
	if(window==NULL || event==NULL) return NULL;
/*	viper=viper_get_instance(); */
	if(viper->wnd_count==0) return NULL;
	
	viper_wnd=viper_get_viper_wnd(window);
	if(viper_wnd!=NULL)
	{
		node=viper_wnd->event_list;
		while(node!=NULL)
		{
			viper_event=(VIPER_EVENT*)node->data;
			if(memcmp(viper_event->event,event,strlen(event))==0) break;
			node=node->next;
		}
	}
	
	if(node!=NULL) return viper_event;
	return (VIPER_EVENT*)NULL;		
}

gint viper_event_set(WINDOW *window,gchar *event,VIPER_FUNC func,gpointer arg)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd=NULL;
	VIPER_EVENT	   *viper_event=NULL;
		
	if(window==NULL || event==NULL || func==NULL) return -1;
	
/*	viper=viper_get_instance(); */
	if(viper->wnd_count==0) return ERR;
	
	viper_event=viper_get_viper_event(window,event);
	if(viper_event==NULL)
	{
		viper_wnd=viper_get_viper_wnd(window);
		viper_event=(VIPER_EVENT*)g_malloc0(sizeof(VIPER_EVENT));
		viper_wnd->event_list=g_slist_prepend(viper_wnd->event_list,
			(gpointer)viper_event);
	}
	
	viper_event->event=event;
	viper_event->func=func;
	viper_event->arg=arg;
		
	if(viper_wnd==NULL) return ERR;
	return 1;
}

gint viper_event_del(WINDOW *window,gchar *event)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	VIPER_EVENT	   *viper_event=NULL;
		
	if(window==NULL || event==NULL) return -1;
	
/*	viper=viper_get_instance(); */
	if(viper->wnd_count==0) return ERR;
	
	viper_wnd=viper_get_viper_wnd(window);
	if(event[0]=='*')
	{
		g_slist_foreach(viper_wnd->event_list,viper_callback_del_event,NULL);
		g_slist_free(viper_wnd->event_list);
		viper_wnd->event_list=NULL;
		viper_event=(gpointer)1;   /* so that we don't return ERR.   */
	}
	else
	{
		viper_event=viper_get_viper_event(window,event);
		if(viper_event!=NULL)
      {
         viper_wnd->event_list=g_slist_remove(viper_wnd->event_list,
            (gconstpointer)viper_event);
         g_free(viper_event);
         if(g_slist_length(viper_wnd->event_list)==0)
         {
            g_slist_free(viper_wnd->event_list);
		      viper_wnd->event_list=NULL;
         }
      }
	}
		
	if(viper_event==NULL) return ERR;
	return 1;
}

gint viper_event_exec(WINDOW *window,gchar *event,gpointer anything)
{
	extern VIPER   *viper;
   VIPER_WND      *viper_wnd;
   VIPER_EVENT	   *viper_event;
   GSList         *copy;
   GSList         *node;
   gboolean	      broadcast=FALSE;
	
	if(window==NULL || event==NULL) return ERR;

   /* if the user wants to send a broadcast event, the value of *window will
      be VIPER_EVENT_BROADCAST.   */
	if(memcmp(window,VIPER_EVENT_BROADCAST,sizeof(VIPER_EVENT_BROADCAST))==0)
		broadcast=TRUE;
		
	/* viper=viper_get_instance();*/ 
	if(viper->wnd_count==0) return ERR;

   if(broadcast==TRUE)
   {
      copy=g_slist_copy(viper->wnd_list);
      
      /* when doing a broadcast, the bottom most window in the stack Z-order
         will be signaled first.  the last window will be the top window.  */
      copy=g_slist_reverse(copy);

      node=copy;
      while(node!=NULL)
	   {
		   viper_wnd=(VIPER_WND*)node->data;
         viper_event=viper_get_viper_event(viper_wnd->user_window,event);
         if(viper_event!=NULL)
         {         
            /* if anything is NULL, pass the default data with the event.  */
		      if(anything==NULL)
               viper_event->func(viper_wnd->user_window,viper_event->arg);
            /* otherwise, pass the override data.  */
            else
               viper_event->func(viper_wnd->user_window,anything);
         }
		   node=node->next;
      }

      if(copy!=NULL) g_slist_free(copy);
	}
   else
   {
      viper_wnd=viper_get_viper_wnd(window);
      viper_event=viper_get_viper_event(viper_wnd->user_window,event);
      if(viper_event!=NULL)
      {      
         /* if anything is NULL, pass the default data with the event.  */
         if(anything==NULL)
            viper_event->func(viper_wnd->user_window,viper_event->arg);
         /* otherwise, pass the override data.  */
         else
            viper_event->func(viper_wnd->user_window,anything);
      }
	}
					
	return 1;
}

/*	these events defined below get automatically added to certain windows
	during initialization	*/
gint viper_event_default_TERM_RESIZE(WINDOW *window,gpointer arg)
{
/*	extern VIPER   *viper; */
	VIPER_WND	   *viper_wnd;
	gint		      x,y;
	
/*	viper=viper_get_instance(); */
	viper_wnd=viper_get_viper_wnd(window);
	
	x=window_check_height(viper_wnd->window);
	y=window_check_width(viper_wnd->window);
	
	if(x>0 || y>0)
		viper_wresize_abs(window,viper_wnd->min_width,viper_wnd->min_height);
	
	viper_screen_redraw(REDRAW_ALL);
	
	return 0;
}	

/* gint viper_event_default_FOCUS(WINDOW *window,gpointer arg)
{
	VIPER		*viper;
	
	viper=viper_get_instance();

	if(viper->xterm==TRUE) viper_window_modify_border(window,A_BOLD,
		viper_color_pair(COLOR_WHITE,COLOR_MAGENTA));
	else viper_window_modify_border(window,A_NORMAL,
		viper_color_pair(COLOR_BLACK,COLOR_MAGENTA));
	
	return 0;
} */

/* gint viper_event_default_UNFOCUS(WINDOW *window,gpointer arg)
{
	viper_window_modify_border(window,A_NORMAL,
		viper_color_pair(COLOR_BLACK,COLOR_CYAN));
	
	return 0;
} */

gint viper_event_default_WINDOW_CLOSE(WINDOW *window,gpointer arg)
{
	return VIPER_EVENT_WINDOW_DESIST;
}

/*	by returning VIPER_EVENT_WINDOW_PERSIST we prevent the user from closing
	the msgbox using the mouse, hotkey, or viper_window_close() function.	*/	
gint viper_event_default_MSGBOX_CLOSE(WINDOW *window,gpointer arg)
{
	beep();
	return VIPER_EVENT_WINDOW_PERSIST;
}
