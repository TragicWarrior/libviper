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
#include "viper_states.h"
#include "viper_kmio.h"

void
viper_window_set_key_func(vwnd_t *vwnd, ViperWkeyFunc func)
{
    if(vwnd == NULL) return;

    if(vwnd != NULL) vwnd->key_func = func;

    return;
}

ViperWkeyFunc
viper_window_get_key_func(vwnd_t *vwnd)
{
    if(vwnd == NULL) return NULL;

    return vwnd->key_func;
}
