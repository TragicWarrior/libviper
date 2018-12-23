#ifndef _VIPER_CALLBACKS_H_
#define	_VIPER_CALLBACKS_H_

#include <ncursesw/curses.h>

#include "viper.h"

/*	these callback perform various utilities which are used throughout
	the library.  EVENT callbacks which are define elsewhere.	*/

int     viper_callback_change_focus(vwnd_t *vwnd, void *arg);
int	    viper_callback_change_eminency(vwnd_t *vwnd, void *arg);

int	    viper_callback_touchwin(vwnd_t *vwnd, void *arg);
int	    viper_callback_blit_window(vwnd_t *vwnd, void *arg);

void    viper_default_wallpaper_agent(int screen_id);

int     viper_default_border_agent_focus(vwnd_t *vwnd, void *arg);
int     viper_default_border_agent_unfocus(vwnd_t *vwnd, void *arg);

void	viper_callback_del_event(void *data, void *anything);

#endif
