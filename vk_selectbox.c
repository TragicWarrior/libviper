#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_listbox.h"
#include "vk_selectbox.h"
#include "vk_item.h"
#include "vk_scroller.h"

static int
_vk_selectbox_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_selectbox_dtor(vk_object_t *object);

static int
_vk_selectbox_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_selectbox_update(vk_listbox_t *listbox);

static int
_vk_selectbox_on_resize(vk_widget_t *widget);

static int
_vk_selectbox_on_recreate(vk_widget_t *widget);

static vk_item_t*
_vk_selectbox_get_item_at(vk_selectbox_t *sb, int idx);

static const char*
_vk_selectbox_glyph(vk_selectbox_t *sb, bool checked, int *col_width);


require_klass(VK_LISTBOX_KLASS);

declare_klass(VK_SELECTBOX_KLASS)
{
    .size = KLASS_SIZE(vk_selectbox_t),
    .name = KLASS_NAME(vk_selectbox_t),
    .ctor = _vk_selectbox_ctor,
    .dtor = _vk_selectbox_dtor,
    .kmio = _vk_selectbox_kmio,
};


vk_selectbox_t*
vk_selectbox_create(int width, int height, int mode)
{
    vk_selectbox_t  *selectbox;

    if(width < 1 || height < 1) return NULL;

    selectbox = (vk_selectbox_t *)vk_object_create(VK_SELECTBOX_KLASS,
        width, height, mode);

    return selectbox;
}

int
vk_selectbox_set_style(vk_selectbox_t *selectbox, int style)
{
    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    selectbox->style = style;

    return 0;
}

int
vk_selectbox_set_wrap(vk_selectbox_t *selectbox, bool allowed)
{
    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    if(allowed)
        VK_LISTBOX(selectbox)->flags |= VK_FLAG_ALLOW_WRAP;
    else
        VK_LISTBOX(selectbox)->flags &= ~VK_FLAG_ALLOW_WRAP;

    return 0;
}

int
vk_selectbox_set_highlight(vk_selectbox_t *selectbox, int fg, int bg)
{
    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    VK_LISTBOX(selectbox)->highlight_fg = fg;
    VK_LISTBOX(selectbox)->highlight_bg = bg;

    return 0;
}

int
vk_selectbox_add_item(vk_selectbox_t *selectbox, char *name,
    VkWidgetFunc func, void *anything)
{
    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    return VK_LISTBOX(selectbox)->_add_item(VK_LISTBOX(selectbox),
        name, func, anything);
}

int
vk_selectbox_toggle_item(vk_selectbox_t *selectbox, int idx)
{
    vk_item_t   *item;

    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    item = _vk_selectbox_get_item_at(selectbox, idx);
    if(item == NULL) return -1;
    if(item->separator_style > 0) return -1;

    if(selectbox->mode == VK_SELECTBOX_RADIO)
    {
        vk_selectbox_uncheck_all(selectbox);
        item->flags |= VK_ITEM_CHECKED;
    }
    else
    {
        item->flags ^= VK_ITEM_CHECKED;
    }

    return 0;
}

bool
vk_selectbox_item_is_checked(vk_selectbox_t *selectbox, int idx)
{
    vk_item_t   *item;

    if(selectbox == NULL) return false;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return false;

    item = _vk_selectbox_get_item_at(selectbox, idx);
    if(item == NULL) return false;

    return (item->flags & VK_ITEM_CHECKED) ? true : false;
}

int
vk_selectbox_check_item(vk_selectbox_t *selectbox, int idx)
{
    vk_item_t   *item;

    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    item = _vk_selectbox_get_item_at(selectbox, idx);
    if(item == NULL) return -1;
    if(item->separator_style > 0) return -1;

    if(selectbox->mode == VK_SELECTBOX_RADIO)
        vk_selectbox_uncheck_all(selectbox);

    item->flags |= VK_ITEM_CHECKED;

    return 0;
}

int
vk_selectbox_uncheck_item(vk_selectbox_t *selectbox, int idx)
{
    vk_item_t   *item;

    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    item = _vk_selectbox_get_item_at(selectbox, idx);
    if(item == NULL) return -1;

    item->flags &= ~VK_ITEM_CHECKED;

    return 0;
}

