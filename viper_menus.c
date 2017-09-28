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
#include "strings.h"

MENU*
viper_menu_create(char **items)
{
    MENU    *menu;
    ITEM    **item_list;
    char    *item;
    int     count = 0;
    int     i;

    if(items[0] == NULL) return NULL;
    while(items[count] != NULL) count++;

    /* new_menu() expects a NULL terminated list.  add 1 item to count */
    item_list = (ITEM**)calloc(1, sizeof(ITEM*) * (count + 1));
    for(i = 0;i < count;i++)
    {
        /*    create a copy of the item so the user does not have to worry about
            maintaining the item statically and/or accidentaly freeing the
            item and causing a seg fault    */
        item = strdup(items[i]);
        item_list[i] = new_item(item, NULL);
    }

    /* terminate list with NULL   */
    item_list[count] = NULL;

    menu = new_menu(item_list);
    return menu;
}

void
viper_menu_items_add(MENU *menu, char **items)
{
    char    **list_copy;
    ITEM    **item_list_old;
    ITEM    **item_list_new;
    int     new_count = 0;
    int     old_count = 0;
    int     count;
    int     i = 0;

    if(items == NULL) return;
    if(items[0] == NULL) return;

    unpost_menu(menu);

    /* count the new and existing items  */
    while(items[new_count] != NULL) new_count++;
    old_count = item_count(menu);
    count = old_count + new_count;

    /* allocate storage for total number of items   */
    item_list_new = (ITEM**)calloc(1, (count + 1) * sizeof(ITEM*));
    item_list_old = menu_items(menu);

    /* copy the old text items into the combined list  */
    memcpy(item_list_new, item_list_old,count * sizeof(ITEM*));

    /* copy the users addtl item list   */
    list_copy = strdupv(items);

    for(i = old_count;i < count;i++)
    {
        item_list_new[i] = new_item(list_copy[i-old_count],NULL);
    }

    /* copy the new text items into the combined list  */
    free(list_copy);
    set_menu_items(menu, item_list_new);
    free(item_list_old);

    return;
}

void
viper_menu_items_change(MENU *menu, char **items)
{
    ITEM    **item_list_old;
    ITEM    **item_list_new;
    char    *item;
    int     new_count = 0;
    int     old_count;
    int     i;

    if(items == NULL) return;
    if(items[0] == NULL) return;

    while(items[new_count] != NULL) new_count++;

    unpost_menu(menu);

    old_count = item_count(menu);
    item_list_old = menu_items(menu);

    item_list_new = (ITEM**)calloc(1, sizeof(ITEM*) * (new_count + 1));
    for(i = 0;i < new_count;i++)
    {
        /*
            create a copy of the item so the user does not have to worry about
            maintaining the item statically and/or accidentaly freeing the
            item and causing a seg fault
        */
        item = strdup(items[i]);
        item_list_new[i] = new_item(item,NULL);
    }

    /* terminate list with NULL   */
    item_list_new[new_count] = NULL;

    /* attach new item list */
    set_menu_items(menu, item_list_new);

    /* free all of the text associated with the old menu items  */
    for(i = 0;i < old_count;i++)
    {
        item = (char*)item_name(item_list_old[i]);
        free_item(item_list_old[i]);
        free(item);
    }
    free(item_list_old);

    return;
}

/*
    this function reparents a menu, places size restrictions on the menu, and
    determines where the menu will be place on the parent.  the return value
    is the pointer to the new subwin which is the container for the menu. a
    value of -1 for width and height indicate that values from scale_menu should
    be used if possible.
*/
WINDOW*
viper_menu_bind(MENU *menu, WINDOW *parent, float x, float y,
         float width, float height)
{
    WINDOW  *container;
    int     tmp_width;
    int     tmp_height;
    int     retval;

    if(parent == NULL || menu == NULL) return NULL;

    if(width > 0 && width < 1)
    {
        window_get_size_scaled(parent, &tmp_width, NULL, width, height);
        width = tmp_width;
    }
    if(width == -1)
    {
        scale_menu(menu, &tmp_width, &tmp_height);
        width = tmp_width;
    }

    if(height > 0 && height < 1)
    {
        window_get_size_scaled(parent, NULL, &tmp_height, width, height);
        height = tmp_height;
    }
    if(height == -1)
    {
        scale_menu(menu, &tmp_width, &tmp_height);
        height = tmp_height;
    }

    getmaxyx(parent, tmp_height, tmp_width);
    if(x>0 && x<1) x = (tmp_width - width) * x;
    if(y>0 && y<1) y = (tmp_height - height) * y;

    /* check for out-of-bounds */
    if((width + x) > tmp_width || (height + y) > tmp_height) return NULL;
    container = derwin(parent, height, width, y, x);

    retval = unpost_menu(menu);
    set_menu_format(menu, height, width);
    set_menu_win(menu, container);

    if(retval != E_NOT_POSTED) post_menu(menu);
    return container;
}

/*
    if you created a menu with viper_menu_create(), then setting a value of TRUE
    is safe for both free_items and free_text.  viper_create_menu() dynamically
    allocates storage for both of these.
*/
void
viper_menu_destroy(MENU *menu, bool free_windows)
{
    WINDOW  *parent;
    WINDOW  *window;
    WINDOW  *subwin;
    ITEM    **items;
    int     count;
    char    *text;
    int     i = 0;

    unpost_menu(menu);

    window = menu_win(menu);
    subwin = menu_sub(menu);
    parent = window->_parent;

    count = item_count(menu);
    if(count == ERR || count < 1) return;  /* currently unhandled error  */

    items = menu_items(menu);
    if(items == NULL) return;             /* currently unhandled error  */

    /* disconnect menu from item array and free resources used by menu   */
    if(free_menu(menu) != E_OK) return;

    for(i = 0;i < count;i++)
    {
        text = (char*)item_name(items[i]);
        if(text != NULL) free(text);
        free_item(items[i]);
    }

    /* free the list which contained the items   */
    free(items);

    if(free_windows == FALSE) return;

    /*    delete subwin if it is completely independent.    */
    if(subwin != parent && subwin != window && subwin != NULL) delwin(subwin);

    /*    delete window if it is not the parent window.    */
    if(parent != NULL && window != parent && window != NULL) delwin(window);

    return;
}
