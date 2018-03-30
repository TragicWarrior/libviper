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

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "viper.h"
#include "private.h"
#include "viper_color.h"
#include "viper_callbacks.h"
#include "viper_kmio.h"
#include "viper_wdestroy.h"

WINDOW      *SCREEN_WINDOW = NULL;
VIPER       *viper = NULL;
uint32_t    viper_global_flags;

VIPER*
viper_init(uint32_t init_flags)
{
    extern VIPER                *viper;
    extern WINDOW               *SCREEN_WINDOW;
    viper_screen_t              *viper_screen = NULL;
    int                         width, height;
    char                        *env;
    mmask_t                     mouse_mask = ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION;
    extern uint32_t             viper_global_flags;
    struct termios              term_desc;
    int                         i;

    if(viper == NULL)
    {
        setlocale(LC_ALL, "UTF-8");

        // newterm(NULL, stdout, stdin);

        SCREEN_WINDOW = initscr();

        viper_global_flags |= init_flags;
        viper = (VIPER*)calloc(1, sizeof(VIPER));

        viper->viper_screen[0].screen = SCREEN_WINDOW;
        viper->cur_scr_id = 0;

        viper_color_init();
        env = getenv("TERM");
        if(strstr(env, "xterm") != NULL) viper->xterm=TRUE;
        viper->user = getuid();
        getmaxyx(SCREEN_WINDOW, height, width);
        viper->border_agent[0] = viper_default_border_agent_unfocus;
        viper->border_agent[1] = viper_default_border_agent_focus;
        mousemask(mouse_mask, NULL);

        for(i = 0; i < MAX_SCREENS; i++)
        {
            viper_screen = &viper->viper_screen[i];

            INIT_LIST_HEAD(&viper_screen->managed_list);
            INIT_LIST_HEAD(&viper_screen->unmanaged_list);

            viper_screen->wallpaper = newwin(height, width, 0, 0);
            viper_screen->wallpaper_agent = viper_default_wallpaper_agent;
        }

        INIT_LIST_HEAD(&viper->zombie_list);

        /*
            these are "normal" settings that would be commonly
            configured for use with the library.  the user can always
            change them back.
        */
        keypad(SCREEN_WINDOW, TRUE);
        nodelay(SCREEN_WINDOW, TRUE);
        scrollok(SCREEN_WINDOW, FALSE);
        noecho();
        raw();
        intrflush(NULL, TRUE);
        curs_set(0);

        /*
            ncurses initscr() is supposed to do this but isn't (at least in
            some versions of ncurses.
        */
        tcgetattr(STDIN_FILENO, &term_desc);
        term_desc.c_lflag &= ~(ECHO);
        tcsetattr(STDIN_FILENO,TCSADRAIN,&term_desc);
    }

    return viper;
}

void
viper_end(void)
{
    extern VIPER        *viper;
    extern WINDOW       *SCREEN_WINDOW;
    struct termios      term_desc;

#if !defined(_NO_GPM) && defined(__linux)
    viper_kmio_gpm(NULL,CMD_GPM_CLOSE);
#endif

    if(viper != NULL)
    {
        free(viper);
        viper = NULL;
    }

    curs_set(1);
    endwin();
    SCREEN_WINDOW = NULL;

    /* clean up the echo suppression hack  */
    tcgetattr(STDIN_FILENO, &term_desc);
    term_desc.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO,TCSADRAIN,&term_desc);

    return;
}


void
viper_set_border_agent(ViperFunc agent, int id)
{
    extern VIPER    *viper;

    if(id > 1) return;

    viper->border_agent[id] = agent;

    return;
}

WINDOW*
viper_window_get_frame(vwnd_t *vwnd)
{
    if(vwnd == NULL) return NULL;

    return vwnd->window_frame;
}

void
viper_window_for_each(int screen_id, bool managed, int vector,
    ViperFunc func, void *arg)
{
    extern VIPER        *viper;
    viper_screen_t      *viper_screen;
    vwnd_t              *vwnd;
    struct list_head    *pos;
    struct list_head    *wnd_list;

    if(func == NULL) return;

    if(screen_id == -1) screen_id = CURRENT_SCREEN_ID;

    viper_screen = &viper->viper_screen[screen_id];

    if(managed == TRUE)
        // wnd_list = &viper->managed_list[screen_id];
        wnd_list = &viper_screen->managed_list;
    else
        // wnd_list = &viper->unmanaged_list[screen_id];
        wnd_list = &viper_screen->unmanaged_list;

    if(list_empty(wnd_list)) return;

    if(vector == VECTOR_BOTTOM_TO_TOP)
    {
        list_for_each_prev(pos, wnd_list)
        {
            vwnd = list_entry(pos, VIPER_WND, list);
            func(vwnd, arg);
        }

    }
    else
    {
        list_for_each(pos, wnd_list)
        {
            vwnd = list_entry(pos, VIPER_WND, list);
            func(vwnd, arg);
        }
    }

    return;
}

int
viper_prune_zombie_list(void)
{
    extern viper_t      *viper;
    struct list_head    *pos;
    struct list_head    *tmp;
    vwnd_t              *vwnd;

    if(list_empty(&viper->zombie_list)) return 0;

    list_for_each_safe(pos, tmp, &viper->zombie_list)
    {
        vwnd = list_entry(pos, vwnd_t, list);

        viper_window_destroy(vwnd);

        list_del(pos);
    }

    return 0;
}
