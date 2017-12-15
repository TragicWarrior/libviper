#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"
#include "vk_menu.h"
#include "vk_item.h"

// base klass methods
static int
_vk_menu_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_menu_dtor(vk_object_t *object);

static int
_vk_menu_kmio(vk_object_t *object, int32_t keystroke);

// super klass methods
static int
_vk_menu_set_frame(vk_menu_t *menu, int style);

static int
_vk_menu_add_separator(vk_menu_t *menu, int style);

static int
_vk_menu_update(vk_menu_t *menu);

static int
_vk_menu_reset(vk_menu_t *menu);

require_klass(VK_LISTBOX_KLASS);

declare_klass(VK_MENU_KLASS)
{
    .size = KLASS_SIZE(vk_menu_t),
    .name = KLASS_NAME(vk_menu_t),
    .ctor = _vk_menu_ctor,
    .dtor = _vk_menu_dtor,
    .kmio = _vk_menu_kmio,
};


// create a new widget from scratch
vk_menu_t*
vk_menu_create(int width, int height)
{
    vk_menu_t   *menu;

    if(height == 0 || width == 0) return NULL;

    menu = (vk_menu_t*)vk_object_create(VK_MENU_KLASS, width, height);

    return menu;
}


int
vk_menu_update(vk_menu_t *menu)
{
    if(menu == NULL) return -1;

    // make sure the item is actually a listbox widget
    if(!vk_object_assert(menu, vk_menu_t)) return -1;

    menu->_update(menu);

    return 0;
}

int
vk_menu_set_frame(vk_menu_t *menu, int style)
{
    int retval;

    if(menu == NULL) return -1;

    retval = menu->_set_frame(menu, style);

    return retval;
}

int
vk_menu_add_separator(vk_menu_t *menu, int style)
{
    if(menu == NULL) return -1;

    menu->_add_separator(menu, style);

    return 0;
}

int
vk_menu_reset(vk_menu_t *menu)
{
    int retval;

    if(menu == NULL) return -1;

    // make sure the item is actually a listbox widget
    if(!vk_object_assert(menu, vk_menu_t)) return -1;

    retval = VK_LISTBOX(menu)->_reset(VK_LISTBOX(menu));

    return retval;
}

void
vk_menu_destroy(vk_menu_t *menu)
{
    if(menu == NULL) return;

    if(!vk_object_assert(menu, vk_menu_t)) return;

    menu->dtor(VK_OBJECT(menu));

    return;
}


static int
_vk_menu_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_menu_t   *menu;
    va_list     args;

    if(object == NULL) return -1;

    /*
        if argp is set then we're being called by a superclass.
        otherwise, we're being called directly.
    */
    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;

        VK_LISTBOX_KLASS->ctor(object, &args);
        va_end(args);
    }
    else
    {
        VK_LISTBOX_KLASS->ctor(object, argp);
    }

    // install our derived klass methods
    menu = VK_MENU(object);

    menu->ctor = _vk_menu_ctor;
    menu->dtor = _vk_menu_dtor;

    menu->_set_frame = _vk_menu_set_frame;
    menu->_add_separator = _vk_menu_add_separator;

    menu->_update = _vk_menu_update;
    menu->_reset = _vk_menu_reset;

    INIT_LIST_HEAD(&VK_LISTBOX(menu)->item_list);

    return 0;
}


static int
_vk_menu_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_menu_t)) return -1;

    // destroy all the list items
    VK_MENU(object)->_reset(VK_MENU(object));

    vk_object_demote(object, vk_listbox_t);
    vk_listbox_destroy(VK_LISTBOX(object));

    return 0;
}

static int
_vk_menu_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_menu_t       *menu;
    vk_listbox_t    *listbox;
    int             retval;

    menu = VK_MENU(object);
    listbox = VK_LISTBOX(object);

    if(list_empty(&listbox->item_list)) return 0;

    switch(keystroke)
    {
        case KEY_UP:    listbox->curr_item--;           break;
        case KEY_DOWN:  listbox->curr_item++;           break;
        case 10:
        {
            retval = listbox->_exec_item(listbox);
            return retval;
        }
    }

    if(listbox->curr_item < 0)
    {
        if(listbox->flags & VK_FLAG_ALLOW_WRAP)
            listbox->curr_item = listbox->item_count - 1;
        else
            listbox->curr_item = 0;
    }

    if(listbox->curr_item > (listbox->item_count - 1))
    {
        if(listbox->flags & VK_FLAG_ALLOW_WRAP)
            listbox->curr_item = 0;
        else
            listbox->curr_item--;
    }

    // now cause the widget to redraw
    menu->_update(menu);
    VK_WIDGET(object)->_draw(VK_WIDGET(object));

    return 0;
}

