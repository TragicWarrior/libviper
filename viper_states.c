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

#include <stdbool.h>
#include <inttypes.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_states.h"
#include "list.h"

uint32_t
viper_window_get_state(WINDOW *window)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;

    if(window == NULL) return 0;

    if(list_empty(&viper->wnd_list)) return 0;
    viper_wnd = viper_get_viper_wnd(window);

    return viper_wnd->window_state;
}

void
viper_window_set_eminency(WINDOW *window, bool value)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    struct list_head    *pos;

    if(window == NULL) return;
    if(list_empty(&viper->wnd_list)) return;

    if(value == FALSE)
    {
        viper_wnd = viper_get_viper_wnd(window);
        if(viper_wnd == NULL) return;

        viper_wnd->window_state &= ~STATE_EMINENT;
        return;
    }

    // never allow a hidden window to be set eminent
    if(is_viper_window_visible(window) == FALSE) return;

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);

        // clear all eminency flags
        viper_wnd->window_state &= ~STATE_EMINENT;
    }

    viper_wnd = viper_get_viper_wnd(window);
    viper_wnd->window_state |= STATE_EMINENT;

    list_move(&viper->wnd_list, &viper_wnd->list);

    return;
}


void
viper_window_set_shadow(WINDOW *window, bool value)
{
    VIPER_WND       *viper_wnd;

    if(window == NULL) return;
    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return;

    if(value == TRUE)
        viper_wnd->window_state |= STATE_SHADOWED;
    else
        viper_wnd->window_state &= ~STATE_SHADOWED;

    return;
}


void
viper_window_set_visible(WINDOW *window, bool value)
{
    VIPER_WND   *viper_wnd;

    if(window == NULL) return;
    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return;

    if(value == TRUE)
        viper_wnd->window_state |= STATE_VISIBLE;
    else
        viper_wnd->window_state &= ~STATE_VISIBLE;

    return;
}

void
viper_window_set_resizable(WINDOW *window, bool value)
{
    VIPER_WND       *viper_wnd;

    if(window == NULL) return;
    viper_wnd = viper_get_viper_wnd(window);
    if(viper_wnd == NULL) return;

    if(value == TRUE)
        viper_wnd->window_state &= ~STATE_NORESIZE;
    else
        viper_wnd->window_state |= ~STATE_NORESIZE;

    return;
}


bool
viper_window_set_focus(WINDOW *window)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    struct list_head    *pos;

    if(window == NULL) return FALSE;

    if(is_viper_window_allowed_focus(window) == FALSE) return FALSE;

    // remove focus from all other windows
    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);

        if(viper_wnd->window_state & STATE_FOCUS)
            viper_event_run(window, "window-deactivate");
        viper_wnd->window_state &= ~STATE_FOCUS;
        viper_event_run(window, "window-unfocus");
    }

    viper_wnd = viper_get_viper_wnd(window);

    viper_wnd->window_state |= STATE_FOCUS;
    viper_event_run(window, "window-focus");
    viper_event_run(window, "window-activate");

    return TRUE;
}

void
viper_window_redraw(WINDOW *window)
{
    extern WINDOW   *SCREEN_WINDOW;
    extern VIPER    *viper;
    VIPER_WND       *redraw_wnd;
    VIPER_WND       *next_wnd;

    if(window == NULL) return;

    /*
        make sure the window is not a subwin, if so, traverse to top and
        change reference.  not sure about the portablility.  relies on a
        consistent implementation of struct win_st
    */
    while(window->_parent != NULL) window = window->_parent;
    redraw_wnd = viper_get_viper_wnd(window);

    /*
        redrawing a window is somewhat complex.  if you just redraw the
        window, and the window is not on the top of the deck, the operation
        will occlude other windows and the rendered z-order will be wrong.
        all windows which are on top and overlap, need to be redrawn.  of
        course redrawing those windows could occlude other windows.  in order
        to deal with this problem, this function is recursive.  by setting
        the viper->redraw_catalyst, we tag the original window so that only after
        the parent instance of this function completes do we copy the virtual
        screen out to the terminal.
    */
    if(viper->redraw_catalyst == NULL) viper->redraw_catalyst = window;
    viper_callback_blit_window(redraw_wnd->window ,NULL);

    /*
        start traversing with the window to be redraw.  this will change as
        the function recurses.
    */
    if(!(list_is_first(&redraw_wnd->list, &viper->wnd_list)))
    {
        next_wnd = list_prev_entry(redraw_wnd, list);

        list_for_each_entry_from_reverse(next_wnd, &viper->wnd_list, list)
        {
            // check to see if we're at the top
            if(viper_deck_check_occlusion(next_wnd, redraw_wnd) == TRUE)
                viper_window_redraw(next_wnd->window);
        }
    }

    /*
        only copy the virtual screen to the terminal *IF* this is the orignal
        instance of this recursive function.
    */
    if(window == viper->redraw_catalyst)
    {
        if(viper->console_mouse != NULL)
        overwrite(viper->console_mouse, SCREEN_WINDOW);

        wnoutrefresh(SCREEN_WINDOW);
        doupdate();
        viper->redraw_catalyst = NULL;
    }

    return;
}

bool
is_viper_window_visible(WINDOW *window)
{
    VIPER_WND           *viper_wnd;

    if(window == NULL) return FALSE;

    viper_wnd = viper_get_viper_wnd(window);

    if(viper_wnd->window_state & STATE_VISIBLE) return TRUE;

    return FALSE;
}

bool
is_viper_window_allowed_focus(WINDOW *window)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    VIPER_WND           *other_wnd;
    struct list_head    *pos;

    if(window == NULL) return FALSE;
    if(list_empty(&viper->wnd_list)) return FALSE;

    viper_wnd = viper_get_viper_wnd(window);

    /*
        notice that an unmanaged window can be moved to the top of the deck
        if it has the emmient bit set.  this trump case allows for special
        cases like a screensaver
    */
    if(viper_wnd->window_state & STATE_EMINENT) return TRUE;

    if((viper_wnd->window_state & STATE_MANAGED) &&
        (viper_wnd->window_state & STATE_VISIBLE)) return TRUE;

    // make sure there aren't any eminent windows in the deck
    list_for_each(pos, &viper->wnd_list)
    {
        other_wnd = list_entry(pos, VIPER_WND, list);

        if(other_wnd->window_state & STATE_EMINENT)
        {
            if(other_wnd != viper_wnd) return FALSE;
        }

        other_wnd = NULL;
    }

    return FALSE;
}
