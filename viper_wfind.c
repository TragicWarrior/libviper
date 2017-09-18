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

WINDOW*
viper_window_find_by_class(gpointer classid)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;
    GSList          *node;

    if(viper->wnd_count == 0) return NULL;
    node = viper->wnd_list;
    while(node != NULL)
    {
        viper_wnd = (VIPER_WND*)node->data;
        if(viper_wnd->classid == classid) break;
        node = node->next;
    }

    if(node != NULL) return viper_wnd->user_window;

    return NULL;
}

WINDOW*
viper_window_find_by_title(gchar *title)
{
    extern VIPER    *viper;
    VIPER_WND       *viper_wnd;
    GSList          *node;

    if(viper->wnd_count == 0) return NULL;
    node = viper->wnd_list;
    while(node != NULL)
    {
        viper_wnd = (VIPER_WND*)node->data;
        if(strcmp(viper_wnd->title,title) == 0) break;
        node = node->next;
    }

    if(node != NULL) return viper_wnd->user_window;

    return NULL;
}