int
vk_selectbox_uncheck_all(vk_selectbox_t *selectbox)
{
    vk_listbox_t        *listbox;
    vk_item_t           *item;
    struct list_head    *pos;

    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    listbox = VK_LISTBOX(selectbox);

    list_for_each(pos, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);
        item->flags &= ~VK_ITEM_CHECKED;
    }

    return 0;
}

int
vk_selectbox_update(vk_selectbox_t *selectbox)
{
    if(selectbox == NULL) return -1;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return -1;

    return VK_LISTBOX(selectbox)->_update(VK_LISTBOX(selectbox));
}

void
vk_selectbox_destroy(vk_selectbox_t *selectbox)
{
    if(selectbox == NULL) return;

    if(!vk_object_assert(selectbox, vk_selectbox_t)) return;

    selectbox->dtor(VK_OBJECT(selectbox));
}


static int
_vk_selectbox_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_selectbox_t  *selectbox;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_LISTBOX_KLASS->ctor(object, argp);

    selectbox = VK_SELECTBOX(object);

    selectbox->mode = va_arg(*argp, int);
    selectbox->style = VK_FRAME_SINGLE;

    va_end(args);

    selectbox->ctor = _vk_selectbox_ctor;
    selectbox->dtor = _vk_selectbox_dtor;

    VK_LISTBOX(selectbox)->_update = _vk_selectbox_update;

    VK_WIDGET(selectbox)->_on_resize = _vk_selectbox_on_resize;
    VK_WIDGET(selectbox)->_on_recreate = _vk_selectbox_on_recreate;

    return 0;
}

static int
_vk_selectbox_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_selectbox_t)) return -1;

    vk_object_demote(object, vk_listbox_t);

    VK_LISTBOX(object)->dtor(object);

    return 0;
}

static int
_vk_selectbox_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_selectbox_t  *selectbox;
    vk_listbox_t    *listbox;

    selectbox = VK_SELECTBOX(object);
    listbox = VK_LISTBOX(object);

    switch(keystroke)
    {
        case 10:
        case ' ':
        {
            vk_item_t *item;

            item = _vk_selectbox_get_item_at(selectbox, listbox->curr_item);
            if(item == NULL) return 0;
            if(item->separator_style > 0) return 0;

            if(selectbox->mode == VK_SELECTBOX_RADIO)
            {
                vk_selectbox_uncheck_all(selectbox);
                item->flags |= VK_ITEM_CHECKED;
            }
            else
            {
                item->flags ^= VK_ITEM_CHECKED;
            }

            listbox->_update(listbox);
            VK_WIDGET(object)->_draw(VK_WIDGET(object));

            return 0;
        }

        default:
            return VK_LISTBOX_KLASS->kmio(object, keystroke);
    }
}

