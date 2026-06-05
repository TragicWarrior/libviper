#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_listbox.h"
#include "vk_item.h"
#include "vk_frame.h"
#include "vk_scroller.h"
#include "vk_box.h"
#include "vk_label.h"
#include "vk_marquee.h"
#include "vk_window.h"
#include "vk_textbox.h"
#include "vk_selectbox.h"
#include "vk_screen.h"

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
textbox_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_textbox_t *tb = VK_TEXTBOX(child);

    if(content_h) *content_h = tb->line_count;
    if(content_w) *content_w = 0;
    if(scroll_y) *scroll_y = tb->scroll_top;
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
            colors = VIPER_COLORS(COLOR_GREEN, COLOR_BLACK);
            wbkgd(canvas, ' ' | colors);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, ((x + y) % 4 == 0) ? '+' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 1:
        {
            colors = VIPER_COLORS(COLOR_YELLOW, COLOR_BLACK);
            wbkgd(canvas, ' ' | colors);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, (y % 2 == 0) ? '-' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 2:
        {
            colors = VIPER_COLORS(COLOR_RED, COLOR_BLACK);
            wbkgd(canvas, ' ' | colors);
            wattron(canvas, colors);

            for(y = 0; y < max_y; y++)
                for(x = 0; x < max_x; x++)
                    mvwaddch(canvas, y, x, ((x % 6 == 0) || (y % 3 == 0)) ? '.' : ' ');

            wattroff(canvas, colors);
            break;
        }

        case 3:
        {
            colors = VIPER_COLORS(COLOR_BLUE, COLOR_BLACK);
            wbkgd(canvas, ' ' | colors);
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
            short   pair = viper_color_pair(COLOR_CYAN, COLOR_BLACK);

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
set_marquee_text(vk_marquee_t *marquee, int surface)
{
    char text[512];

    snprintf(text, sizeof(text),
        "[Surface %d: %s]"
        "  VK Klass Reference Demo"
        "  |  Built: " __DATE__ " " __TIME__
        "  |  TAB:focus  \xE2\x86\x91\xE2\x86\x93:nav  Enter:sel"
        "  d:surface  t:teleport  q:quit",
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
about_on_recreate(vk_widget_t *widget)
{
    int attr;

    attr = VIPER_COLORS(COLOR_YELLOW, COLOR_BLUE);
    wbkgd(widget->canvas, ' ' | VIPER_COLORS(COLOR_WHITE, COLOR_BLUE));

    wattron(widget->canvas, attr | A_BOLD);
    mvwprintw(widget->canvas, 1, 2, "VK Widget Toolkit");
    wattroff(widget->canvas, attr | A_BOLD);

    mvwprintw(widget->canvas, 3,  2, "Keyboard:");
    mvwprintw(widget->canvas, 4,  4, "TAB ......... cycle focus");
    mvwprintw(widget->canvas, 5,  4, "Up/Down ..... navigate items");
    mvwprintw(widget->canvas, 6,  4, "Enter ....... select item");
    mvwprintw(widget->canvas, 7,  4, "d ........... switch surface");
    mvwprintw(widget->canvas, 8,  4, "t ........... teleport to PTY");
    mvwprintw(widget->canvas, 9,  4, "q ........... quit");

    mvwprintw(widget->canvas, 11, 2, "Features:");
    mvwprintw(widget->canvas, 12, 4, "- Virtual surfaces");
    mvwprintw(widget->canvas, 13, 4, "- Terminal migration (teleport)");
    mvwprintw(widget->canvas, 14, 4, "- Scrollable containers");
    mvwprintw(widget->canvas, 15, 4, "- Marquee text ticker");
    mvwprintw(widget->canvas, 16, 4, "- Frame border styles");
    mvwprintw(widget->canvas, 17, 4, "- Menu with separators");

    return 0;
}

static vk_widget_t*
build_about_widget(int width, int height)
{
    vk_widget_t *widget;

    widget = vk_widget_create(width, height);
    if(widget == NULL) return NULL;

    vk_widget_set_colors(widget, COLOR_WHITE, COLOR_BLUE);
    widget->_on_recreate = about_on_recreate;
    about_on_recreate(widget);

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

    // shared
    vk_marquee_t    *marquee;

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
    vk_screen_attach_widget(vk_screen, 0, VK_WIDGET(box));
    vk_widget_move(VK_WIDGET(box), 0, 1);

    // panel 1: window with listbox + scrollers
    window1 = vk_window_create(slot_w, box_h);
    vk_window_set_title(window1, " Listbox ");
    vk_window_set_title_justify(window1, VK_JUSTIFY_LEFT);
    vk_window_set_border_colors(window1, COLOR_CYAN, COLOR_BLACK);

    listbox = build_listbox(inner_w, inner_h);
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
    vk_window_set_child(window3, VK_WIDGET(textbox3));

    vscroller3 = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscroller3, VK_FRAME_SINGLE);
    vk_scroller_set_border_colors(vscroller3, COLOR_WHITE, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscroller3, VK_WIDGET(textbox3));
    vk_scroller_set_scroll_info(vscroller3, textbox_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(textbox3), vscroller3);

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

    // --- surface 1: languages ---

    vk_screen_add_surface(vk_screen);

    lang_frame = vk_frame_create(max_x, box_h);
    vk_frame_set_border_style(lang_frame, VK_FRAME_DOUBLE);
    vk_frame_set_border_colors(lang_frame, COLOR_YELLOW, COLOR_BLACK);

    lang_listbox = build_lang_listbox(max_x - 2, box_h - 2);
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
    vk_screen_attach_widget(vk_screen, 2, VK_WIDGET(box2));
    vk_widget_move(VK_WIDGET(box2), 0, 1);

    // pane 1: checkbox selectbox
    window4 = vk_window_create(slot_w, box_h);
    vk_window_set_title(window4, " Checkbox ");
    vk_window_set_title_justify(window4, VK_JUSTIFY_LEFT);
    vk_window_set_border_colors(window4, COLOR_CYAN, COLOR_BLACK);

    checkbox = build_checkbox(inner_w, inner_h);
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
    vk_window_set_child(about_window, about);

    vk_box_set_widget(box2, 0, VK_WIDGET(window4));
    vk_box_set_widget(box2, 1, VK_WIDGET(window5));
    vk_box_set_widget(box2, 2, VK_WIDGET(about_window));

    vk_selectbox_update(checkbox);
    vk_selectbox_update(radio);
    vk_window_update(window4);
    vk_window_update(window5);
    vk_window_update(about_window);

    vk_box_update(box2);

    // --- surface 3: dotfield wallpaper ---

    vk_screen_add_surface(vk_screen);

    // --- surface 4: shade wallpaper ---

    vk_screen_add_surface(vk_screen);

    // --- initial draw and event loop ---

    vk_box_update(box);
    vk_marquee_run(marquee);
    vk_screen_refresh(vk_screen);

    wtimeout(stdscr, 100);

    while((key = wgetch(stdscr)) != 'q')
    {
        if(key == KEY_RESIZE || vk_screen_poll_resize(vk_screen))
        {
            if(key == KEY_RESIZE) vk_screen_resize(vk_screen);

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

                box_h = max_y - 1;
                slot_w = max_x / 3;

                vk_widget_resize(VK_WIDGET(marquee), max_x, 1);
                vk_widget_resize(VK_WIDGET(box), max_x, box_h);
                vk_widget_resize(VK_WIDGET(lang_frame), max_x, box_h);
                vk_widget_resize(VK_WIDGET(box2), max_x, box_h);

                wtimeout(stdscr, 100);
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
                if(key == KEY_SRIGHT)
                    vk_widget_resize(target,
                        target->width + 1, WSIZE_UNCHANGED);
                else if(key == KEY_SLEFT && target->width > 12)
                    vk_widget_resize(target,
                        target->width - 1, WSIZE_UNCHANGED);
                else if(key == '+')
                    vk_widget_resize(target,
                        WSIZE_UNCHANGED, target->height + 1);
                else if(key == '-' && target->height > 6)
                    vk_widget_resize(target,
                        WSIZE_UNCHANGED, target->height - 1);
            }
        }
        else if(key != ERR)
        {
            if(current_surface == 0)
                vk_object_push_keystroke(VK_OBJECT(box), key);
            else if(current_surface == 1)
                vk_object_push_keystroke(VK_OBJECT(lang_frame), key);
            else if(current_surface == 2)
                vk_object_push_keystroke(VK_OBJECT(box2), key);
        }

        if(current_surface == 0)
        {
            short w1_fg = (box->focused_slot == 0) ? COLOR_CYAN : COLOR_WHITE;
            short w2_fg = (box->focused_slot == 1) ? COLOR_GREEN : COLOR_WHITE;
            short w3_fg = (box->focused_slot == 2) ? COLOR_YELLOW : COLOR_WHITE;

            vk_window_set_border_colors(window1, w1_fg, COLOR_BLACK);
            vk_scroller_set_border_colors(vscroller1, w1_fg, COLOR_BLACK);
            vk_scroller_set_border_colors(hscroller1, w1_fg, COLOR_BLACK);
            vk_window_update(window1);

            vk_window_set_border_colors(window2, w2_fg, COLOR_BLACK);
            vk_scroller_set_border_colors(vscroller2, w2_fg, COLOR_BLACK);
            vk_scroller_set_border_colors(hscroller2, w2_fg, COLOR_BLACK);
            vk_window_update(window2);

            vk_window_set_border_colors(window3, w3_fg, COLOR_BLACK);
            vk_scroller_set_border_colors(vscroller3, w3_fg, COLOR_BLACK);
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
            short w4_fg = (box2->focused_slot == 0) ? COLOR_CYAN : COLOR_WHITE;
            short w5_fg = (box2->focused_slot == 1) ? COLOR_GREEN : COLOR_WHITE;
            short wa_fg = (box2->focused_slot == 2) ? COLOR_YELLOW : COLOR_WHITE;

            vk_window_set_border_colors(window4, w4_fg, COLOR_BLACK);
            vk_selectbox_update(checkbox);
            vk_window_update(window4);

            vk_window_set_border_colors(window5, w5_fg, COLOR_BLACK);
            vk_selectbox_update(radio);
            vk_window_update(window5);

            vk_window_set_border_colors(about_window, wa_fg, COLOR_BLACK);
            vk_window_update(about_window);

            vk_box_update(box2);
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

    vk_screen_destroy(vk_screen);

    return 0;
}
