#ifndef _VIPER_EVENTS_H
#define _VIPER_EVENTS_H

#include <stdbool.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#include "viper.h"

/*
    these events get automatically added to certain windows during
	initialization
*/

VIPER_EVENT*    viper_get_viper_event(vwnd_t *vwnd, char *event);

int     viper_event_default_WINDOW_CLOSE(vwnd_t *vwnd, void *arg);
int     viper_event_default_TERM_RESIZE(vwnd_t *vwnd, void *arg);
int     viper_event_default_MSGBOX_CLOSE(vwnd_t *vwnd, void *arg);

#endif
