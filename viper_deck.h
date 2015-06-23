#ifndef _VIPER_DECK_H
#define _VIPER_DECK_H

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

/* void			viper_deck_validate_top(void); */
gboolean	viper_deck_check_occlusion(VIPER_WND *bottom_wnd,VIPER_WND *top_wnd);

#endif
