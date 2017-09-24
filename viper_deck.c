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

#include <string.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_screen.h"
#include "viper_states.h"

#include "list.h"

WINDOW*
viper_deck_hit_test(int x, int y)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd = NULL;
    struct list_head    *pos = NULL;

    if(list_empty(&viper->wnd_list)) return NULL;

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);
        if(wenclose(viper_wnd->window, y, x) == TRUE) break;

        viper_wnd = NULL;
    }

    if(viper_wnd == NULL) return NULL;

    return viper_wnd->user_window;
}

WINDOW*
viper_window_get_top(uint32_t state_mask)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    struct list_head    *pos;

    if(list_empty(&viper->wnd_list)) return NULL;

    if(state_mask == 0)
    {
        viper_wnd = list_first_entry(&viper->wnd_list, VIPER_WND, list);

        return viper_wnd->user_window;
    }

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);

        if(viper_wnd->window_state & state_mask) break;

        viper_wnd = NULL;
    }

    if(viper_wnd == NULL) return NULL;

    return viper_wnd->user_window;
}


void
viper_window_set_top(WINDOW *window)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd = NULL;
    VIPER_WND       *top_wnd = NULL;

    if(list_empty(&viper->wnd_list)) return;

    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return;

    /*	only allow those windows which can catch focus to be placed on top	*/
    if(is_viper_window_allowed_focus(window) == FALSE) return;

    /*	is this window already on top?  if so, nothing to be done	*/
    top_wnd = list_entry(&viper->wnd_list, VIPER_WND, list);
    if(viper_wnd == top_wnd) return;

    /*	place eminent window on top... "no questions asked"	*/
    if(viper_wnd->window_state & STATE_EMINENT)
    {
        list_del(&viper_wnd->list);
        list_add(&viper_wnd->list, &viper->wnd_list);

        viper_window_focus(window);

        return;
    }

    /* if there aren't any eminent windows in the deck, fulfill request	*/
    if(viper_window_get_top(STATE_EMINENT) == NULL)
    {
        list_del(&viper_wnd->list);
        list_add(&viper_wnd->list, &viper->wnd_list);

        viper_window_focus(window);
    }

    return;
}

void
viper_deck_cycle(int vector)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;
    VIPER_WND       *first_wnd = NULL;

    if(list_empty(&viper->wnd_list)) return;

    /* honor window eminency flag */
    viper_wnd = list_first_entry(&viper->wnd_list, VIPER_WND, list);
    if(viper_wnd->window_state & STATE_EMINENT) return;

    do
    {
        if(vector > 0)
        {
            list_rotate_left(&viper->wnd_list);
        }

        if(vector < 0)
        {
            list_rotate_right(&viper->wnd_list);
        }

        // get what's on top now
        viper_wnd = list_first_entry(&viper->wnd_list, VIPER_WND, list);
    }
    while(is_viper_window_allowed_focus(viper_wnd->window) == FALSE);

    viper_window_focus(TOPMOST_WINDOW);
    return;
}

char**
viper_deck_get_wndlist(void)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    char                **titles;
    struct list_head    *pos;
    int                 i = 0;

    if(list_empty(&viper->wnd_list)) return NULL;

    titles = (char**)calloc(1, sizeof(char*) * 256);

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);

        if(viper_wnd->title != NULL)
        {
            titles[i] = strdup(viper_wnd->title);
            i++;
        }
    }

    return titles;
}

inline gboolean
viper_deck_check_occlusion(VIPER_WND *top_wnd, VIPER_WND *bottom_wnd)
{
    gint    bx, by, bw, bh;
    gint    tx, ty, tw, th;

    getbegyx(bottom_wnd->window, by, bx);
    getmaxyx(bottom_wnd->window, bh, bw);
    getbegyx(top_wnd->window, ty, tx);
    getmaxyx(top_wnd->window, th, tw);

    /* factor in shadow effects */
    if(top_wnd->window_state & STATE_SHADOWED)
    {
        th++;
        tw++;
    }

    if(by > ty + th + 1) return FALSE;
    if(bx > tx + tw + 1) return FALSE;
    if(bx + bw + 1 < tx) return FALSE;
    if(by + bh + 1 < ty) return FALSE;

    return TRUE;
}
