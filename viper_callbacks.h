#ifndef _VIPER_CALLBACKS_H_
#define	_VIPER_CALLBACKS_H_

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

/*	these callback perform various utilities which are used throughout
	the library.  EVENT callbacks which are define elsewhere.	*/

int     viper_callback_change_focus(WINDOW *window, void *arg);
int	    viper_callback_change_eminency(WINDOW *window, void *arg);
int	    viper_callback_touchwin(WINDOW *window, void *arg);
int	    viper_callback_blit_window(WINDOW *window, void *arg);
int     viper_default_wallpaper_agent(WINDOW *window, void *arg);
int     viper_default_border_agent_focus(WINDOW *window, void *arg);
int     viper_default_border_agent_unfocus(WINDOW *window, void *arg);

int	    viper_enum_window_titles(WINDOW *window, void *anything);

void	viper_callback_del_event(void *data, void *anything);

#endif
