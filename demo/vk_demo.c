#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_listbox.h"
#include "vk_item.h"
#include "vk_menu.h"
#include "vk_frame.h"
#include "vk_scroller.h"
#include "vk_box.h"
#include "vk_label.h"
#include "vk_marquee.h"
#include "vk_screen.h"

static int
on_item_activate(vk_widget_t *widget, void *anything)
{
    (void)widget;
    (void)anything;
    return 0;
}

static int
listbox_max_item_width(vk_listbox_t *lb)
{
    struct list_head    *pos;
    vk_item_t          *item;
    int                 max_w = 0;
    int                 len;

    list_for_each(pos, &lb->item_list)
    {
        item = list_entry(pos, vk_item_t, list);
        if(item->name != NULL)
        {
            len = strlen(item->name);
            if(len > max_w) max_w = len;
        }
    }

    return max_w;
}

static void
listbox_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_listbox_t *lb = VK_LISTBOX(child);

    if(content_h) *content_h = lb->item_count;
    if(content_w) *content_w = listbox_max_item_width(lb);
    if(scroll_y) *scroll_y = lb->scroll_top;
    if(scroll_x) *scroll_x = 0;
}

static void
menu_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_listbox_t *lb = VK_LISTBOX(child);

    if(content_h) *content_h = lb->item_count;
    if(content_w) *content_w = listbox_max_item_width(lb);
    if(scroll_y) *scroll_y = lb->scroll_top;
    if(scroll_x) *scroll_x = 0;
}

static void
draw_chrome(WINDOW *screen, int max_x, int slot_w, int focus)
{
    const char  *panels[] = { "Listbox", "Menu", "Widget" };
    int         colors;

    colors = VIPER_COLORS(COLOR_WHITE, COLOR_BLACK);
    wattron(screen, colors);
    mvwprintw(screen, 1, 2, "Scroller+Listbox");
    mvwprintw(screen, 1, slot_w + 2, "Scroller+Menu");
    mvwprintw(screen, 1, (slot_w * 2) + 2, "Frame+Widget");
    wattroff(screen, colors);

    colors = VIPER_COLORS(COLOR_CYAN, COLOR_BLACK);
    wattron(screen, colors | A_BOLD);
    mvwprintw(screen, 1, max_x - 18, "Focus: %-10s", panels[focus]);
    wattroff(screen, colors | A_BOLD);
}

static vk_listbox_t*
build_listbox(int width, int height)
{
    vk_listbox_t    *listbox;
    int             i;

    const char *items[] = {
        "Alpha",    "Beta",     "Gamma",    "Delta",    "Epsilon",
        "Zeta",     "Eta",      "Theta",    "Iota",     "Kappa",
        "Lambda",   "Mu",       "Nu",       "Xi",       "Omicron",
        "Pi",       "Rho",      "Sigma",    "Tau",      "Upsilon",
        "Phi",      "Chi",      "Psi",      "Omega",
        "Hydrogen",     "Helium",       "Lithium",
        "Beryllium",    "Boron",        "Carbon",
        "Nitrogen",     "Oxygen",       "Fluorine",
        "Neon",         "Sodium",       "Magnesium",
        "Aluminum",     "Silicon",      "Phosphorus",
        "Sulfur",       "Chlorine",     "Argon",
        "Potassium",    "Calcium",      "Scandium",
        "Titanium",     "Vanadium",     "Chromium",
        "Manganese",    "Iron",         "Cobalt",
        "Nickel",       "Copper",       "Zinc",
    };
    int item_count = sizeof(items) / sizeof(items[0]);

    listbox = vk_listbox_create(width, height);
    if(listbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(listbox), COLOR_WHITE, COLOR_BLACK);
    vk_listbox_set_highlight(listbox, COLOR_BLACK, COLOR_CYAN);
    vk_listbox_set_wrap(listbox, TRUE);

    for(i = 0; i < item_count; i++)
        vk_listbox_add_item(listbox, (char *)items[i],
            on_item_activate, (void *)items[i]);

    return listbox;
}

