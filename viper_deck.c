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
#include "private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_screen.h"
#include "viper_states.h"

#include "list.h"

vwnd_t*
viper_deck_hit_test(int screen_id, bool managed, int x, int y)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd = NULL;
    struct list_head    *pos = NULL;
    struct list_head    *wnd_list;

    if(screen_id == -1)
        screen_id = CURRENT_SCREEN_ID;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return NULL;

    list_for_each(pos, wnd_list)
    {
        vwnd = list_entry(pos, VIPER_WND, list);

        // window frame and user window will be the same
        // for unmanaged windows so this always works
        if(wenclose(WINDOW_FRAME(vwnd), y, x) == TRUE) break;

        vwnd = NULL;
    }

    if(vwnd == NULL) return NULL;

    return vwnd;
}

vwnd_t*
viper_window_get_top(int screen_id, bool managed)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd;
    struct list_head    *wnd_list;
    struct list_head    *pos = NULL;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return NULL;

    list_for_each(pos, wnd_list)
    {
        vwnd = list_entry(pos, vwnd_t, list);

        if(vwnd->window_state & STATE_VISIBLE) break;

        vwnd = NULL;
    }

    if(vwnd == NULL) return NULL;

    return vwnd;
}


bool
viper_window_set_top(vwnd_t *vwnd)
{
    extern VIPER        *viper;
    struct list_head    *wnd_list;
    int                 screen_id;

    if(vwnd == NULL) return FALSE;

    // make sure we're on the active screen
    // possibly change behavior later to allow cycling on
    // inactive screen
    screen_id = CURRENT_SCREEN_ID;
    if(vwnd->ctx->screen_id != screen_id) return FALSE;

    if(vwnd->ctx->managed == TRUE)
    {
        wnd_list = &viper->managed_list[screen_id];
    }
    else
    {
        wnd_list = &viper->unmanaged_list[screen_id];
    }

    if(list_empty(wnd_list)) return FALSE;

    // only allow those windows which can catch focus to be placed on top
    if(is_viper_window_visible(vwnd) == FALSE) return FALSE;

    if(viper_window_set_focus(vwnd) == FALSE) return FALSE;

    // move window to the front of the deck
    list_move(&vwnd->list, wnd_list);

    return TRUE;
}

void
viper_deck_cycle(int screen_id, bool managed, int vector)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd;
    vwnd_t              *first_wnd = NULL;
    struct list_head    *wnd_list;

    // don't allow cycling of offscreen decks
    if(screen_id != CURRENT_SCREEN_ID) return;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return;
    if(list_is_singular(wnd_list)) return;

    do
    {
        if(vector >= VECTOR_TOP_TO_BOTTOM)
        {
            list_rotate_left(wnd_list);
        }

        if(vector <= VECTOR_BOTTOM_TO_TOP)
        {
            list_rotate_right(wnd_list);
        }

        // get what's on top now
        vwnd = list_first_entry(wnd_list, vwnd_t, list);

        if(first_wnd == NULL)
        {
            first_wnd = vwnd;
            continue;
        }

        if(vwnd != NULL)
        {
            if(is_viper_window_visible(vwnd) == TRUE) break;
        }
    }
    while(vwnd != first_wnd);

    if(viper_window_set_top(vwnd) == TRUE)
    {
        viper_window_redraw(vwnd);
        viper_screen_redraw(screen_id, REDRAW_ALL);
    }

    return;
}

char**
viper_deck_get_wndlist(int screen_id, bool managed)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd;
    char                **titles;
    struct list_head    *pos;
    struct list_head    *wnd_list;
    int                 i = 0;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return NULL;

    titles = (char**)calloc(1, sizeof(char*) * 256);

    list_for_each(pos, wnd_list)
    {
        vwnd = list_entry(pos, vwnd_t, list);

        if(vwnd->title != NULL)
        {
            titles[i] = strdup(vwnd->title);
            i++;
        }
    }

    return titles;
}

inline bool
viper_deck_check_occlusion(vwnd_t *top_wnd, vwnd_t *bottom_wnd)
{
    int     bx, by, bw, bh;
    int     tx, ty, tw, th;

    getbegyx(bottom_wnd->window_frame, by, bx);
    getmaxyx(bottom_wnd->window_frame, bh, bw);
    getbegyx(top_wnd->window_frame, ty, tx);
    getmaxyx(top_wnd->window_frame, th, tw);

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
