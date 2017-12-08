#ifndef _VIPER_STATES_H
#define _VIPER_STATES_H

#include <stdbool.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#include "viper.h"

bool    is_viper_window_visible(vwnd_t *vwnd);

#endif
