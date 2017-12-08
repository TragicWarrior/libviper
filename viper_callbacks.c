/*************************************************************************
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 2007      Bryan Christ <bryan.christ@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

#include <curses.h>

#include "viper.h"
#include "private.h"
#include "viper_callbacks.h"
#include "viper_states.h"

int
viper_callback_touchwin(vwnd_t *vwnd, void *arg)
{
	if(vwnd == NULL) return ERR;

	if(vwnd->window_state & STATE_VISIBLE)
	{
		touchwin(vwnd->user_window);
		touchwin(vwnd->window_frame);
	}

    (void)arg;

	return 0;
}

inline int
viper_callback_blit_window(vwnd_t *vwnd, void *arg)
{
	WINDOW		    *shadow_window;
    int             idx = 0;
    int             retval = 0;

	if(vwnd == NULL) return ERR;

	if(!(vwnd->window_state & STATE_VISIBLE)) return 0;

	if(vwnd->window_state & STATE_VISIBLE)
	{
        /* run the border agent to decorate the window. */
        if(vwnd->ctx->managed == TRUE)
        {
            if(vwnd->window_state & STATE_FOCUS) idx = 1;
            if(vwnd->border_agent[idx] != NULL)
            {
                vwnd->border_agent[idx](vwnd, (void*)vwnd);
            }
        }

		if(vwnd->window_state & STATE_SHADOWED)
		{
			shadow_window = window_create_shadow(WINDOW_FRAME(vwnd),
                CURRENT_SCREEN);
			overwrite(shadow_window, CURRENT_SCREEN);
			delwin(shadow_window);
		}

		retval = overwrite(WINDOW_FRAME(vwnd), CURRENT_SCREEN);
	}

    if(retval == ERR) return -1;

    (void)arg;

	return 0;
}

void
viper_callback_del_event(void *data, void *anything)
{
	free(data);

    // suppress compiler warnings
    (void)anything;

	return;
}

void
viper_default_wallpaper_agent(int screen_id)
{
    extern VIPER    *viper;
    int             width, height;

    if(screen_id > MAX_SCREENS) return;

    // make sure the wallpaper window is the same size
    // as the target screen
    getmaxyx(viper->screen[screen_id], height, width);
    wresize(viper->wallpaper[screen_id], height, width);

#ifdef _VIPER_WIDE

	static cchar_t	bg_char;
	wchar_t			wch[] = {0x0020, 0x0000};

	setcchar(&bg_char, wch, 0, 0, NULL);

	window_fill(viper->wallpaper[screen_id], &bg_char,
        viper_color_pair(COLOR_WHITE, COLOR_BLUE), A_NORMAL);
#else
	window_fill(viper->wallpaper[screen_id], ' ',
        viper_color_pair(COLOR_WHITE, COLOR_BLUE), A_NORMAL);
#endif

	return;
}

int
viper_default_border_agent_focus(vwnd_t *vwnd, void *arg)
{
    if(vwnd == NULL) return -1;
    if(vwnd->ctx->managed == FALSE) return -1;

    window_decorate(WINDOW_FRAME(vwnd), (char*)vwnd->title, TRUE);

    window_modify_border(WINDOW_FRAME(vwnd), A_NORMAL,
      viper_color_pair(COLOR_MAGENTA, COLOR_WHITE));

    (void)arg;

    return 0;
}

int
viper_default_border_agent_unfocus(vwnd_t *vwnd, void *arg)
{
    if(vwnd == NULL) return -1;
    if(vwnd->ctx->managed == FALSE) return -1;

    window_decorate(WINDOW_FRAME(vwnd), (char*)vwnd->title, TRUE);

    window_modify_border(WINDOW_FRAME(vwnd), A_BOLD,
        viper_color_pair(COLOR_BLACK,COLOR_WHITE));

    (void)arg;

    return 0;
}

