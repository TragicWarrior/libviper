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
#include <stdbool.h>

#include "viper.h"
#include "private.h"
#include "list.h"

vwnd_t*
viper_window_find_by_class(int screen_id, bool managed, void *classid)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd = NULL;
    struct list_head    *wnd_list;
    struct list_head    *pos;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return NULL;

    list_for_each(pos, wnd_list)
    {
        vwnd = list_entry(pos, vwnd_t, list);
        if(vwnd->classid == classid) break;

        vwnd = NULL;
    }

    return vwnd;
}

vwnd_t*
viper_window_find_by_title(int screen_id, bool managed, char *title)
{
    extern VIPER        *viper;
    vwnd_t              *vwnd = NULL;
    struct list_head    *wnd_list;
    struct list_head    *pos;

    if(managed == TRUE)
        wnd_list = &viper->managed_list[screen_id];
    else
        wnd_list = &viper->unmanaged_list[screen_id];

    if(list_empty(wnd_list)) return NULL;

    list_for_each(pos, wnd_list)
    {
        vwnd = list_entry(pos, vwnd_t, list);
        if(strcmp(vwnd->title, title) == 0) break;

        vwnd = NULL;
    }

    return vwnd;
}
