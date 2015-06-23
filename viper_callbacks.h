#ifndef _VIPER_CALLBACKS_H_
#define	_VIPER_CALLBACKS_H_

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

/*	these callback perform various utilities which are used throughout
	the library.  EVENT callbacks which are define elsewhere.	*/

gint	viper_callback_change_focus(WINDOW *window,gpointer arg);
gint	viper_callback_change_eminency(WINDOW *window,gpointer arg);
gint	viper_callback_touchwin(WINDOW *window,gpointer arg);
gint	viper_callback_blit_window(WINDOW *window,gpointer arg);

gint	viper_default_wallpaper_agent(WINDOW *window,gpointer arg);
gint  viper_default_border_agent_focus(WINDOW *window,gpointer arg);
gint  viper_default_border_agent_unfocus(WINDOW *window,gpointer arg);

gint	viper_enum_window_titles(WINDOW *window,gpointer anything);

void	viper_callback_del_event(gpointer data,gpointer anything);

#endif