static int
_vk_menu_set_frame(vk_menu_t *menu, int style)
{
    if(menu == NULL) return -1;

    menu->frame_style = style;

    return 0;
}

static int
_vk_menu_add_separator(vk_menu_t *menu, int style)
{
    vk_listbox_t    *listbox;
    vk_item_t       *item;

    if(menu == NULL) return -1;
    if(style < 1) style = VK_SEPARATOR_BLANK;

    listbox = VK_LISTBOX(menu);

    item = (vk_item_t *)calloc(1, sizeof(vk_item_t));
    item->separator_style = style;

    list_add_tail(&item->list, &listbox->item_list);
    listbox->item_count++;

    return 0;
}

static int
_vk_menu_update(vk_menu_t *menu)
{
    vk_widget_t         *widget;
    vk_listbox_t        *listbox;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_height;
    int                 paint_width;
    int                 paint_colors;
    int                 highlight;
    attr_t              highlight_attr = A_NORMAL;
    int                 idx = 0;
    int                 x = 0;
    int                 y = 0;

    if(menu == NULL) return -1;

    widget = VK_WIDGET(menu);
    listbox = VK_LISTBOX(menu);

    widget->_erase(widget);
    paint_height = widget->height;
    paint_width = widget->width;

    // if highlight colors net set, use inverted widget colors
    if(listbox->highlight_fg == -1) listbox->highlight_fg = widget->bg;
    if(listbox->highlight_bg == -1) listbox->highlight_bg = widget->fg;

    paint_colors = VIPER_COLORS(widget->fg, widget->bg);
    highlight = VIPER_COLORS(listbox->highlight_fg, listbox->highlight_bg);

    wattron(widget->canvas, paint_colors);

    if((listbox->title != NULL) || (menu->frame_style > 0))
    {
        paint_height--;
        y++;
    }

    if(menu->frame_style > 0)
    {
        if(menu->frame_style == VK_FRAME_SINGLE)
        {
            wborder(VK_WIDGET(menu)->canvas, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        paint_width -= 2;
        x++;
    }

    // if the title exists we need to show it and account for it
    if(listbox->title != NULL)
    {
        mvwaddstr(widget->canvas, 0, (widget->width / 2), listbox->title);
    }

    // set simple bounds when all items will fit in the paint area
    if(listbox->item_count <= paint_height)
    {
        listbox->scroll_top = 0;
    }


    listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);

    // clamp region when items don't fit in paint area
    if(listbox->item_count > paint_height)
    {
        while(listbox->curr_item < listbox->scroll_top)
        {
            listbox->scroll_top--;
            listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);
        }

        while(listbox->curr_item > listbox->scroll_bottom)
        {
            listbox->scroll_top++;
            listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);
        }
    }

    list_for_each(pos, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(idx < listbox->scroll_top)
        {
            idx++;
            continue;
        }

        if(idx > listbox->scroll_bottom) break;

        if(item->separator_style > 0)
        {
            if(item->separator_style == VK_SEPARATOR_BLANK)
            {
                mvwprintw(widget->canvas, y, x, "%-*c",
                    paint_width, ' ');
                highlight_attr = A_NORMAL;
            }

            if(item->separator_style == VK_SEPARATOR_SINGLE)
            {
                mvwhline(widget->canvas, y, x, ACS_HLINE, paint_width);
                highlight_attr = A_ALTCHARSET;
            }
        }
        else
        {
            mvwprintw(widget->canvas, y, x, "%-*.*s",
                paint_width,
                paint_width,
                item->name);
            highlight_attr = A_NORMAL;
        }

        if(idx == listbox->curr_item)
        {
            mvwchgat(widget->canvas, y, x, paint_width,
                highlight_attr, PAIR_NUMBER(highlight), NULL);
        }

        y++;
        idx++;
    }

    while(y < paint_height)
    {
        mvwprintw(widget->canvas, y, x, "%-*c",
            paint_width, ' ');

        y++;
    }

    return 0;
}


// this might be unnecessary if reset ends up
// not being significantly different
static int
_vk_menu_reset(vk_menu_t *menu)
{
    if(menu == NULL) return -1;

    VK_LISTBOX(menu)->_reset(VK_LISTBOX(menu));

    return 0;
}
