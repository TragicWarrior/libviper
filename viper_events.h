#ifndef _VIPER_EVENTS_H
#define _VIPER_EVENTS_H

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

/*
    these events get automatically added to certain windows during
	initialization
*/

gint    viper_event_default_WINDOW_CLOSE(WINDOW *window, gpointer arg);
gint    viper_event_default_TERM_RESIZE(WINDOW *window, gpointer arg);
gint    viper_event_default_MSGBOX_CLOSE(WINDOW *window, gpointer arg);

#endif
