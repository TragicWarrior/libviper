#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ncursesw/curses.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"
#include "vk_dropdown.h"
#include "vk_window.h"
#include "vk_item.h"
#include "vk_event.h"

static int
_vk_dropdown_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_dropdown_dtor(vk_object_t *object);

static int
_vk_dropdown_update(vk_listbox_t *listbox);

require_klass(VK_LISTBOX_KLASS);

declare_klass(VK_DROPDOWN_KLASS)
{
    .size = KLASS_SIZE(vk_dropdown_t),
    .name = KLASS_NAME(vk_dropdown_t),
    .ctor = _vk_dropdown_ctor,
    .dtor = _vk_dropdown_dtor,
};

inline vk_dropdown_t*
vk_dropdown_create(int width, int max_visible)
{
    vk_dropdown_t   *dropdown;
    int             height;

    if(width < 3) return NULL;
    if(max_visible < 1) max_visible = 5;

    height = 1;

    dropdown = (vk_dropdown_t *)vk_object_create(VK_DROPDOWN_KLASS,
        width, height, max_visible);

    return dropdown;
}

inline int
vk_dropdown_set_relief_style(vk_dropdown_t *dropdown, int style)
{
    vk_widget_t *widget;

    if(dropdown == NULL) return -1;

    dropdown->relief_style = style;

    widget = VK_WIDGET(dropdown);

    if(style == VK_FRAME_SINGLE || style == VK_FRAME_ASCII)
        dropdown->collapsed_height = 3;
    else
        dropdown->collapsed_height = 1;

    vk_widget_resize(widget, widget->width, dropdown->collapsed_height);

    return 0;
}

inline int
vk_dropdown_set_expanded(vk_dropdown_t *dropdown, bool expanded)
{
    if(dropdown == NULL) return -1;
    if(dropdown->expanded == expanded) return 0;

    dropdown->expanded = expanded;

    if(!expanded && dropdown->popup != NULL)
    {
        vk_listbox_t *lb = dropdown->popup_listbox;
        dropdown->popup_listbox = NULL;
        vk_window_destroy(dropdown->popup);
        dropdown->popup = NULL;
        vk_listbox_destroy(lb);
    }

    if(expanded)
    {
        vk_listbox_t        *listbox;
        vk_listbox_t        *popup_lb;
        vk_widget_t         *widget;
        vk_item_t           *item;
        struct list_head    *pos;
        int                 list_h;
        int                 w;

        listbox = VK_LISTBOX(dropdown);
        widget = VK_WIDGET(dropdown);
        w = widget->width;

        list_h = listbox->item_count;
        if(list_h > dropdown->max_visible) list_h = dropdown->max_visible;
        if(list_h < 1) list_h = 1;

        dropdown->popup = vk_window_create(w, list_h + 2);
        vk_window_set_border_style(dropdown->popup, VK_FRAME_SINGLE);
        vk_window_set_border_colors(dropdown->popup,
            widget->fg, widget->bg);

        popup_lb = vk_listbox_create(w - 2, list_h);
        vk_widget_set_colors(VK_WIDGET(popup_lb), widget->fg, widget->bg);
        vk_listbox_set_highlight(popup_lb,
            listbox->highlight_fg, listbox->highlight_bg);

        list_for_each(pos, &listbox->item_list)
        {
            item = list_entry(pos, vk_item_t, list);
            if(item->name != NULL)
                vk_listbox_add_item(popup_lb, item->name, NULL, NULL);
        }

        vk_listbox_set_curr(popup_lb, listbox->curr_item);
        vk_listbox_update(popup_lb);

        vk_window_set_child(dropdown->popup, VK_WIDGET(popup_lb));
        dropdown->popup_listbox = popup_lb;

        vk_window_update(dropdown->popup);
    }

    VK_LISTBOX(dropdown)->_update(VK_LISTBOX(dropdown));

    return 0;
}

inline bool
vk_dropdown_get_expanded(vk_dropdown_t *dropdown)
{
    if(dropdown == NULL) return false;

    return dropdown->expanded;
}

inline vk_widget_t*
vk_dropdown_get_popup(vk_dropdown_t *dropdown)
{
    if(dropdown == NULL) return NULL;

    return VK_WIDGET(dropdown->popup);
}

inline int
vk_dropdown_popup_navigate(vk_dropdown_t *dropdown, int direction)
{
    if(dropdown == NULL) return -1;
    if(dropdown->popup_listbox == NULL) return -1;

    if(direction < 0)
        vk_listbox_set_prev(dropdown->popup_listbox);
    else
        vk_listbox_set_next(dropdown->popup_listbox);

    vk_listbox_update(dropdown->popup_listbox);
    vk_window_update(dropdown->popup);

    return 0;
}

inline int
vk_dropdown_popup_select(vk_dropdown_t *dropdown)
{
    vk_listbox_t *listbox;
    int sel;

    if(dropdown == NULL) return -1;
    if(dropdown->popup_listbox == NULL) return -1;

    listbox = VK_LISTBOX(dropdown);
    sel = vk_listbox_get_curr(dropdown->popup_listbox);

    vk_listbox_set_curr(listbox, sel);

    vk_dropdown_set_expanded(dropdown, false);

    return 0;
}

inline int
vk_dropdown_update(vk_dropdown_t *dropdown)
{
    if(dropdown == NULL) return -1;

    return VK_LISTBOX(dropdown)->_update(VK_LISTBOX(dropdown));
}

inline void
vk_dropdown_destroy(vk_dropdown_t *dropdown)
{
    if(dropdown == NULL) return;

    if(!vk_object_assert(dropdown, vk_dropdown_t)) return;

    dropdown->dtor(VK_OBJECT(dropdown));
}

