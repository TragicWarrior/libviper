#ifndef _VIPER_DECK_H
#define _VIPER_DECK_H

#include <stdbool.h>

#include <ncursesw/curses.h>

bool    viper_deck_check_occlusion(VIPER_WND *bottom_wnd, VIPER_WND *top_wnd);

#endif