static int
_vk_selectbox_update(vk_listbox_t *listbox)
{
    vk_selectbox_t      *selectbox;
    vk_widget_t         *widget;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 paint_width;
    int                 paint_height;
    int                 paint_colors;
    int                 highlight;
    attr_t              highlight_attr;
    int                 ind_width;
    const char          *ind;
    bool                checked;
    int                 name_width;
    int                 idx = 0;
    int                 y = 0;

    if(listbox == NULL) return -1;

    selectbox = VK_SELECTBOX(listbox);
    widget = VK_WIDGET(listbox);

    widget->_erase(widget);
    paint_width = widget->width;
    paint_height = widget->height;

    if(widget->vscroller != NULL) paint_width--;
    if(widget->hscroller != NULL) paint_height--;

    if(listbox->highlight_fg == -1) listbox->highlight_fg = widget->bg;
    if(listbox->highlight_bg == -1) listbox->highlight_bg = widget->fg;

    paint_colors = VIPER_COLORS(widget->fg, widget->bg) | widget->attrs;
    highlight = VIPER_COLORS(listbox->highlight_fg, listbox->highlight_bg);

    wbkgd(widget->canvas, ' ' | paint_colors);
    wattron(widget->canvas, paint_colors);

    if(listbox->item_count <= paint_height)
        listbox->scroll_top = 0;

    listbox->scroll_bottom = listbox->scroll_top + (paint_height - 1);

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

    _vk_selectbox_glyph(selectbox, false, &ind_width);
    name_width = paint_width - ind_width;
    if(name_width < 1) name_width = 1;

    list_for_each(pos, &listbox->item_list)
    {
        item = list_entry(pos, vk_item_t, list);

        if(idx < listbox->scroll_top)
        {
            idx++;
            continue;
        }

        if(idx > listbox->scroll_bottom) break;

        highlight_attr = A_NORMAL;

        if(item->separator_style > 0)
        {
            if(item->separator_style == VK_SEPARATOR_BLANK)
            {
                mvwprintw(widget->canvas, y, 0, "%-*c",
                    paint_width, ' ');
            }

            if(item->separator_style == VK_SEPARATOR_SINGLE)
            {
                mvwhline(widget->canvas, y, 0, ACS_HLINE, paint_width);
                highlight_attr = A_ALTCHARSET;
            }
        }
        else
        {
            checked = (item->flags & VK_ITEM_CHECKED) ? true : false;
            ind = _vk_selectbox_glyph(selectbox, checked, &ind_width);

            mvwprintw(widget->canvas, y, 0, "%s", ind);
            mvwprintw(widget->canvas, y, ind_width, "%-*.*s",
                name_width, name_width,
                item->name);
        }

        if(idx == listbox->curr_item)
        {
            mvwchgat(widget->canvas, y, 0, paint_width,
                highlight_attr, PAIR_NUMBER(highlight), NULL);
        }

        y++;
        idx++;
    }

    while(y < paint_height)
    {
        mvwprintw(widget->canvas, y, 0, "%-*c", paint_width, ' ');
        y++;
    }

    wattroff(widget->canvas, paint_colors);

    if(widget->vscroller != NULL)
    {
        if(vk_scroller_update(widget->vscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        if(vk_scroller_update(widget->hscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->hscroller));
    }

    return 0;
}

static int
_vk_selectbox_on_resize(vk_widget_t *widget)
{
    if(widget->vscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->vscroller), 1, widget->height);
        vk_widget_move(VK_WIDGET(widget->vscroller), widget->width - 1, 0);
    }

    if(widget->hscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->hscroller), widget->width, 1);
        vk_widget_move(VK_WIDGET(widget->hscroller), 0, widget->height - 1);
    }

    return VK_LISTBOX(widget)->_update(VK_LISTBOX(widget));
}

static int
_vk_selectbox_on_recreate(vk_widget_t *widget)
{
    if(widget->vscroller != NULL)
    {
        VK_WIDGET(widget->vscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        VK_WIDGET(widget->hscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->hscroller));
    }

    return VK_LISTBOX(widget)->_update(VK_LISTBOX(widget));
}

static vk_item_t*
_vk_selectbox_get_item_at(vk_selectbox_t *sb, int idx)
{
    vk_listbox_t        *listbox;
    vk_item_t           *item;
    struct list_head    *pos;
    int                 i = 0;

    listbox = VK_LISTBOX(sb);

    list_for_each(pos, &listbox->item_list)
    {
        if(i == idx)
        {
            item = list_entry(pos, vk_item_t, list);
            return item;
        }
        i++;
    }

    return NULL;
}

static const char*
_vk_selectbox_glyph(vk_selectbox_t *sb, bool checked, int *col_width)
{
    if(sb->mode == VK_SELECTBOX_CHECKBOX)
    {
        *col_width = 4;

        if(sb->style == VK_FRAME_ASCII)
            return checked ? "[x] " : "[ ] ";
        else
            return checked ? "[\xe2\x9c\x93] " : "[ ] ";
    }
    else
    {
        if(sb->style == VK_FRAME_ASCII)
        {
            *col_width = 4;
            return checked ? "(*) " : "( ) ";
        }
        else
        {
            *col_width = 2;
            return checked ? "\xe2\x97\x8f " : "\xe2\x97\x8b ";
        }
    }
}
