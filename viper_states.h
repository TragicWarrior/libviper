#ifndef _VIPER_STATES_H
#define _VIPER_STATES_H

#include <stdbool.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

bool    is_viper_window_allowed_focus(WINDOW *window);

#endif
