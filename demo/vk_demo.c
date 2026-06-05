#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <langinfo.h>

#include "vdk.h"

#define NUM_SURFACES    5

static const char *surface_names[] =
{
    "Widgets",
    "Languages",
    "Selectbox",
    "Dotfield",
    "Shade",
};

static int
on_item_activate(vk_widget_t *widget, void *anything)
{
    (void)widget;
    (void)anything;
    return 0;
}

static int
on_teleport(vk_object_t *object, int event, void *data)
{
    (void)object;
    (void)event;
    (void)data;

    vdk_color_init();

    return 0;
}

struct focus_ctx
{
    vk_window_t     *window;
    vk_scroller_t   *vscroller;
    vk_scroller_t   *hscroller;
    short           active_fg;
};

static int
on_window_focus(vk_object_t *object, int event, void *data)
{
    struct focus_ctx *ctx = data;

    (void)object;
    (void)event;

    vk_window_set_border_colors(ctx->window, ctx->active_fg, COLOR_BLACK);

    if(ctx->vscroller)
        vk_scroller_set_border_colors(ctx->vscroller,
            ctx->active_fg, COLOR_BLACK);
    if(ctx->hscroller)
        vk_scroller_set_border_colors(ctx->hscroller,
            ctx->active_fg, COLOR_BLACK);

    return 0;
}

static int
on_window_unfocus(vk_object_t *object, int event, void *data)
{
    struct focus_ctx *ctx = data;

    (void)object;
    (void)event;

    vk_window_set_border_colors(ctx->window, COLOR_WHITE, COLOR_BLACK);

    if(ctx->vscroller)
        vk_scroller_set_border_colors(ctx->vscroller,
            COLOR_WHITE, COLOR_BLACK);
    if(ctx->hscroller)
        vk_scroller_set_border_colors(ctx->hscroller,
            COLOR_WHITE, COLOR_BLACK);

    return 0;
}

static int
on_listbox_select(vk_object_t *object, int event, void *data)
{
    vk_listbox_t    *listbox = VK_LISTBOX(object);
    vk_window_t     *window = data;
    char            item[32];
    char            title[48];

    (void)event;

    if(vk_listbox_get_item(listbox, vk_listbox_get_curr(listbox),
        item, sizeof(item)) == 0)
    {
        snprintf(title, sizeof(title), " Listbox: %s ", item);
        vk_window_set_title(window, title);
    }

    return 0;
}

static int
on_checkbox_change(vk_object_t *object, int event, void *data)
{
    vk_selectbox_t  *sb = VK_SELECTBOX(object);
    vk_window_t     *window = data;
    char            title[48];
    int             count;
    int             checked = 0;
    int             i;

    (void)event;

    count = vk_selectbox_get_item_count(sb);

    for(i = 0; i < count; i++)
        if(vk_selectbox_item_is_checked(sb, i))
            checked++;

    snprintf(title, sizeof(title), " Checkbox [%d/%d] ", checked, count);
    vk_window_set_title(window, title);

    return 0;
}

static int
on_radio_change(vk_object_t *object, int event, void *data)
{
    vk_selectbox_t  *sb = VK_SELECTBOX(object);
    vk_window_t     *window = data;
    char            item[32];
    char            title[48];
    int             count;
    int             i;

    (void)event;

    count = vk_selectbox_get_item_count(sb);

    for(i = 0; i < count; i++)
    {
        if(vk_selectbox_item_is_checked(sb, i))
        {
            vk_selectbox_get_item(sb, i, item, sizeof(item));
            snprintf(title, sizeof(title), " Theme: %s ", item);
            vk_window_set_title(window, title);
            return 0;
        }
    }

    return 0;
}

static int
on_textbox_scroll(vk_object_t *object, int event, void *data)
{
    vk_textbox_t    *tb = VK_TEXTBOX(object);
    vk_window_t     *window = data;
    char            title[48];

    (void)event;

    snprintf(title, sizeof(title), " Textbox [line %d/%d] ",
        vk_textbox_get_scroll_pos(tb) + 1,
        vk_textbox_get_line_count(tb));
    vk_window_set_title(window, title);

    return 0;
}

static void
listbox_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_listbox_t *lb = VK_LISTBOX(child);
    int metrics_w = 0;

    vk_listbox_get_metrics(lb, &metrics_w, NULL);

    if(content_h) *content_h = vk_listbox_get_item_count(lb);
    if(content_w) *content_w = metrics_w;
    if(scroll_y) *scroll_y = vk_listbox_get_scroll_pos(lb);
    if(scroll_x) *scroll_x = 0;
}

static void
textbox_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_textbox_t *tb = VK_TEXTBOX(child);

    if(content_h) *content_h = vk_textbox_get_line_count(tb);
    if(content_w) *content_w = 0;
    if(scroll_y) *scroll_y = vk_textbox_get_scroll_pos(tb);
    if(scroll_x) *scroll_x = 0;
}

