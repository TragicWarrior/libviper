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
#include "private.h"
#include "viper_callbacks.h"
#include "viper_deck.h"
#include "viper_states.h"
#include "list.h"

struct _redraw_marker_s
{
    vwnd_t  *catalyst;
    bool    managed;
};

typedef struct _redraw_marker_s     redraw_marker_t;

uint32_t
viper_window_get_state(vwnd_t *vwnd)
{
    if(vwnd == NULL) return 0;

    return vwnd->window_state;
}

void
viper_window_set_shadow(vwnd_t *vwnd, bool value)
{
    if(vwnd == NULL) return;

    if(value == TRUE)
        vwnd->window_state |= STATE_SHADOWED;
    else
        vwnd->window_state &= ~STATE_SHADOWED;

    return;
}


void
viper_window_set_visible(vwnd_t *vwnd, bool value)
{
    if(vwnd == NULL) return;

    if(value == TRUE)
        vwnd->window_state |= STATE_VISIBLE;
    else
        vwnd->window_state &= ~STATE_VISIBLE;

    return;
}

void
viper_window_set_resizable(vwnd_t *vwnd, bool value)
{
    if(vwnd == NULL) return;

    if(value == TRUE)
        vwnd->window_state &= ~STATE_NORESIZE;
    else
        vwnd->window_state |= ~STATE_NORESIZE;

    return;
}

int
viper_window_get_screen_id(vwnd_t *vwnd)
{
    if(vwnd == NULL) return -1;

    return vwnd->ctx->screen_id;
}

bool
viper_window_set_focus(vwnd_t *vwnd)
{
    extern VIPER        *viper;
    viper_screen_t      *viper_screen;
    vwnd_t              *sibling_wnd;
    struct list_head    *wnd_list;
    struct list_head    *pos = NULL;
    int                 screen_id;

    if(vwnd == NULL) return FALSE;

    screen_id = vwnd->ctx->screen_id;

    viper_screen = &viper->viper_screen[screen_id];

    if(vwnd->ctx->managed == TRUE)
        wnd_list = &viper_screen->managed_list;
    else
        wnd_list = &viper_screen->unmanaged_list;

    // remove focus from all other windows
    list_for_each(pos, wnd_list)
    {
        sibling_wnd = list_entry(pos, vwnd_t, list);

        if(sibling_wnd->window_state & STATE_FOCUS)
        {
            sibling_wnd->window_state &= ~STATE_FOCUS;
            viper_event_run(sibling_wnd, "window-unfocus");
        }
    }

    vwnd->window_state |= STATE_FOCUS;
    viper_event_run(vwnd, "window-focus");

    return TRUE;
}

void
viper_window_redraw(vwnd_t *vwnd)
{
    static redraw_marker_t  *marker = NULL;
    extern VIPER            *viper;
    viper_screen_t          *viper_screen;
    vwnd_t                  *next_wnd;
    struct list_head        *wnd_list;
    int                     screen_id;

    if(vwnd == NULL) return;

    screen_id = vwnd->ctx->screen_id;
    if(screen_id != CURRENT_SCREEN_ID) return;

    viper_screen = &viper->viper_screen[screen_id];

    // init the recursive tracking marker
    if(marker == NULL)
    {
        marker = (redraw_marker_t *)calloc(1, sizeof(redraw_marker_t));
        marker->catalyst = vwnd;
        marker->managed = vwnd->ctx->managed;
    }

    if(marker->managed == TRUE)
        wnd_list = &viper_screen->managed_list;
    else
        wnd_list = &viper_screen->unmanaged_list;

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
    // if(viper->redraw_catalyst == NULL) viper->redraw_catalyst = vwnd;
    viper_callback_blit_window(vwnd, NULL);

    /*
        start traversing with the window to be redraw.  this will change as
        the function recurses.
    */
    if(!(list_is_first(&vwnd->list, wnd_list)))
    {
        next_wnd = list_prev_entry(vwnd, list);

        list_for_each_entry_from_reverse(next_wnd, wnd_list, list)
        {
            // check to see if we're at the top
            if(viper_deck_check_occlusion(next_wnd, vwnd) == TRUE)
                viper_window_redraw(next_wnd);
        }
    }

    /*
        only copy the virtual screen to the terminal *IF* this is the orignal
        instance of this recursive function.
    */
    if(vwnd == marker->catalyst)
    {
        /*
            if the window was a managed window then we need to blit all the
            unamanged windows just in case.  it would be much more efficient to
            only blit the affected windows but that would take some addtional
            recursive logic.
        */
        if(marker->managed == TRUE)
        {
            viper_window_for_each(screen_id, FALSE, VECTOR_BOTTOM_TO_TOP,
                viper_callback_blit_window, NULL);
        }

        free(marker);
        marker = NULL;

        if(viper->console_mouse != NULL)
        overwrite(viper->console_mouse, CURRENT_SCREEN);
    }

    return;
}

bool
is_viper_window_visible(vwnd_t *vwnd)
{
    if(vwnd == NULL) return FALSE;

    if(vwnd->window_state & STATE_VISIBLE) return TRUE;

    return FALSE;
}

