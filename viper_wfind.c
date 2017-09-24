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
#include "list.h"

WINDOW*
viper_window_find_by_class(void *classid)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    struct list_head    *pos;

    if(list_empty(&viper->wnd_list)) return NULL;

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);
        if(viper_wnd->classid == classid) break;

        viper_wnd = NULL;
    }

    if(viper_wnd == NULL) return NULL;

    return viper_wnd->user_window;
}

WINDOW*
viper_window_find_by_title(char *title)
{
    extern VIPER        *viper;
    VIPER_WND           *viper_wnd;
    struct list_head    *pos;

    if(list_empty(&viper->wnd_list)) return NULL;

    list_for_each(pos, &viper->wnd_list)
    {
        viper_wnd = list_entry(pos, VIPER_WND, list);
        if(strcmp(viper_wnd->title,title) == 0) break;

        viper_wnd = NULL;
    }

    if(viper_wnd == NULL) return NULL;

    return viper_wnd->user_window;
}