static void
wallpaper_callback(vk_screen_t *screen, int surface_id, WINDOW *canvas)
{
    int     max_y, max_x;
    int     x, y;
    int     colors;

    (void)screen;

    getmaxyx(canvas, max_y, max_x);

    switch(surface_id)
    {
        case 0:
        {
            colors = VDK_COLORS(COLOR_GREEN, COLOR_BLACK);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, ((x + y) % 4 == 0) ? '+' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 1:
        {
            colors = VDK_COLORS(COLOR_YELLOW, COLOR_BLACK);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, (y % 2 == 0) ? '-' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 2:
        {
            colors = VDK_COLORS(COLOR_RED, COLOR_BLACK);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, ((x % 6 == 0) || (y % 3 == 0)) ? '.' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 3:
        {
            colors = VDK_COLORS(COLOR_BLUE, COLOR_BLACK);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
            {
                for(x = 0; x < max_x; x++)
                {
                    if((x + y) % 2 == 0)
                        mvwaddch(canvas, y, x, '.');
                    else
                        mvwaddch(canvas, y, x, ' ');
                }
            }

            wattroff(canvas, colors);
            break;
        }

        case 4:
        {
            cchar_t shade;
            wchar_t wch[2] = {0x2591, 0};
            short   pair = vdk_color_pair(COLOR_CYAN, COLOR_BLACK);

            setcchar(&shade, wch, A_NORMAL, pair, NULL);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwadd_wch(canvas, y, x, &shade);

            break;
        }
    }
}

static vk_textbox_t*
build_textbox(int width, int height)
{
    vk_textbox_t    *textbox;
    const char      *text =
        "VK Textbox Widget\n"
        "\n"
        "This is a multi-line read-only text display widget "
        "that supports word wrapping and vertical scrolling "
        "via an attached scroller.\n"
        "\n"
        "The scroller is attached directly to the textbox "
        "(not the parent window), demonstrating the "
        "\"scrollbar inside\" pattern where the scrollbar "
        "lives in the content area rather than on the "
        "host's border.\n"
        "\n"
        "Keyboard:\n"
        "  Up/Down ..... scroll one line\n"
        "  PgUp/PgDn ... scroll one page\n"
        "  Home/End .... jump to top/bottom\n"
        "\n"
        "The word wrap engine splits text at word boundaries "
        "when it reaches the paint width. Hard newlines in "
        "the source text are always honored. When the widget "
        "is resized, the text is reflowed automatically to "
        "fit the new width.\n"
        "\n"
        "Lorem ipsum dolor sit amet, consectetur adipiscing "
        "elit. Sed do eiusmod tempor incididunt ut labore et "
        "dolore magna aliqua. Ut enim ad minim veniam, quis "
        "nostrud exercitation ullamco laboris nisi ut aliquip "
        "ex ea commodo consequat.\n"
        "\n"
        "Duis aute irure dolor in reprehenderit in voluptate "
        "velit esse cillum dolore eu fugiat nulla pariatur. "
        "Excepteur sint occaecat cupidatat non proident, sunt "
        "in culpa qui officia deserunt mollit anim id est "
        "laborum.";

    textbox = vk_textbox_create(width, height);
    if(textbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(textbox), COLOR_GREEN, COLOR_BLACK);
    vk_widget_set_attrs(VK_WIDGET(textbox), A_BOLD);
    vk_textbox_set_word_wrap(textbox, TRUE);
    vk_textbox_set_text(textbox, text);

    return textbox;
}

static void
deck_draw_chrome(vk_window_t *window, WINDOW *canvas)
{
    int         max_y, max_x;
    int         sx;
    cchar_t     cc;
    wchar_t     wch_close[2] = {0x2715, 0};
    wchar_t     wch_square[2] = {0x25A1, 0};
    wchar_t     wch_hline[2] = {0x2500, 0};
    wchar_t     wch_block[2] = {' ', 0};
    wchar_t     wch_rtee[2];
    wchar_t     wch_ltee[2];
    short       fg, bg, pair;
    int         style;

    getmaxyx(canvas, max_y, max_x);
    (void)max_y;

    fg = vk_window_get_border_fg(window);
    bg = vk_window_get_border_bg(window);
    if(fg == -1 || bg == -1)
    {
        short wfg, wbg;
        vk_widget_get_colors(VK_WIDGET(window), &wfg, &wbg);
        if(fg == -1) fg = wfg;
        if(bg == -1) bg = wbg;
    }
    pair = vdk_color_pair(fg, bg);

    style = vk_window_get_border_style(window);
    if(style & VK_FRAME_DOUBLE)
    {
        wch_rtee[0] = 0x2563;  wch_rtee[1] = 0;
        wch_ltee[0] = 0x2560;  wch_ltee[1] = 0;
    }
    else
    {
        wch_rtee[0] = 0x2524;  wch_rtee[1] = 0;
        wch_ltee[0] = 0x251C;  wch_ltee[1] = 0;
    }

    // ┤ [block ─ block □ block ✕ block] ├
    setcchar(&cc, wch_rtee, A_NORMAL, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 10, &cc);

    setcchar(&cc, wch_block, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 9, &cc);

    setcchar(&cc, wch_hline, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 8, &cc);

    setcchar(&cc, wch_block, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 7, &cc);

    setcchar(&cc, wch_square, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 6, &cc);

    setcchar(&cc, wch_block, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 5, &cc);

    setcchar(&cc, wch_close, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 4, &cc);

    setcchar(&cc, wch_block, A_REVERSE, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 3, &cc);

    setcchar(&cc, wch_ltee, A_NORMAL, pair, NULL);
    mvwadd_wch(canvas, 0, max_x - 2, &cc);

    setcchar(&cc, wch_hline, A_NORMAL, pair, NULL);
    for(sx = 1; sx < max_x - 1; sx++)
        mvwadd_wch(canvas, 2, sx, &cc);
}

static void
deck_notes_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "Meeting Notes");
    mvwprintw(canvas, 3, 2, "- Review architecture");
    mvwprintw(canvas, 4, 2, "- Update timeline");
    mvwprintw(canvas, 5, 2, "- Assign tasks");
    mvwprintw(canvas, 6, 2, "- Plan next sprint");
    mvwprintw(canvas, 7, 2, "- Demo to stakeholders");
}

static void
deck_tasks_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "Task List");
    mvwprintw(canvas, 3, 2, "[x] Implement deck widget");
    mvwprintw(canvas, 4, 2, "[x] Z-order management");
    mvwprintw(canvas, 5, 2, "[ ] Hit testing");
    mvwprintw(canvas, 6, 2, "[ ] Mouse drag support");
    mvwprintw(canvas, 7, 2, "[ ] Window shadows");
}

static void
deck_help_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "VK Deck Widget");
    mvwprintw(canvas, 3, 2, "TAB cycles through the");
    mvwprintw(canvas, 4, 2, "window stack. The top");
    mvwprintw(canvas, 5, 2, "window receives focus");
    mvwprintw(canvas, 6, 2, "and keyboard input.");
}

static void
deck_log_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "Activity Log");
    mvwprintw(canvas, 3, 2, "09:14  Build succeeded");
    mvwprintw(canvas, 4, 2, "09:15  Tests passed (47/47)");
    mvwprintw(canvas, 5, 2, "09:16  Deploy to staging");
    mvwprintw(canvas, 6, 2, "09:22  Health check OK");
    mvwprintw(canvas, 7, 2, "09:30  Promote to prod");
}

static void
deck_transport_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "Media Player");
    mvwprintw(canvas, 3, 2, "Now Playing:");
    mvwprintw(canvas, 4, 2, "Track 3 - Interlude");
}

static void
deck_files_decorate(vk_window_t *window, WINDOW *canvas, void *data)
{
    (void)data;

    deck_draw_chrome(window, canvas);

    mvwprintw(canvas, 1, 2, "/ edit path  BS parent");
}

static int
listbox_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_listbox_t *listbox = VK_LISTBOX(object);

    switch(keystroke)
    {
        case KEY_UP:
            vk_listbox_set_prev(listbox);
            break;

        case KEY_DOWN:
            vk_listbox_set_next(listbox);
            break;

        case KEY_CRLF:
            vk_listbox_exec_curr(listbox);
            break;

        default:
            return 0;
    }

    vk_listbox_update(listbox);
    vk_widget_draw(VK_WIDGET(object));

    return 0;
}

static int
textbox_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_textbox_t *textbox = VK_TEXTBOX(object);

    switch(keystroke)
    {
        case KEY_UP:        vk_textbox_scroll_up(textbox);      break;
        case KEY_DOWN:      vk_textbox_scroll_down(textbox);    break;
        case KEY_PPAGE:     vk_textbox_scroll_pgup(textbox);    break;
        case KEY_NPAGE:     vk_textbox_scroll_pgdn(textbox);    break;
        case KEY_HOME:      vk_textbox_scroll_home(textbox);    break;
        case KEY_END:       vk_textbox_scroll_end(textbox);     break;
        default:            return 0;
    }

    vk_textbox_update(textbox);

    return 0;
}

static int
selectbox_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_selectbox_t *selectbox = VK_SELECTBOX(object);

    switch(keystroke)
    {
        case KEY_CRLF:
        case ' ':
            vk_selectbox_toggle_item(selectbox,
                vk_selectbox_get_curr(selectbox));
            break;

        case KEY_UP:
            vk_selectbox_set_prev(selectbox);
            break;

        case KEY_DOWN:
            vk_selectbox_set_next(selectbox);
            break;

        default:
            return 0;
    }

    vk_selectbox_update(selectbox);
    vk_widget_draw(VK_WIDGET(object));

    return 0;
}