static int
_vk_dropdown_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_dropdown_t   *dropdown;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_LISTBOX_KLASS->ctor(object, argp);

    dropdown = VK_DROPDOWN(object);

    dropdown->max_visible = va_arg(*argp, int);
    dropdown->expanded = false;
    dropdown->relief_style = VK_BUTTON_BASIC;
    dropdown->collapsed_height = 1;
    dropdown->popup = NULL;
    dropdown->popup_listbox = NULL;

    va_end(args);

    dropdown->ctor = _vk_dropdown_ctor;
    dropdown->dtor = _vk_dropdown_dtor;

    VK_LISTBOX(dropdown)->_update = _vk_dropdown_update;

    return 0;
}

static int
_vk_dropdown_dtor(vk_object_t *object)
{
    vk_dropdown_t *dropdown;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_dropdown_t)) return -1;

    dropdown = VK_DROPDOWN(object);

    if(dropdown->popup != NULL)
    {
        vk_listbox_t *lb = dropdown->popup_listbox;
        dropdown->popup_listbox = NULL;
        vk_window_destroy(dropdown->popup);
        dropdown->popup = NULL;
        vk_listbox_destroy(lb);
    }

    vk_object_demote(object, vk_listbox_t);

    VK_LISTBOX(object)->dtor(object);

    return 0;
}

static int
_vk_dropdown_update(vk_listbox_t *listbox)
{
    vk_dropdown_t       *dropdown;
    vk_widget_t         *widget;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_colors;
    int                 name_width;
    int                 idx = 0;
    const char          *text = "";
    cchar_t             cc;

    if(listbox == NULL) return -1;

    dropdown = VK_DROPDOWN(listbox);
    widget = VK_WIDGET(listbox);

    widget->_erase(widget);

    paint_colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg))
        | widget->attrs;

    list_for_each(pos, &listbox->item_list)
    {
        if(idx == listbox->curr_item)
        {
            item = list_entry(pos, vk_item_t, list);
            if(item->name != NULL) text = item->name;
            break;
        }
        idx++;
    }

    if(dropdown->relief_style == VK_FRAME_SINGLE)
    {
        short hi_pair = vdk_color_pair(COLOR_WHITE, widget->bg);
        short sh_pair = vdk_color_pair(COLOR_BLACK, widget->bg);
        int row;

        vk_widget_fill(widget, ' ' | paint_colors);

        row = 0;
        {
            wchar_t wch[2] = { 0 };
            attr_t attrs;
            short dummy;

            getcchar(WACS_ULCORNER, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs | widget->attrs, hi_pair, NULL);
            mvwadd_wch(widget->canvas, 0, 0, &cc);

            getcchar(WACS_HLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs | widget->attrs, hi_pair, NULL);
            for(int c = 1; c < widget->width - 3; c++)
                mvwadd_wch(widget->canvas, 0, c, &cc);

            getcchar(WACS_TTEE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, 0, widget->width - 3, &cc);

            getcchar(WACS_HLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, 0, widget->width - 2, &cc);

            getcchar(WACS_URCORNER, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, 0, widget->width - 1, &cc);
        }

        row = 1;
        name_width = widget->width - 5;
        if(name_width < 1) name_width = 1;

        {
            wchar_t wch[2] = { 0 };
            attr_t attrs;
            short dummy;

            getcchar(WACS_VLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs | widget->attrs, hi_pair, NULL);
            mvwadd_wch(widget->canvas, row, 0, &cc);
        }

        wattron(widget->canvas, paint_colors);
        mvwprintw(widget->canvas, row, 1, " %-*.*s",
            name_width, name_width, text);
        wattroff(widget->canvas, paint_colors);

        {
            wchar_t wch[2] = { 0 };
            attr_t attrs;
            short dummy;

            getcchar(WACS_VLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, row, widget->width - 3, &cc);
        }

        wattron(widget->canvas, paint_colors);
        mvwaddstr(widget->canvas, row, widget->width - 2, "\xe2\x96\xbc");
        wattroff(widget->canvas, paint_colors);

        {
            wchar_t wch[2] = { 0 };
            attr_t attrs;
            short dummy;

            getcchar(WACS_VLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, row, widget->width - 1, &cc);
        }

        row = 2;
        {
            wchar_t wch[2] = { 0 };
            attr_t attrs;
            short dummy;

            getcchar(WACS_LLCORNER, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs | widget->attrs, hi_pair, NULL);
            mvwadd_wch(widget->canvas, row, 0, &cc);

            getcchar(WACS_HLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            for(int c = 1; c < widget->width - 3; c++)
                mvwadd_wch(widget->canvas, row, c, &cc);

            getcchar(WACS_BTEE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, row, widget->width - 3, &cc);

            getcchar(WACS_HLINE, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, row, widget->width - 2, &cc);

            getcchar(WACS_LRCORNER, wch, &attrs, &dummy, NULL);
            setcchar(&cc, wch, attrs, sh_pair, NULL);
            mvwadd_wch(widget->canvas, row, widget->width - 1, &cc);
        }
    }
    else
    {
        name_width = widget->width - 2;
        if(name_width < 1) name_width = 1;

        vk_widget_fill(widget, ' ' | paint_colors);
        wattron(widget->canvas, paint_colors);
        mvwprintw(widget->canvas, 0, 0, "%-*.*s",
            name_width, name_width, text);
        mvwaddstr(widget->canvas, 0, name_width, " \xe2\x96\xbc");
        wattroff(widget->canvas, paint_colors);
    }

    return 0;
}
