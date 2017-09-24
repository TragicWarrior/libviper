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

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "viper_states.h"

gint
viper_callback_change_focus(WINDOW *window, gpointer arg)
{
	VIPER_WND	   *viper_wnd;

	viper_wnd=viper_get_viper_wnd(window);

	if(is_viper_window_allowed_focus(window) == FALSE) return 0;

	/* set focus for only the designated window */
	if(window == (WINDOW*)arg)
	{
		viper_wnd->window_state |= STATE_FOCUS;
		viper_event_run(window, "window-focus");
		viper_event_run(window, "window-activate");
	}
	/* remove focus from all other windows */
	else
	{
		if(viper_wnd->window_state & STATE_FOCUS)
			viper_event_run(window, "window-deactivate");
		viper_wnd->window_state &= ~STATE_FOCUS;
		viper_event_run(window, "window-unfocus");
	}

	return 0;
}

gint
viper_callback_change_eminency(WINDOW *window, gpointer arg)
{
	VIPER_WND   	*viper_wnd;

	viper_wnd = viper_get_viper_wnd(window);

	if(window == (WINDOW*)arg) viper_wnd->window_state |= STATE_EMINENT;
	else viper_wnd->window_state &= ~STATE_EMINENT;

	return 0;
}

gint
viper_callback_touchwin(WINDOW *window, gpointer arg)
{
	VIPER_WND	*viper_wnd;

	if(window == NULL) return ERR;
	viper_wnd = viper_get_viper_wnd(window);
	if(viper_wnd == NULL) return ERR;

	if(viper_wnd->window_state & STATE_VISIBLE)
	{
		touchwin(viper_wnd->user_window);
		touchwin(viper_wnd->window);
	}

	return 0;
}

inline gint
viper_callback_blit_window(WINDOW *window,gpointer arg)
{
    extern WINDOW   *SCREEN_WINDOW;
	VIPER_WND	    *viper_wnd;
	WINDOW		    *shadow_window;
    guint32         state_mask = 0;
    gint            idx = 0;

	if(window == NULL) return ERR;
	viper_wnd = viper_get_viper_wnd(window);
	if(viper_wnd == NULL) return ERR;

	if(!(viper_wnd->window_state & STATE_VISIBLE)) return 0;
	if(arg != NULL) state_mask = *(guint32*)arg;
	else state_mask = ~0;

	if(viper_wnd->window_state & state_mask)
	{
        /* run the border agent to decorate the window. */
        if(viper_wnd->window_state & STATE_MANAGED)
        {
            if(viper_wnd->window_state & STATE_FOCUS) idx=1;
            if(viper_wnd->border_agent[idx] != NULL)
            {
                viper_wnd->border_agent[idx](viper_wnd->window,
                    (gpointer)viper_wnd);
            }
        }

		if(viper_wnd->window_state & STATE_SHADOWED)
		{
			shadow_window = window_create_shadow(viper_wnd->window,
                SCREEN_WINDOW);
			overwrite(shadow_window, SCREEN_WINDOW);
			delwin(shadow_window);
		}

		overwrite(viper_wnd->window, SCREEN_WINDOW);
	}

	return 0;
}

void
viper_callback_del_event(gpointer data, gpointer anything)
{
	g_free(data);
	return;
}

gint
viper_default_wallpaper_agent(WINDOW *window, gpointer arg)
{
    extern WINDOW   *SCREEN_WINDOW;
    WINDOW          *screen_window;
	gint			width,height;
#ifdef _VIPER_WIDE
	static cchar_t	bg_char;
	wchar_t			wch[] = {0x0020, 0x0000};
#endif

	if(arg != NULL) screen_window = (WINDOW*)arg;
	else screen_window = SCREEN_WINDOW;

    getmaxyx(screen_window, height, width);
	wresize(screen_window, height, width);

#ifdef _VIPER_WIDE
	setcchar(&bg_char,wch, 0, 0, NULL);
	window_fill(window, &bg_char, viper_color_pair(COLOR_WHITE,COLOR_BLUE),
      A_NORMAL);
#else
	window_fill(window,' ', viper_color_pair(COLOR_WHITE,COLOR_BLUE), A_NORMAL);
#endif

	return 0;
}

gint
viper_default_border_agent_focus(WINDOW *window, gpointer anything)
{
    VIPER_WND   *viper_wnd;

    viper_wnd = (VIPER_WND*)anything;

    window_decorate(window, (gchar*)viper_wnd->title, TRUE);
    window_modify_border(viper_wnd->window, A_NORMAL,
      viper_color_pair(COLOR_MAGENTA, COLOR_WHITE));

    return 0;
}

gint
viper_default_border_agent_unfocus(WINDOW *window, gpointer anything)
{
    VIPER_WND   *viper_wnd;

    viper_wnd = (VIPER_WND*)anything;

    window_decorate(window,(gchar*)viper_wnd->title,TRUE);
    window_modify_border(viper_wnd->window,A_BOLD,
        viper_color_pair(COLOR_BLACK,COLOR_WHITE));

    return 0;
}

/*
gint
viper_enum_window_titles(WINDOW *window, gpointer anything)
{
	VIPER_WND  *viper_wnd;
	gchar      **titles;

	titles = (gchar**)anything;
	viper_wnd = viper_get_viper_wnd(window);

	while(titles[0] != NULL) titles++;

	*titles = g_strdup(viper_wnd->title);

	return 0;
}
*/