static int
box_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_box_t    *box = VK_BOX(object);
    int         slot;
    int         count;
    vk_widget_t *child;

    if(keystroke == KEY_TAB)
    {
        slot = vk_box_get_subfocus(box);
        count = vk_box_get_slot_count(box);
        vk_box_set_subfocus(box, (slot + 1) % count);
        return 0;
    }

    child = vk_box_get_widget(box, vk_box_get_subfocus(box));
    if(child != NULL)
        return vk_object_push_keystroke(VK_OBJECT(child), keystroke);

    return 0;
}

static int
deck_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_deck_t   *deck = VK_DECK(object);
    vk_widget_t *top;

    if(keystroke == KEY_TAB)
    {
        vk_deck_cycle(deck, VK_VECTOR_LEFT);
        return 0;
    }

    top = vk_deck_get_top(deck);
    if(top != NULL)
        return vk_object_push_keystroke(VK_OBJECT(top), keystroke);

    return 0;
}

static int
transport_kmio(vk_object_t *object, int32_t keystroke)
{
    static int      active_slot = -1;
    vk_box_t        *box = VK_BOX(object);
    int             slot;
    int             count;
    vk_widget_t     *btn;

    slot = vk_box_get_subfocus(box);
    count = vk_box_get_slot_count(box);

    if(keystroke == KEY_LEFT)
    {
        vk_box_set_subfocus(box, (slot > 0) ? slot - 1 : count - 1);
        return 0;
    }

    if(keystroke == KEY_RIGHT)
    {
        vk_box_set_subfocus(box, (slot + 1) % count);
        return 0;
    }

    if(keystroke == ' ')
    {
        btn = vk_box_get_widget(box, active_slot);
        if(btn != NULL)
            vk_button_release(VK_BUTTON(btn));

        btn = vk_box_get_widget(box, slot);
        if(btn != NULL)
        {
            vk_button_press(VK_BUTTON(btn));
            active_slot = slot;
        }

        return 0;
    }

    return 0;
}

static void
set_marquee_text(vk_marquee_t *marquee, int surface)
{
    char text[512];

    snprintf(text, sizeof(text),
        "[Surface %d: %s]"
        "  VK Klass Reference Demo"
        "  |  Built: " __DATE__ " " __TIME__
        "  |  TAB:focus  \xE2\x86\x91\xE2\x86\x93:nav  Enter:sel"
        "  d:surface  f:freeze  h:marquee  t:teleport  w:deck  q:quit",
        surface + 1, surface_names[surface]);

    vk_marquee_set_text(marquee, text);
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

    vk_widget_set_colors(VK_WIDGET(listbox), COLOR_CYAN, COLOR_BLUE);
    vk_listbox_set_highlight(listbox, COLOR_WHITE, COLOR_MAGENTA);
    vk_listbox_set_wrap(listbox, TRUE);

    for(i = 0; i < item_count; i++)
        vk_listbox_add_item(listbox, (char *)items[i],
            on_item_activate, (void *)items[i]);

    return listbox;
}

static vk_listbox_t*
build_menu(int width, int height)
{
    vk_listbox_t    *listbox;

    listbox = vk_listbox_create(width, height);
    if(listbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(listbox), COLOR_YELLOW, COLOR_BLACK);
    vk_listbox_set_highlight(listbox, COLOR_BLACK, COLOR_GREEN);
    vk_listbox_set_wrap(listbox, TRUE);

    vk_listbox_add_item(listbox, "New", on_item_activate, "New");
    vk_listbox_add_item(listbox, "Open", on_item_activate, "Open");
    vk_listbox_add_item(listbox, "Open Recent",
        on_item_activate, "Open Recent");
    vk_listbox_add_item(listbox, "Save", on_item_activate, "Save");
    vk_listbox_add_item(listbox, "Save As", on_item_activate, "Save As");
    vk_listbox_add_item(listbox, "Export", on_item_activate, "Export");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(listbox, "Undo", on_item_activate, "Undo");
    vk_listbox_add_item(listbox, "Redo", on_item_activate, "Redo");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(listbox, "Cut", on_item_activate, "Cut");
    vk_listbox_add_item(listbox, "Copy", on_item_activate, "Copy");
    vk_listbox_add_item(listbox, "Paste", on_item_activate, "Paste");
    vk_listbox_add_item(listbox, "Delete", on_item_activate, "Delete");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(listbox, "Select All",
        on_item_activate, "Select All");
    vk_listbox_add_item(listbox, "Select None",
        on_item_activate, "Select None");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_BLANK);
    vk_listbox_add_item(listbox, "Find", on_item_activate, "Find");
    vk_listbox_add_item(listbox, "Find Next",
        on_item_activate, "Find Next");
    vk_listbox_add_item(listbox, "Find Previous",
        on_item_activate, "Find Previous");
    vk_listbox_add_item(listbox, "Replace", on_item_activate, "Replace");
    vk_listbox_add_item(listbox, "Go To Line",
        on_item_activate, "Go To Line");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(listbox, "Word Wrap",
        on_item_activate, "Word Wrap");
    vk_listbox_add_item(listbox, "Show Whitespace",
        on_item_activate, "Show Whitespace");
    vk_listbox_add_item(listbox, "Line Numbers",
        on_item_activate, "Line Numbers");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_SINGLE);
    vk_listbox_add_item(listbox, "Zoom In", on_item_activate, "Zoom In");
    vk_listbox_add_item(listbox, "Zoom Out",
        on_item_activate, "Zoom Out");
    vk_listbox_add_item(listbox, "Reset Zoom",
        on_item_activate, "Reset Zoom");
    vk_listbox_add_separator(listbox, VK_SEPARATOR_BLANK);
    vk_listbox_add_item(listbox, "Preferences",
        on_item_activate, "Preferences");
    vk_listbox_add_item(listbox, "About", on_item_activate, "About");

    return listbox;
}

static vk_listbox_t*
build_lang_listbox(int width, int height)
{
    vk_listbox_t    *listbox;
    int             i;

    const char *items[] = {
        "C",            "Python",       "JavaScript",
        "Java",         "Rust",         "Go",
        "TypeScript",   "C++",          "Swift",
        "Kotlin",       "Ruby",         "PHP",
        "Scala",        "Haskell",      "Lua",
        "Perl",         "R",            "Julia",
        "Dart",         "Elixir",       "Clojure",
        "Erlang",       "F#",           "OCaml",
        "Zig",          "Nim",          "Ada",
        "Fortran",      "COBOL",        "Pascal",
        "Scheme",       "Prolog",       "Smalltalk",
        "Tcl",          "VHDL",         "Verilog",
    };
    int item_count = sizeof(items) / sizeof(items[0]);

    listbox = vk_listbox_create(width, height);
    if(listbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(listbox), COLOR_WHITE, COLOR_BLUE);
    vk_listbox_set_highlight(listbox, COLOR_BLUE, COLOR_WHITE);
    vk_listbox_set_wrap(listbox, TRUE);

    for(i = 0; i < item_count; i++)
        vk_listbox_add_item(listbox, (char *)items[i],
            on_item_activate, (void *)items[i]);

    return listbox;
}

