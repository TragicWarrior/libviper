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

gint
viper_form_driver(FORM *form, gint request, guint32 flags,
    chtype active, chtype normal, gshort cursor_color)
{
    WINDOW  *window;
    chtype  eraser;
    chtype  temp_ch;
    gint    x, y;
    gint    retval;
    gshort  fg, bg;

    if(form == NULL) return ERR;

    if(form_sub(form) != form_win(form)) window = form_sub(form);
    else window = form_win(form);

    getyx(window, y, x);
    eraser = field_back(current_field(form));
    mvwchgat(window, y, x, 1, (eraser & A_ATTRIBUTES),
        PAIR_NUMBER(eraser & A_COLOR), NULL);

    retval = form_driver(form, request);

    if(flags & FORM_COLORIZE)
        viper_form_colorize(form, active, normal, active, normal);

    if(flags & FORM_CURSOR_NONE) return retval;

    temp_ch = termattrs();
    if((flags & FORM_CURSOR_ULINE) && !(temp_ch & A_UNDERLINE)) return ERR;

    getyx(window, y, x);
    temp_ch = field_fore(current_field(form));
    if(flags & FORM_CURSOR_ULINE)
        mvwchgat(window, y, x, 1, (temp_ch & A_ATTRIBUTES) | A_UNDERLINE,
            PAIR_NUMBER(temp_ch & A_COLOR),NULL);
    else
    {
        pair_content(PAIR_NUMBER(temp_ch & A_COLOR), &fg, &bg);
        if(cursor_color!=-1)
        {
            bg = cursor_color;
            mvwchgat(window, y, x, 1, A_NORMAL,
                viper_color_pair(fg, bg), NULL);
        }
        else
            mvwchgat(window, y, x, 1, A_REVERSE,
                viper_color_pair(fg, bg), NULL);
    }

    return E_OK;
}

void
viper_form_colorize(FORM *form, chtype field_active, chtype field_normal,
    chtype text_active, chtype text_normal)
{
    FIELD   **fields;

    fields = form_fields(form);
    while(*fields != NULL)
    {
        if(*fields == current_field(form))
        {
            set_field_fore(*fields, text_active);
            set_field_back(*fields, field_active);
        }
        else
        {
            set_field_fore(*fields, text_normal);
            set_field_back(*fields, field_normal);
        }

        fields++;
    }

    return;
}

void
viper_form_destroy(FORM *form, gboolean free_windows)
{
    WINDOW  *parent;
	WINDOW	*window;
	WINDOW	*subwin;
	FIELD	**fields;
	gint	count;

	unpost_form(form);

	window = form_win(form);
	subwin = form_sub(form);
	parent = window->_parent;

	count = field_count(form);
	fields = form_fields(form);
	free_form(form);

	while(count > 0)
	{
		free_field(fields[count-1]);
		count--;
	}
	g_free(fields);

    if(free_windows == FALSE) return;

	/*	delete subwin if it is completely independent.	*/
	if(subwin != parent && subwin != window && subwin != NULL) delwin(subwin);

	/*	delete window if it is not the parent window.	*/
	if(parent != NULL && window != parent && window != NULL) delwin(window);

	return;
}
