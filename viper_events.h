#ifndef _VIPER_EVENTS_H
#define _VIPER_EVENTS_H

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

/*
    these events get automatically added to certain windows during
	initialization
*/

int    viper_event_default_WINDOW_CLOSE(WINDOW *window, void *arg);
int    viper_event_default_TERM_RESIZE(WINDOW *window, void *arg);
int    viper_event_default_MSGBOX_CLOSE(WINDOW *window, void *arg);

#endif