static int
about_on_recreate(vk_object_t *object, int event, void *data)
{
    vk_widget_t *widget = VK_WIDGET(object);
    WINDOW *canvas = vk_widget_get_canvas(widget);

    (void)event;
    (void)data;

    vk_widget_fill(widget, ' ' | VDK_COLORS(COLOR_WHITE, COLOR_BLUE));

    wattron(canvas, VDK_COLORS(COLOR_YELLOW, COLOR_BLUE) | A_BOLD);
    mvwprintw(canvas, 1, 2, "VK Widget Toolkit");
    wattroff(canvas, VDK_COLORS(COLOR_YELLOW, COLOR_BLUE) | A_BOLD);

    wattron(canvas, VDK_COLORS(COLOR_WHITE, COLOR_BLUE));
    mvwprintw(canvas, 3,  2, "Keyboard:");
    mvwprintw(canvas, 4,  4, "TAB ......... cycle focus");
    mvwprintw(canvas, 5,  4, "Up/Down ..... navigate items");
    mvwprintw(canvas, 6,  4, "Enter ....... select item");
    mvwprintw(canvas, 7,  4, "d ........... switch surface");
    mvwprintw(canvas, 8,  4, "f ........... freeze marquee");
    mvwprintw(canvas, 9,  4, "h ........... toggle marquee");
    mvwprintw(canvas, 10, 4, "t ........... teleport to PTY");
    mvwprintw(canvas, 11, 4, "w ........... toggle deck overlay");
    mvwprintw(canvas, 12, 4, "q ........... quit");

    mvwprintw(canvas, 14, 2, "Features:");
    mvwprintw(canvas, 15, 4, "- Virtual surfaces");
    mvwprintw(canvas, 16, 4, "- Terminal migration (teleport)");
    mvwprintw(canvas, 17, 4, "- Scrollable containers");
    mvwprintw(canvas, 18, 4, "- Marquee text ticker");
    mvwprintw(canvas, 19, 4, "- Frame border styles");
    mvwprintw(canvas, 20, 4, "- Deck with shadows");
    wattroff(canvas, VDK_COLORS(COLOR_WHITE, COLOR_BLUE));

    return 0;
}

static vk_widget_t*
build_about_widget(int width, int height)
{
    vk_widget_t *widget;

    widget = vk_widget_create(width, height);
    if(widget == NULL) return NULL;

    vk_widget_set_colors(widget, COLOR_WHITE, COLOR_BLUE);
    vk_object_register_event(VK_OBJECT(widget),
        VK_EVENT_ON_RECREATE, about_on_recreate, NULL);
    about_on_recreate(VK_OBJECT(widget), VK_EVENT_ON_RECREATE, NULL);

    return widget;
}

