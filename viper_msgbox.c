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

#include <inttypes.h>
#include <string.h>

#include "viper.h"
#include "viper_msgbox.h"
#include "viper_events.h"
#include "strings.h"

WINDOW*
viper_msgbox_create(char *title, float x, float y,
    int width, int height, char *msg, uint32_t flags)
{
    WINDOW      *window;
    char        *icons[]={" II "," WW "," EE "," ?? "};
    chtype      icon_colors[] = {
                    VIPER_COLORS(COLOR_WHITE,COLOR_BLACK),
                    VIPER_COLORS(COLOR_YELLOW,COLOR_BLACK),
                    VIPER_COLORS(COLOR_RED,COLOR_BLACK),
                    VIPER_COLORS(COLOR_BLUE,COLOR_BLACK)};

    char        **msg_dissect = NULL;
    char        **pos;
    int         longest_line;
    int         min_width;
    int         min_height;
    char        *prompt = NULL;
    int         idx = -1;
    int         tmp;

    if(msg == NULL) return NULL;
    msg_dissect = strsplitv(msg, "\n\r");
    longest_line = calc_msgbox_metrics(msg_dissect, &min_width, &min_height);

    if(height < 1) height = min_height;
    if(width < 1) width = min_width;

    if(flags & MSGBOX_ICON_INFO) idx = 0;
    if(flags & MSGBOX_ICON_WARN) idx = 1;
    if(flags & MSGBOX_ICON_ERROR) idx = 2;
    if(flags & MSGBOX_ICON_QUESTION) idx = 3;

    if((flags & MSGBOX_TYPE_OK) || (flags & MSGBOX_TYPE_YESNO)) height += 2;
    if(flags & MSGBOX_TYPE_OK) prompt = "[Enter] Okay";
    if(flags & MSGBOX_TYPE_YESNO) prompt = "[Y]es | [N]o";

    if(idx != -1)
    {
        tmp = strlen(msg_dissect[0]) + strlen(icons[idx]) + 2;
        if(tmp > width)
        {
            width = tmp;
            longest_line = 0;
        }
    }

    window = viper_window_create(title, x, y, width + 2, height + 2, TRUE);
    getmaxyx(window, height, width);
    wresize(window, height - 2, width - 2);
    mvderwin(window, 2, 2);

    wmove(window, 0, 0);
    if(idx != -1)
    {
        wattron(window, icon_colors[idx] | A_REVERSE);
        wprintw(window, "%s", icons[idx], msg);
        wattroff(window, icon_colors[idx] | A_REVERSE);
        wprintw(window," ");
    }

    /* checking the _WRAPPED flag may not be portable. */
    pos = msg_dissect;
    while(*pos != NULL)
    {
        if(strlen(*pos) == width - 2) wprintw(window, "%s", *pos);
        else wprintw(window, "%s\n", *pos);
        pos++;
    }
    strfreev(msg_dissect);

    if(prompt != NULL)
    {
        tmp = (width - 1 - strlen(prompt)) / 2;
        mvwprintw(window, height - 3, tmp, prompt);
    }

    if(flags & MSGBOX_FLAG_EMINENT)
    {
        viper_window_set_state(window, STATE_EMINENT);
        viper_event_set(window, "window-close",
            viper_event_default_MSGBOX_CLOSE,NULL);
    }

    if(flags & MSGBOX_TYPE_OK)
        viper_window_set_key_func(window, viper_kbd_default_MSGBOX_OK);

    // squelch compiler warning
    (void)longest_line;

    return window;
}

/*    convenience function for msgbox flag MSGBOX_FLAG_OK    */
int
viper_kbd_default_MSGBOX_OK(int32_t keystroke, WINDOW *window)
{
    if(keystroke != KEY_CRLF) return 1;

    viper_window_destroy(window);

    return 1;
}

int
calc_msgbox_metrics(char **msg_array, int *width, int *height)
{
    int     line_width = 0;
    int     line_count = 0;
    int     idx = 0;

    *height = 0;
    *width = 0;
    while(*msg_array != NULL)
    {
        line_count++;
        line_width = strlen(*msg_array);
        if(line_width > (*width))
        {
            *width = line_width;
            idx = line_count - 1;
        }

        msg_array++;
    }

    *height = line_count;

    /* return the index of the longest line.  */
    return idx;
}
