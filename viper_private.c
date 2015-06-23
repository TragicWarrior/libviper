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

#include <unistd.h>
#include <termios.h>

#include "viper.h"
#include "viper_color.h"
#include "viper_callbacks.h"
#include "viper_private.h"
#include "viper_kmio.h"

WINDOW   *SCREEN_WINDOW=NULL;
VIPER    *viper=NULL;
guint32  viper_global_flags;

VIPER* viper_init(guint32 init_flags)
{
	extern VIPER				*viper;
   extern WINDOW           *SCREEN_WINDOW;
	static GStaticRecMutex	lock=G_STATIC_REC_MUTEX_INIT;
	gint							width,height;
	gchar							*env;
   mmask_t                 mouse_mask=ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;
	extern guint32				viper_global_flags;
   struct termios          term_desc;

	if(viper == NULL)
	{
      SCREEN_WINDOW=initscr();

		viper_global_flags |= init_flags;
		viper=(VIPER*)g_malloc0(sizeof(VIPER));
		viper->lock=&lock;
		viper_color_init();
		env=getenv("TERM");
		if(g_strrstr(env,"xterm")!=NULL) viper->xterm=TRUE;
		viper->user=getuid();
		getmaxyx(SCREEN_WINDOW,height,width);
		viper->wallpaper=newwin(height,width,0,0);
		viper->wallpaper_agent=viper_default_wallpaper_agent;
		viper_default_wallpaper_agent(viper->wallpaper,SCREEN_WINDOW);
      viper->border_agent[0]=viper_default_border_agent_unfocus;
      viper->border_agent[1]=viper_default_border_agent_focus;
      mousemask(mouse_mask,NULL);

		/*	these are "normal" settings that would be commonly
			configured for use with the library.  the user can always
			change them back.	*/
		keypad(SCREEN_WINDOW,TRUE);
   	nodelay(SCREEN_WINDOW,TRUE);
   	scrollok(SCREEN_WINDOW,FALSE);
	   noecho();
   	raw();
   	intrflush(NULL,TRUE);
		curs_set(0);

      /* ncurses initscr() is supposed to do this but isn't (at least in
         some versions of ncurses.  */ 
      tcgetattr(STDIN_FILENO,&term_desc);
      term_desc.c_lflag &= ~(ECHO);
      tcsetattr(STDIN_FILENO,TCSADRAIN,&term_desc);
	}

	return viper;
}

void viper_end(void)
{
   extern VIPER            *viper;
   extern WINDOW           *SCREEN_WINDOW;
   struct termios          term_desc;

#if !defined(_NO_GPM) && defined(__linux)
   viper_kmio_gpm(NULL,CMD_GPM_CLOSE);
#endif

   if(viper != NULL) 
   {
      g_static_rec_mutex_free(viper->lock);
      g_free(viper);
      viper=NULL;
   }

   curs_set(1);
   endwin();
   SCREEN_WINDOW=NULL;

   /* clean up the echo suppression hack  */
   tcgetattr(STDIN_FILENO,&term_desc);
   term_desc.c_lflag |= ECHO;
   tcsetattr(STDIN_FILENO,TCSADRAIN,&term_desc);

   return;
}


void viper_set_border_agent(VIPER_FUNC agent,gint id)
{
   extern VIPER   *viper;

   if(id>1) return;

   viper->border_agent[id]=agent;

   return;
}

WINDOW*	viper_get_window_frame(WINDOW *window)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;

	if(viper->wnd_count==0) return NULL;
	viper_wnd=viper_get_viper_wnd(window);

	if(viper_wnd==NULL) window=NULL;
	else window=viper_wnd->window;

	return window;
}

inline VIPER_WND* viper_get_viper_wnd(WINDOW *window)
{
	extern VIPER	*viper;
	VIPER_WND	   *viper_wnd;
	GSList		   *node;

	if(window==NULL) return NULL;
	if(viper->wnd_count==0) return NULL;

	node=viper->wnd_list;
	while(node!=NULL)
	{
		viper_wnd=(VIPER_WND*)node->data;
		if(viper_wnd->window==(gpointer)window) break;
		if(viper_wnd->user_window==(gpointer)window) break;
		node=node->next;
	}

	if(node!=NULL) return viper_wnd;
	return (VIPER_WND*)NULL;
}


inline void viper_window_for_each(VIPER_FUNC func,gpointer arg,gint vector)
{
	extern VIPER   *viper;
	VIPER_WND	   *viper_wnd;
	GSList		   *copy=NULL;
	GSList		   *node;

	if(func==NULL) return;
	if(viper->wnd_list==0) return;

	if(vector==VECTOR_BOTTOM_TO_TOP)
	{
		copy=g_slist_copy(viper->wnd_list);
		copy=g_slist_reverse(copy);
		node=copy;
	}
	else node=viper->wnd_list;

	while(node!=NULL)
	{
		viper_wnd=(VIPER_WND*)node->data;
		func(viper_wnd->user_window,arg);
		node=node->next;
	}

	if(copy!=NULL) g_slist_free(copy);
	return;
}