static vk_selectbox_t*
build_checkbox(int width, int height)
{
    vk_selectbox_t  *selectbox;

    selectbox = vk_selectbox_create(width, height, VK_SELECTBOX_CHECKBOX);
    if(selectbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(selectbox), COLOR_WHITE, COLOR_MAGENTA);
    vk_selectbox_set_highlight(selectbox, COLOR_MAGENTA, COLOR_WHITE);
    vk_selectbox_set_wrap(selectbox, TRUE);

    vk_selectbox_add_item(selectbox, "Write unit tests", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Update documentation", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Fix memory leaks", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Code review", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Profile performance", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Add CI pipeline", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Refactor widgets", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Fix warnings", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Add error handling", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Write examples", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Audit dependencies", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Set up linter", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Benchmark rendering", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Add man pages", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Valgrind clean run", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Fuzzer harness", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Thread safety audit", NULL, NULL);
    vk_selectbox_add_item(selectbox, "API versioning", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Pkg-config update", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Release checklist", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Changelog review", NULL, NULL);
    vk_selectbox_add_item(selectbox, "License headers", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Static analysis", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Coverage report", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Cross-compile test", NULL, NULL);
    vk_selectbox_add_item(selectbox, "BSD port test", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Locale testing", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Mouse support", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Color fallbacks", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Accessibility pass", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Drag-and-drop", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Undo/redo stack", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Clipboard support", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Tab completion", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Search in listbox", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Config file parser", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Hotkey bindings", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Theme engine", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Plugin loader", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Event bus", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Focus ring debug", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Z-order manager", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Tooltip widget", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Statusbar widget", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Progress bar", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Spinner widget", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Tree view", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Table widget", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Split pane", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Dialog builder", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Form validation", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Input masks", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Date picker", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Color picker", NULL, NULL);
    vk_selectbox_add_item(selectbox, "File browser", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Notification popups", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Animation engine", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Double buffering", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Memory pool alloc", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Signal handling", NULL, NULL);

    vk_selectbox_check_item(selectbox, 1);
    vk_selectbox_check_item(selectbox, 3);

    return selectbox;
}

static vk_selectbox_t*
build_radio(int width, int height)
{
    vk_selectbox_t  *selectbox;

    selectbox = vk_selectbox_create(width, height, VK_SELECTBOX_RADIO);
    if(selectbox == NULL) return NULL;

    vk_widget_set_colors(VK_WIDGET(selectbox), COLOR_BLACK, COLOR_CYAN);
    vk_selectbox_set_highlight(selectbox, COLOR_CYAN, COLOR_BLACK);
    vk_selectbox_set_wrap(selectbox, TRUE);

    vk_selectbox_add_item(selectbox, "Default", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Dark", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Light", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Solarized", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Monokai", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Nord", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Dracula", NULL, NULL);
    vk_selectbox_add_item(selectbox, "Gruvbox", NULL, NULL);

    vk_selectbox_check_item(selectbox, 0);

    return selectbox;
}

int main(void)
{
    vk_screen_t     *vk_screen;
    WINDOW          *screen;
    int             max_y, max_x;
    int             box_h;
    int             slot_w, inner_w, inner_h;
    int             current_surface = 0;
    int             deck_surface = -1;
    int32_t         key;

    // surface 0: widgets
    vk_box_t        *box;
    vk_window_t     *window1;
    vk_window_t     *window2;
    vk_window_t     *window3;
    vk_scroller_t   *vscroller1;
    vk_scroller_t   *hscroller1;
    vk_scroller_t   *vscroller2;
    vk_scroller_t   *hscroller2;
    vk_scroller_t   *vscroller3;
    vk_listbox_t    *listbox;
    vk_listbox_t    *menu;
    vk_textbox_t    *textbox3;

    // surface 1: languages
    vk_frame_t      *lang_frame;
    vk_scroller_t   *lang_vscroller;
    vk_scroller_t   *lang_hscroller;
    vk_listbox_t    *lang_listbox;

    // surface 2: selectbox
    vk_box_t        *box2;
    vk_window_t     *window4;
    vk_window_t     *window5;
    vk_window_t     *about_window;
    vk_scroller_t   *vscroller4;
    vk_scroller_t   *vscroller5;
    vk_selectbox_t  *checkbox;
    vk_selectbox_t  *radio;
    vk_widget_t     *about;

    // surface 4: deck
    vk_deck_t       *deck;
    vk_window_t     *deck_win1;
    vk_window_t     *deck_win2;
    vk_window_t     *deck_win3;
    vk_window_t     *deck_win4;
    vk_window_t     *deck_win5;
    vk_box_t        *deck_box5;
    vk_button_t     *deck_buttons[5];
    vk_window_t     *deck_win6;
    vk_filedialog_t *filedialog;

    // focus context for event-driven highlighting
    struct focus_ctx fc_s0[3];
    struct focus_ctx fc_s2[3];

    // shared
    vk_marquee_t    *marquee;

    vk_screen = vk_screen_create();
    if(vk_screen == NULL)
    {
        fprintf(stderr, "vk_screen_create failed\n");
        return 1;
    }

    vdk_color_init();
    vk_object_register_event(VK_OBJECT(vk_screen),
        VK_EVENT_ON_TELEPORT, on_teleport, NULL);

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

    box_h = max_y - 1;
    slot_w = max_x / 3;
    inner_w = slot_w - 2;
    inner_h = box_h - 2;

    vk_screen_set_wallpaper(vk_screen, wallpaper_callback);

    // --- marquee (shared across surfaces) ---

    marquee = vk_marquee_create(max_x);
    vk_widget_set_colors(VK_WIDGET(marquee), COLOR_WHITE, COLOR_BLUE);
    vk_widget_set_attrs(VK_WIDGET(marquee), A_BOLD);
    set_marquee_text(marquee, 0);
    vk_marquee_set_direction(marquee, VK_SCROLL_LOOP);
    vk_marquee_set_speed(marquee, 2);
    vk_screen_attach_widget(vk_screen, 0, VK_WIDGET(marquee));
    vk_widget_move(VK_WIDGET(marquee), 0, 0);

    // --- surface 0: widgets ---

    box = vk_box_create(max_x, box_h, VK_BOX_HORIZONTAL, 3);
    vk_object_set_kmio(VK_OBJECT(box), box_kmio);
    vk_screen_attach_widget(vk_screen, 0, VK_WIDGET(box));
    vk_widget_move(VK_WIDGET(box), 0, 1);

    // panel 1: window with listbox + scrollers
    window1 = vk_window_create(slot_w, box_h);

    vk_window_set_title(window1, " Listbox ");
    vk_window_set_title_justify(window1, VK_JUSTIFY_LEFT);
    vk_window_set_border_colors(window1, COLOR_CYAN, COLOR_BLACK);

    listbox = build_listbox(inner_w, inner_h);
    vk_object_set_kmio(VK_OBJECT(listbox), listbox_kmio);
    vk_widget_set_expand(VK_WIDGET(listbox));
    vk_window_set_child(window1, VK_WIDGET(listbox));

    vscroller1 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller1, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(vscroller1, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller1, VK_WIDGET(listbox));
    vk_scroller_set_scroll_info(vscroller1, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window1), vscroller1);

    hscroller1 = vk_scroller_create(VK_SCROLLBAR_HORIZONTAL);
    vk_scroller_set_border_style(hscroller1, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(hscroller1, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_set_scroll_source(hscroller1, VK_WIDGET(listbox));
    vk_scroller_set_scroll_info(hscroller1, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window1), hscroller1);

    // panel 2: window with menu + scrollers
    window2 = vk_window_create(slot_w, box_h);

    vk_window_set_title(window2, " Menu ");
    vk_window_set_title_justify(window2, VK_JUSTIFY_CENTER);
    vk_window_set_border_colors(window2, COLOR_WHITE, COLOR_BLACK);

    menu = build_menu(inner_w, inner_h);
    vk_object_set_kmio(VK_OBJECT(menu), listbox_kmio);
    vk_widget_set_expand(VK_WIDGET(menu));
    vk_window_set_child(window2, VK_WIDGET(menu));

    vscroller2 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller2, VK_FRAME_DOUBLE);
    vk_scroller_set_border_colors(vscroller2, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller2, VK_WIDGET(menu));
    vk_scroller_set_scroll_info(vscroller2, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window2), vscroller2);

    hscroller2 = vk_scroller_create(VK_SCROLLBAR_HORIZONTAL);
    vk_scroller_set_border_style(hscroller2, VK_FRAME_DOUBLE);
    vk_scroller_set_border_colors(hscroller2, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_set_scroll_source(hscroller2, VK_WIDGET(menu));
    vk_scroller_set_scroll_info(hscroller2, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window2), hscroller2);

    // panel 3: window with textbox + scroller attached to textbox
    window3 = vk_window_create(slot_w, box_h);

    vk_window_set_title(window3, " Textbox ");
    vk_window_set_title_justify(window3, VK_JUSTIFY_RIGHT);
    vk_window_set_border_colors(window3, COLOR_WHITE, COLOR_BLACK);

    textbox3 = build_textbox(inner_w, inner_h);
    vk_object_set_kmio(VK_OBJECT(textbox3), textbox_kmio);
    vk_widget_set_expand(VK_WIDGET(textbox3));
    vk_window_set_child(window3, VK_WIDGET(textbox3));

    vscroller3 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller3, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(vscroller3, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller3, VK_WIDGET(textbox3));
    vk_scroller_set_scroll_info(vscroller3, textbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(textbox3), vscroller3);

    vk_widget_set_expand(VK_WIDGET(window1));
    vk_widget_set_expand(VK_WIDGET(window2));
    vk_widget_set_expand(VK_WIDGET(window3));
    vk_box_set_widget(box, 0, VK_WIDGET(window1));
    vk_box_set_widget(box, 1, VK_WIDGET(window2));
    vk_box_set_widget(box, 2, VK_WIDGET(window3));

    vk_listbox_update(listbox);
    vk_listbox_update(menu);
    vk_textbox_update(textbox3);
    vk_window_update(window1);
    vk_window_update(window2);
    vk_window_update(window3);

    vk_box_update(box);

    // surface 0: event registrations
    fc_s0[0] = (struct focus_ctx){ window1, vscroller1, hscroller1, COLOR_CYAN };
    fc_s0[1] = (struct focus_ctx){ window2, vscroller2, hscroller2, COLOR_GREEN };
    fc_s0[2] = (struct focus_ctx){ window3, vscroller3, NULL, COLOR_YELLOW };

    vk_object_register_event(VK_OBJECT(window1),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s0[0]);
    vk_object_register_event(VK_OBJECT(window1),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s0[0]);
    vk_object_register_event(VK_OBJECT(window2),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s0[1]);
    vk_object_register_event(VK_OBJECT(window2),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s0[1]);
    vk_object_register_event(VK_OBJECT(window3),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s0[2]);
    vk_object_register_event(VK_OBJECT(window3),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s0[2]);

    vk_object_register_event(VK_OBJECT(listbox),
        VK_EVENT_ON_SELECT, on_listbox_select, window1);
    vk_object_register_event(VK_OBJECT(menu),
        VK_EVENT_ON_SELECT, on_listbox_select, window2);
    vk_object_register_event(VK_OBJECT(textbox3),
        VK_EVENT_ON_SCROLL, on_textbox_scroll, window3);

    // --- surface 1: languages ---

    vk_screen_add_surface(vk_screen);

    lang_frame = vk_frame_create(max_x, box_h);

    vk_frame_set_border_style(lang_frame, VK_FRAME_DOUBLE);
    vk_frame_set_border_colors(lang_frame, COLOR_YELLOW, COLOR_BLACK);

    lang_listbox = build_lang_listbox(max_x - 2, box_h - 2);
    vk_object_set_kmio(VK_OBJECT(lang_listbox), listbox_kmio);
    vk_widget_set_expand(VK_WIDGET(lang_listbox));
    vk_frame_set_child(lang_frame, VK_WIDGET(lang_listbox));

    lang_vscroller = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(lang_vscroller, VK_FRAME_DOUBLE);
    vk_scroller_set_border_colors(lang_vscroller, COLOR_YELLOW, COLOR_BLACK);
    vk_scroller_set_scroll_source(lang_vscroller, VK_WIDGET(lang_listbox));
    vk_scroller_set_scroll_info(lang_vscroller, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(lang_frame), lang_vscroller);

    lang_hscroller = vk_scroller_create(VK_SCROLLBAR_HORIZONTAL);
    vk_scroller_set_border_style(lang_hscroller, VK_FRAME_DOUBLE);
    vk_scroller_set_border_colors(lang_hscroller, COLOR_YELLOW, COLOR_BLACK);
    vk_scroller_set_scroll_source(lang_hscroller, VK_WIDGET(lang_listbox));
    vk_scroller_set_scroll_info(lang_hscroller, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(lang_frame), lang_hscroller);

    vk_screen_attach_widget(vk_screen, 1, VK_WIDGET(lang_frame));
    vk_widget_move(VK_WIDGET(lang_frame), 0, 1);

    vk_listbox_update(lang_listbox);
    vk_frame_update(lang_frame);

    // --- surface 2: selectbox ---

    vk_screen_add_surface(vk_screen);

    box2 = vk_box_create(max_x, box_h, VK_BOX_HORIZONTAL, 3);
    vk_object_set_kmio(VK_OBJECT(box2), box_kmio);
    vk_screen_attach_widget(vk_screen, 2, VK_WIDGET(box2));
    vk_widget_move(VK_WIDGET(box2), 0, 1);

    // pane 1: checkbox selectbox
    window4 = vk_window_create(slot_w, box_h);

    vk_window_set_title(window4, " Checkbox ");
    vk_window_set_title_justify(window4, VK_JUSTIFY_LEFT);
    vk_window_set_border_colors(window4, COLOR_CYAN, COLOR_BLACK);

    checkbox = build_checkbox(inner_w, inner_h);
    vk_object_set_kmio(VK_OBJECT(checkbox), selectbox_kmio);
    vk_widget_set_expand(VK_WIDGET(checkbox));
    vk_window_set_child(window4, VK_WIDGET(checkbox));

    vscroller4 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller4, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(vscroller4, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller4, VK_WIDGET(checkbox));
    vk_scroller_set_scroll_info(vscroller4, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window4), vscroller4);

    // pane 2: radio selectbox
    window5 = vk_window_create(slot_w, box_h);

    vk_window_set_title(window5, " Radio ");
    vk_window_set_title_justify(window5, VK_JUSTIFY_CENTER);
    vk_window_set_border_colors(window5, COLOR_WHITE, COLOR_BLACK);

    radio = build_radio(inner_w, inner_h);
    vk_object_set_kmio(VK_OBJECT(radio), selectbox_kmio);
    vk_widget_set_expand(VK_WIDGET(radio));
    vk_window_set_child(window5, VK_WIDGET(radio));

    vscroller5 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller5, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(vscroller5, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller5, VK_WIDGET(radio));
    vk_scroller_set_scroll_info(vscroller5, listbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window5), vscroller5);

    // pane 3: about
    about_window = vk_window_create(slot_w, box_h);

    vk_window_set_border_style(about_window, VK_FRAME_SINGLE);
    vk_window_set_border_colors(about_window, COLOR_WHITE, COLOR_BLACK);
    vk_window_set_title(about_window, " About ");
    vk_window_set_title_justify(about_window, VK_JUSTIFY_RIGHT);

    about = build_about_widget(inner_w, inner_h);
    vk_widget_set_expand(about);
    vk_window_set_child(about_window, about);

    vk_widget_set_expand(VK_WIDGET(window4));
    vk_widget_set_expand(VK_WIDGET(window5));
    vk_widget_set_expand(VK_WIDGET(about_window));
    vk_box_set_widget(box2, 0, VK_WIDGET(window4));
    vk_box_set_widget(box2, 1, VK_WIDGET(window5));
    vk_box_set_widget(box2, 2, VK_WIDGET(about_window));

    vk_selectbox_update(checkbox);
    vk_selectbox_update(radio);
    vk_window_update(window4);
    vk_window_update(window5);
    vk_window_update(about_window);

    vk_box_update(box2);

    // surface 2: event registrations
    fc_s2[0] = (struct focus_ctx){ window4, vscroller4, NULL, COLOR_CYAN };
    fc_s2[1] = (struct focus_ctx){ window5, vscroller5, NULL, COLOR_GREEN };
    fc_s2[2] = (struct focus_ctx){ about_window, NULL, NULL, COLOR_YELLOW };

    vk_object_register_event(VK_OBJECT(window4),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s2[0]);
    vk_object_register_event(VK_OBJECT(window4),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s2[0]);
    vk_object_register_event(VK_OBJECT(window5),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s2[1]);
    vk_object_register_event(VK_OBJECT(window5),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s2[1]);
    vk_object_register_event(VK_OBJECT(about_window),
        VK_EVENT_ON_FOCUS, on_window_focus, &fc_s2[2]);
    vk_object_register_event(VK_OBJECT(about_window),
        VK_EVENT_ON_UNFOCUS, on_window_unfocus, &fc_s2[2]);

    vk_object_register_event(VK_OBJECT(checkbox),
        VK_EVENT_ON_SELECT, on_checkbox_change, window4);
    vk_object_register_event(VK_OBJECT(checkbox),
        VK_EVENT_ON_UNSELECT, on_checkbox_change, window4);
    vk_object_register_event(VK_OBJECT(radio),
        VK_EVENT_ON_SELECT, on_radio_change, window5);

    // --- surface 3: dotfield wallpaper ---

    vk_screen_add_surface(vk_screen);

    // --- surface 4: shade ---

    vk_screen_add_surface(vk_screen);

    // --- deck (floating, toggled with 'w') ---

    deck = vk_deck_create();
    vk_object_set_kmio(VK_OBJECT(deck), deck_kmio);
    vk_deck_set_shadow(deck, TRUE);

    deck_win1 = vk_window_create(35, 10);
    vk_window_set_title(deck_win1, " Notes ");
    vk_window_set_border_style(deck_win1, VK_FRAME_SINGLE);
    vk_window_set_border_colors(deck_win1, COLOR_WHITE, COLOR_BLACK);
    vk_widget_set_colors(VK_WIDGET(deck_win1), COLOR_WHITE, COLOR_BLUE);
    vk_window_set_decorate(deck_win1, deck_notes_decorate, NULL);
    vk_window_update(deck_win1);
    vk_deck_add_widget(deck, VK_WIDGET(deck_win1), VK_DECK_TOP);
    vk_widget_move(VK_WIDGET(deck_win1), 3, 2);

    deck_win2 = vk_window_create(35, 10);
    vk_window_set_title(deck_win2, " Tasks ");
    vk_window_set_border_style(deck_win2, VK_FRAME_DOUBLE);
    vk_window_set_border_colors(deck_win2, COLOR_RED, COLOR_BLACK);
    vk_widget_set_colors(VK_WIDGET(deck_win2), COLOR_WHITE, COLOR_GREEN);
    vk_window_set_decorate(deck_win2, deck_tasks_decorate, NULL);
    vk_window_update(deck_win2);
    vk_deck_add_widget(deck, VK_WIDGET(deck_win2), VK_DECK_TOP);
    vk_widget_move(VK_WIDGET(deck_win2), 15, 5);

    deck_win3 = vk_window_create(35, 10);
    vk_window_set_title(deck_win3, " Help ");
    vk_window_set_border_style(deck_win3, VK_FRAME_SINGLE);
    vk_window_set_border_colors(deck_win3, COLOR_WHITE, COLOR_BLACK);
    vk_widget_set_colors(VK_WIDGET(deck_win3), COLOR_WHITE, COLOR_MAGENTA);
    vk_window_set_decorate(deck_win3, deck_help_decorate, NULL);
    vk_window_update(deck_win3);
    vk_deck_add_widget(deck, VK_WIDGET(deck_win3), VK_DECK_TOP);
    vk_widget_move(VK_WIDGET(deck_win3), 27, 8);

    deck_win4 = vk_window_create(35, 10);
    vk_window_set_title(deck_win4, " Log ");
    vk_window_set_border_style(deck_win4,
        VK_FRAME_SINGLE | VK_FRAME_REVERSE);
    vk_window_set_border_colors(deck_win4, COLOR_YELLOW, COLOR_BLACK);
    vk_widget_set_colors(VK_WIDGET(deck_win4), COLOR_WHITE, COLOR_CYAN);
    vk_window_set_decorate(deck_win4, deck_log_decorate, NULL);
    vk_window_update(deck_win4);
    vk_deck_add_widget(deck, VK_WIDGET(deck_win4), VK_DECK_TOP);
    vk_widget_move(VK_WIDGET(deck_win4), 39, 11);

    {
        int         has_utf8;
        int         bi;
        const char  *btn_labels_utf8[] =
        {
            "\xe2\x96\xb6",        /* ▶ */
            "\xc2\xbb",            /* » */
            "\xc2\xab",            /* « */
            "\xe2\x96\xa0",        /* ■ */
            "\xe2\x8f\xb8",        /* ⏸ */
        };
        const char  *btn_labels_ascii[] =
        {
            "Play", "FF", "RW", "Stop", "Pause",
        };

        has_utf8 = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0);

        deck_win5 = vk_window_create(35, 10);
        vk_window_set_title(deck_win5, " Transport ");
        vk_window_set_border_style(deck_win5,
            VK_FRAME_DOUBLE | VK_FRAME_REVERSE);
        vk_window_set_border_colors(deck_win5, COLOR_MAGENTA, COLOR_BLACK);
        vk_widget_set_colors(VK_WIDGET(deck_win5), COLOR_WHITE, COLOR_RED);
        vk_window_set_decorate(deck_win5, deck_transport_decorate, NULL);

        for(bi = 0; bi < 5; bi++)
        {
            const char *label = has_utf8
                ? btn_labels_utf8[bi] : btn_labels_ascii[bi];

            deck_buttons[bi] = vk_button_create(label);
            vk_widget_set_colors(VK_WIDGET(deck_buttons[bi]),
                COLOR_WHITE, COLOR_RED);
            vk_button_set_pressed_colors(deck_buttons[bi],
                COLOR_BLACK, COLOR_WHITE);
            if(bi == 3)
                vk_button_set_relief_style(deck_buttons[bi],
                    VK_FRAME_ASCII);
            vk_button_update(deck_buttons[bi]);
        }

        deck_box5 = vk_box_create(33, 3, VK_BOX_HORIZONTAL, 5);
        vk_widget_set_colors(VK_WIDGET(deck_box5), COLOR_WHITE, COLOR_RED);

        for(bi = 0; bi < 5; bi++)
            vk_box_set_widget(deck_box5, bi, VK_WIDGET(deck_buttons[bi]));

        vk_object_set_kmio(VK_OBJECT(deck_box5), transport_kmio);


        vk_widget_set_attrs(VK_WIDGET(deck_buttons[0]), A_BOLD);

        vk_window_set_child(deck_win5, VK_WIDGET(deck_box5));
        vk_widget_move(VK_WIDGET(deck_box5), 1, 6);
        vk_window_update(deck_win5);
        vk_deck_add_widget(deck, VK_WIDGET(deck_win5), VK_DECK_TOP);
        vk_widget_move(VK_WIDGET(deck_win5), 51, 14);
    }

    // deck window 6: file dialog
    {
        deck_win6 = vk_window_create(42, 20);
        vk_window_set_title(deck_win6, " Files ");
        vk_window_set_border_style(deck_win6, VK_FRAME_SINGLE);
        vk_window_set_border_colors(deck_win6, COLOR_WHITE, COLOR_BLACK);
        vk_widget_set_colors(VK_WIDGET(deck_win6), COLOR_WHITE, COLOR_BLUE);
        vk_window_set_decorate(deck_win6, deck_files_decorate, NULL);

        filedialog = vk_filedialog_create(40, 15, VK_FRAME_SINGLE, true);
        vk_filedialog_set_colors(filedialog, COLOR_WHITE, COLOR_BLUE);
        vk_filedialog_set_highlight(filedialog, COLOR_BLUE, COLOR_WHITE);



        vk_window_set_child(deck_win6, VK_WIDGET(filedialog));
        vk_widget_move(VK_WIDGET(filedialog), 1, 3);
        vk_filedialog_update(filedialog);
        vk_window_update(deck_win6);
        vk_deck_add_widget(deck, VK_WIDGET(deck_win6), VK_DECK_TOP);
        vk_widget_move(VK_WIDGET(deck_win6), 63, 17);
    }

    // --- initial draw and event loop ---

    vk_box_update(box);
    vk_marquee_run(marquee);
    vk_screen_refresh(vk_screen);

    wtimeout(stdscr, 100);

    while((key = wgetch(stdscr)) != 'q')
    {
        if(key == KEY_RESIZE)
        {
            vk_screen_resize(vk_screen);

            screen = vk_screen_get_window(vk_screen);
            getmaxyx(screen, max_y, max_x);

            box_h = max_y - 1;
            slot_w = max_x / 3;

            vk_widget_resize(VK_WIDGET(marquee), max_x, 1);
            vk_widget_resize(VK_WIDGET(box), max_x, box_h);
            vk_widget_resize(VK_WIDGET(lang_frame), max_x, box_h);
            vk_widget_resize(VK_WIDGET(box2), max_x, box_h);
        }
        else if(key == 'd')
        {
            int old = current_surface;
            current_surface = (current_surface + 1) % NUM_SURFACES;

            vk_screen_detach_widget(vk_screen, old, VK_WIDGET(marquee));
            vk_screen_switch_surface(vk_screen, current_surface);
            vk_screen_attach_widget(vk_screen, current_surface,
                VK_WIDGET(marquee));
            vk_widget_move(VK_WIDGET(marquee), 0, 0);

            set_marquee_text(marquee, current_surface);
        }
        else if(key == 't')
        {
            char    pty_path[128] = "";
            int     pos = 0;
            int     colors;
            int     ch;

            screen = vk_screen_get_window(vk_screen);
            colors = VDK_COLORS(COLOR_WHITE, COLOR_BLUE) | A_BOLD;
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

                box_h = max_y - 1;
                slot_w = max_x / 3;

                vk_widget_resize(VK_WIDGET(marquee), max_x, 1);
                vk_widget_resize(VK_WIDGET(box), max_x, box_h);
                vk_widget_resize(VK_WIDGET(lang_frame), max_x, box_h);
                vk_widget_resize(VK_WIDGET(box2), max_x, box_h);
    
                wtimeout(stdscr, 100);
            }
        }
        else if(key == 'h')
        {
            if(vk_widget_is_visible(VK_WIDGET(marquee)))
                vk_widget_hide(VK_WIDGET(marquee));
            else
                vk_widget_show(VK_WIDGET(marquee));
        }
        else if(key == 'f')
        {
            if(vk_widget_get_state(VK_WIDGET(marquee)) & VK_STATE_FROZEN)
                vk_widget_thaw(VK_WIDGET(marquee));
            else
                vk_widget_freeze(VK_WIDGET(marquee));
        }
        else if(key == 'w')
        {
            if(deck_surface == current_surface)
            {
                vk_screen_detach_widget(vk_screen, deck_surface,
                    VK_WIDGET(deck));
                deck_surface = -1;
            }
            else
            {
                if(deck_surface >= 0)
                    vk_screen_detach_widget(vk_screen, deck_surface,
                        VK_WIDGET(deck));

                vk_screen_attach_widget(vk_screen, current_surface,
                    VK_WIDGET(deck));
                deck_surface = current_surface;
            }
        }
        else if(key == KEY_SRIGHT || key == KEY_SLEFT
            || key == '+' || key == '-')
        {
            vk_widget_t *target = NULL;

            if(current_surface == 0) target = VK_WIDGET(box);
            else if(current_surface == 1) target = VK_WIDGET(lang_frame);
            else if(current_surface == 2) target = VK_WIDGET(box2);

            if(target != NULL)
            {
                int tw, th;
                vk_widget_get_metrics(target, &tw, &th);

                if(key == KEY_SRIGHT)
                    vk_widget_resize(target, tw + 1, WSIZE_UNCHANGED);
                else if(key == KEY_SLEFT && tw > 12)
                    vk_widget_resize(target, tw - 1, WSIZE_UNCHANGED);
                else if(key == '+')
                    vk_widget_resize(target, WSIZE_UNCHANGED, th + 1);
                else if(key == '-' && th > 6)
                    vk_widget_resize(target, WSIZE_UNCHANGED, th - 1);
            }
        }
        else if(key != ERR)
        {
            if(deck_surface == current_surface)
                vk_object_push_keystroke(VK_OBJECT(deck), key);
            else if(current_surface == 0)
                vk_object_push_keystroke(VK_OBJECT(box), key);
            else if(current_surface == 1)
                vk_object_push_keystroke(VK_OBJECT(lang_frame), key);
            else if(current_surface == 2)
                vk_object_push_keystroke(VK_OBJECT(box2), key);
        }

        if(current_surface == 0)
        {
            vk_window_update(window1);
            vk_window_update(window2);
            vk_textbox_update(textbox3);
            vk_window_update(window3);
            vk_box_update(box);
        }
        else if(current_surface == 1)
        {
            vk_listbox_update(lang_listbox);
            vk_frame_update(lang_frame);
        }
        else if(current_surface == 2)
        {
            vk_selectbox_update(checkbox);
            vk_window_update(window4);
            vk_selectbox_update(radio);
            vk_window_update(window5);
            vk_window_update(about_window);
            vk_box_update(box2);
        }
        if(deck_surface == current_surface)
        {
            vk_widget_t *top = vk_deck_get_top(deck);

            short c1 = COLOR_WHITE, c2 = COLOR_RED, c3 = COLOR_WHITE;
            short c4 = COLOR_YELLOW, c5 = COLOR_MAGENTA, c6 = COLOR_WHITE;
            if(top == VK_WIDGET(deck_win1)) c1 = COLOR_YELLOW;
            else if(top == VK_WIDGET(deck_win2)) c2 = COLOR_GREEN;
            else if(top == VK_WIDGET(deck_win3)) c3 = COLOR_CYAN;
            else if(top == VK_WIDGET(deck_win4)) c4 = COLOR_WHITE;
            else if(top == VK_WIDGET(deck_win5)) c5 = COLOR_WHITE;
            else if(top == VK_WIDGET(deck_win6)) c6 = COLOR_GREEN;

            vk_window_set_border_colors(deck_win1, c1, COLOR_BLACK);
            vk_window_set_border_colors(deck_win2, c2, COLOR_BLACK);
            vk_window_set_border_colors(deck_win3, c3, COLOR_BLACK);
            vk_window_set_border_colors(deck_win4, c4, COLOR_BLACK);
            vk_window_set_border_colors(deck_win5, c5, COLOR_BLACK);
            vk_window_set_border_colors(deck_win6, c6, COLOR_BLACK);

            vk_window_update(deck_win1);
            vk_window_update(deck_win2);
            vk_window_update(deck_win3);
            vk_window_update(deck_win4);

            {
                int bi;
                int bf = vk_box_get_subfocus(deck_box5);
                for(bi = 0; bi < 5; bi++)
                {
                    vk_widget_set_attrs(VK_WIDGET(deck_buttons[bi]),
                        (bi == bf) ? A_BOLD : 0);
                    vk_button_update(deck_buttons[bi]);
                }
            }
            vk_box_update(deck_box5);
            vk_window_update(deck_win5);

            vk_filedialog_update(filedialog);
            vk_window_update(deck_win6);
        }

        vk_marquee_run(marquee);
        vk_screen_refresh(vk_screen);
    }

    vk_marquee_destroy(marquee);
    vk_box_destroy(box);

    vk_scroller_destroy(vscroller1);
    vk_scroller_destroy(hscroller1);
    vk_scroller_destroy(vscroller2);
    vk_scroller_destroy(hscroller2);
    vk_scroller_destroy(vscroller3);

    vk_window_destroy(window1);
    vk_window_destroy(window2);
    vk_window_destroy(window3);

    vk_listbox_destroy(listbox);
    vk_listbox_destroy(menu);
    vk_textbox_destroy(textbox3);

    vk_scroller_destroy(lang_vscroller);
    vk_scroller_destroy(lang_hscroller);
    vk_frame_destroy(lang_frame);
    vk_listbox_destroy(lang_listbox);

    vk_scroller_destroy(vscroller4);
    vk_scroller_destroy(vscroller5);
    vk_box_destroy(box2);
    vk_window_destroy(window4);
    vk_window_destroy(window5);
    vk_window_destroy(about_window);
    vk_selectbox_destroy(checkbox);
    vk_selectbox_destroy(radio);
    vk_widget_destroy(about);

    vk_deck_destroy(deck);
    vk_window_destroy(deck_win1);
    vk_window_destroy(deck_win2);
    vk_window_destroy(deck_win3);
    vk_window_destroy(deck_win4);
    {
        int bi;
        for(bi = 0; bi < 5; bi++)
            vk_button_destroy(deck_buttons[bi]);
    }
    vk_box_destroy(deck_box5);
    vk_window_destroy(deck_win5);
    vk_window_destroy(deck_win6);
    vk_filedialog_destroy(filedialog);

    vk_screen_destroy(vk_screen);

    return 0;
}
