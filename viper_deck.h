#ifndef _VIPER_DECK_H
#define _VIPER_DECK_H

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

gboolean    viper_deck_check_occlusion(VIPER_WND *bottom_wnd, VIPER_WND *top_wnd);

#endif
