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
#include "viper_wdecorate.h"
#include "list.h"

void
viper_window_modify_border(WINDOW *window, gint attrs, gshort colors)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;

    if(list_empty(&viper->wnd_list)) return;

    viper_wnd = viper_get_viper_wnd(window);

    if(viper_wnd != NULL)
        window_modify_border(viper_wnd->window, attrs, colors);

    return;
}

void
viper_window_set_border_agent(WINDOW *window, VIPER_FUNC agent, int id)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;

    if(list_empty(&viper->wnd_list)) return;

    if(id < 1) return;
    viper_wnd = viper_get_viper_wnd(window);

    if(viper_wnd != NULL) viper_wnd->border_agent[id] = agent;

    return;
}