static vk_menu_t*
build_menu(int width, int height)
{
    vk_menu_t   *menu;

    menu = vk_menu_create(width, height);
    if(menu == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(menu), COLOR_WHITE, COLOR_BLACK);
    vk_listbox_set_highlight(VK_LISTBOX(menu), COLOR_BLACK, COLOR_GREEN);
    vk_listbox_set_wrap(VK_LISTBOX(menu), TRUE);
    vk_menu_set_frame(menu, VK_FRAME_NONE);

    vk_listbox_add_item(VK_LISTBOX(menu), "New",
        on_item_activate, "New");
    vk_listbox_add_item(VK_LISTBOX(menu), "Open",
        on_item_activate, "Open");
    vk_listbox_add_item(VK_LISTBOX(menu), "Open Recent",
        on_item_activate, "Open Recent");
    vk_listbox_add_item(VK_LISTBOX(menu), "Save",
        on_item_activate, "Save");
    vk_listbox_add_item(VK_LISTBOX(menu), "Save As",
        on_item_activate, "Save As");
    vk_listbox_add_item(VK_LISTBOX(menu), "Export",
        on_item_activate, "Export");
    vk_menu_add_separator(menu, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(VK_LISTBOX(menu), "Undo",
        on_item_activate, "Undo");
    vk_listbox_add_item(VK_LISTBOX(menu), "Redo",
        on_item_activate, "Redo");
    vk_menu_add_separator(menu, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(VK_LISTBOX(menu), "Cut",
        on_item_activate, "Cut");
    vk_listbox_add_item(VK_LISTBOX(menu), "Copy",
        on_item_activate, "Copy");
    vk_listbox_add_item(VK_LISTBOX(menu), "Paste",
        on_item_activate, "Paste");
    vk_listbox_add_item(VK_LISTBOX(menu), "Delete",
        on_item_activate, "Delete");
    vk_menu_add_separator(menu, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(VK_LISTBOX(menu), "Select All",
        on_item_activate, "Select All");
    vk_listbox_add_item(VK_LISTBOX(menu), "Select None",
        on_item_activate, "Select None");
    vk_menu_add_separator(menu, VK_SEPARATOR_BLANK);
    vk_listbox_add_item(VK_LISTBOX(menu), "Find",
        on_item_activate, "Find");
    vk_listbox_add_item(VK_LISTBOX(menu), "Find Next",
        on_item_activate, "Find Next");
    vk_listbox_add_item(VK_LISTBOX(menu), "Find Previous",
        on_item_activate, "Find Previous");
    vk_listbox_add_item(VK_LISTBOX(menu), "Replace",
        on_item_activate, "Replace");
    vk_listbox_add_item(VK_LISTBOX(menu), "Go To Line",
        on_item_activate, "Go To Line");
    vk_menu_add_separator(menu, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(VK_LISTBOX(menu), "Word Wrap",
        on_item_activate, "Word Wrap");
    vk_listbox_add_item(VK_LISTBOX(menu), "Show Whitespace",
        on_item_activate, "Show Whitespace");
    vk_listbox_add_item(VK_LISTBOX(menu), "Line Numbers",
        on_item_activate, "Line Numbers");
    vk_menu_add_separator(menu, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(VK_LISTBOX(menu), "Zoom In",
        on_item_activate, "Zoom In");
    vk_listbox_add_item(VK_LISTBOX(menu), "Zoom Out",
        on_item_activate, "Zoom Out");
    vk_listbox_add_item(VK_LISTBOX(menu), "Reset Zoom",
        on_item_activate, "Reset Zoom");
    vk_menu_add_separator(menu, VK_SEPARATOR_BLANK);
    vk_listbox_add_item(VK_LISTBOX(menu), "Preferences",
        on_item_activate, "Preferences");
    vk_listbox_add_item(VK_LISTBOX(menu), "About",
        on_item_activate, "About");

    return menu;
}

static int
plain_widget_on_recreate(vk_widget_t *widget)
{
    int fill_attr;

    fill_attr = VIPER_COLORS(COLOR_WHITE, COLOR_BLUE);
    wbkgd(widget->canvas, ' ' | fill_attr);

    mvwprintw(widget->canvas, 1, 2, "Plain Widget");
    mvwprintw(widget->canvas, 3, 2, "A basic vk_widget_t");
    mvwprintw(widget->canvas, 4, 2, "with colored fill");
    mvwprintw(widget->canvas, 5, 2, "inside an ASCII");
    mvwprintw(widget->canvas, 6, 2, "bordered frame.");
    mvwprintw(widget->canvas, 8, 2, "This widget has");
    mvwprintw(widget->canvas, 9, 2, "no kmio handler,");
    mvwprintw(widget->canvas, 10, 2, "so arrow keys do");
    mvwprintw(widget->canvas, 11, 2, "nothing here.");

    return 0;
}

static vk_widget_t*
build_plain_widget(int width, int height)
{
    vk_widget_t *widget;

    widget = vk_widget_create(width, height);
    if(widget == NULL) return NULL;

    vk_widget_set_colors(widget, COLOR_WHITE, COLOR_BLUE);
    widget->_on_recreate = plain_widget_on_recreate;
    plain_widget_on_recreate(widget);

    return widget;
}

int main(void)
{
    vk_screen_t     *vk_screen;
    WINDOW          *screen;
    vk_box_t        *box;
    vk_scroller_t   *scroller1;
    vk_scroller_t   *scroller2;
    vk_frame_t      *frame3;
    vk_listbox_t    *listbox;
    vk_menu_t       *menu;
    vk_widget_t     *plain;
    vk_marquee_t    *marquee;
    int             max_y, max_x;
    int             box_h;
    int             slot_w, inner_w, inner_h;
    int32_t         key;

    setlocale(LC_ALL, "");

    vk_screen = vk_screen_create();
    if(vk_screen == NULL)
    {
        fprintf(stderr, "vk_screen_create failed\n");
        return 1;
    }

    screen = vk_screen_get_window(vk_screen);

    getmaxyx(screen, max_y, max_x);

    if(max_x < 60 || max_y < 14)
    {
        vk_screen_destroy(vk_screen);
        fprintf(stderr,
            "Terminal too small (need 60x14, have %dx%d)\n",
            max_x, max_y);
        return 1;
    }

    box_h = max_y - 2;
    slot_w = max_x / 3;
    inner_w = slot_w - 2;
    inner_h = box_h - 2;

    marquee = vk_marquee_create(max_x);
    vk_widget_set_colors(VK_WIDGET(marquee), COLOR_WHITE, COLOR_BLUE);
    vk_widget_set_attrs(VK_WIDGET(marquee), A_BOLD);
    vk_marquee_set_text(marquee,
        "VK Klass Reference Demo"
        "  |  Built: " __DATE__ " " __TIME__
        "  |  TAB:focus  \xE2\x86\x91\xE2\x86\x93:nav  Enter:sel"
        "  S-\xE2\x86\x90\xE2\x86\x92:width  +/-:height  q:quit");
    vk_marquee_set_direction(marquee, VK_SCROLL_LOOP);
    vk_marquee_set_speed(marquee, 2);
    vk_screen_attach_widget(vk_screen, 0, VK_WIDGET(marquee));
    vk_widget_move(VK_WIDGET(marquee), 0, 0);

    box = vk_box_create(max_x, box_h, VK_BOX_HORIZONTAL, 3);
    vk_screen_attach_widget(vk_screen, 0, VK_WIDGET(box));
    vk_widget_move(VK_WIDGET(box), 0, 2);

    scroller1 = vk_scroller_create(slot_w, box_h);
    vk_scroller_set_border_style(scroller1, VK_FRAME_SINGLE);
    vk_scroller_set_scrollbar(scroller1, VK_SCROLLBAR_BOTH);
    vk_scroller_set_scroll_info(scroller1, listbox_scroll_info);

    listbox = build_listbox(inner_w, inner_h);
    vk_scroller_set_child(scroller1, VK_WIDGET(listbox));

    scroller2 = vk_scroller_create(slot_w, box_h);
    vk_scroller_set_border_style(scroller2, VK_FRAME_DOUBLE);
    vk_scroller_set_scrollbar(scroller2, VK_SCROLLBAR_BOTH);
    vk_scroller_set_scroll_info(scroller2, menu_scroll_info);

    menu = build_menu(inner_w, inner_h);
    vk_scroller_set_child(scroller2, VK_WIDGET(menu));

    frame3 = vk_frame_create(slot_w, box_h);
    vk_frame_set_border_style(frame3, VK_FRAME_ASCII);

    plain = build_plain_widget(inner_w, inner_h);
    vk_frame_set_child(frame3, plain);

    vk_box_set_widget(box, 0, VK_WIDGET(scroller1));
    vk_box_set_widget(box, 1, VK_WIDGET(scroller2));
    vk_box_set_widget(box, 2, VK_WIDGET(frame3));

    vk_listbox_update(listbox);
    vk_menu_update(menu);

    vk_scroller_set_border_colors(scroller1, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_update(scroller1);

    vk_scroller_set_border_colors(scroller2, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_update(scroller2);

    vk_frame_set_border_colors(frame3, COLOR_WHITE, COLOR_BLACK);
    vk_frame_update(frame3);

    vk_box_update(box);
    vk_widget_draw(VK_WIDGET(box));

    vk_marquee_run(marquee);
    vk_widget_draw(VK_WIDGET(marquee));

    draw_chrome(screen, max_x, slot_w, box->focused_slot);
    vk_screen_refresh(vk_screen);

    wtimeout(stdscr, 100);

    while((key = wgetch(stdscr)) != 'q')
    {
        if(key == KEY_RESIZE)
        {
            vk_screen_resize(vk_screen);
            screen = vk_screen_get_window(vk_screen);
            getmaxyx(screen, max_y, max_x);

            box_h = max_y - 2;
            slot_w = max_x / 3;

            vk_widget_resize(VK_WIDGET(marquee), max_x, 1);
            vk_widget_resize(VK_WIDGET(box), max_x, box_h);
        }
        else if(key == 't')
        {
            char    pty_path[128] = "";
            int     pos = 0;
            int     colors;
            int     ch;

            colors = VIPER_COLORS(COLOR_WHITE, COLOR_BLUE) | A_BOLD;
            werase(screen);
            wattron(screen, colors);
            mvwhline(screen, 0, 0, ' ', max_x);
            mvwprintw(screen, 0, 0, "Teleport to: ");
            wattroff(screen, colors);
            overwrite(screen, stdscr);
            wrefresh(stdscr);

            wtimeout(stdscr, -1);
            curs_set(1);

            while((ch = wgetch(stdscr)) != '\n')
            {
                if(ch == 27)
                {
                    pos = 0;
                    break;
                }

                if((ch == KEY_BACKSPACE || ch == 127) && pos > 0)
                {
                    pos--;
                    pty_path[pos] = '\0';
                }
                else if(ch >= 32 && ch < 127 && pos < (int)sizeof(pty_path) - 1)
                {
                    pty_path[pos++] = ch;
                    pty_path[pos] = '\0';
                }

                wattron(screen, colors);
                mvwhline(screen, 0, 0, ' ', max_x);
                mvwprintw(screen, 0, 0, "Teleport to: %s", pty_path);
                wattroff(screen, colors);
                overwrite(screen, stdscr);
                wrefresh(stdscr);
            }

            curs_set(0);
            wtimeout(stdscr, 100);

            if(pos > 0)
            {
                vk_screen_teleport(vk_screen, pty_path);
                screen = vk_screen_get_window(vk_screen);
                getmaxyx(screen, max_y, max_x);

                box_h = max_y - 2;
                slot_w = max_x / 3;

                vk_widget_resize(VK_WIDGET(marquee), max_x, 1);
                vk_widget_resize(VK_WIDGET(box), max_x, box_h);

                wtimeout(stdscr, 100);
            }
        }
        else if(key == KEY_SRIGHT)
        {
            vk_widget_resize(VK_WIDGET(box),
                VK_WIDGET(box)->width + 1, WSIZE_UNCHANGED);
        }
        else if(key == KEY_SLEFT)
        {
            if(VK_WIDGET(box)->width > 12)
                vk_widget_resize(VK_WIDGET(box),
                    VK_WIDGET(box)->width - 1, WSIZE_UNCHANGED);
        }
        else if(key == '+')
        {
            vk_widget_resize(VK_WIDGET(box),
                WSIZE_UNCHANGED, VK_WIDGET(box)->height + 1);
        }
        else if(key == '-')
        {
            if(VK_WIDGET(box)->height > 6)
                vk_widget_resize(VK_WIDGET(box),
                    WSIZE_UNCHANGED, VK_WIDGET(box)->height - 1);
        }
        else if(key != ERR)
        {
            vk_object_push_keystroke(VK_OBJECT(box), key);
        }

        werase(screen);

        short s1_fg = (box->focused_slot == 0) ? COLOR_CYAN : COLOR_WHITE;
        short s2_fg = (box->focused_slot == 1) ? COLOR_GREEN : COLOR_WHITE;
        short f3_fg = (box->focused_slot == 2) ? COLOR_YELLOW : COLOR_WHITE;

        vk_scroller_set_border_colors(scroller1, s1_fg, COLOR_BLACK);
        vk_scroller_update(scroller1);

        vk_scroller_set_border_colors(scroller2, s2_fg, COLOR_BLACK);
        vk_scroller_update(scroller2);

        vk_frame_set_border_colors(frame3, f3_fg, COLOR_BLACK);
        vk_frame_update(frame3);

        vk_box_update(box);
        vk_widget_draw(VK_WIDGET(box));

        vk_marquee_run(marquee);
        vk_widget_draw(VK_WIDGET(marquee));

        draw_chrome(screen, max_x, slot_w, box->focused_slot);
        vk_screen_refresh(vk_screen);
    }

    vk_marquee_destroy(marquee);
    vk_box_destroy(box);

    vk_scroller_destroy(scroller1);
    vk_scroller_destroy(scroller2);
    vk_frame_destroy(frame3);

    vk_listbox_destroy(listbox);
    vk_menu_destroy(menu);
    vk_widget_destroy(plain);

    vk_screen_destroy(vk_screen);

    return 0;
}
