/*
    test_inherit -- headless checks of widget properties and of colours
    handed from a container to what it holds.

    Properties: one is changed at a time and the others are left alone;
    the names that have always been there (show, hide, freeze, thaw,
    set_expand) do what they did.

    Inheritance: a widget that follows its container takes the
    container's colours whenever the container is redrawn -- whatever
    order the tree was built in, and again after the container's
    colours change -- while a widget given colours of its own keeps
    them.

    Exit status 0 when every check passes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "vdk.h"

static int  passed = 0;
static int  failed = 0;

/* record one check */
static void
check(const char *group, const char *what, int ok)
{
    if(ok) passed++;
    else failed++;

    printf("%s  %-10s %s\n", ok ? "PASS" : "FAIL", group, what);
}

/* does the widget have exactly these colours? */
static int
colours(void *widget, short want_fg, short want_bg)
{
    short   fg = -1;
    short   bg = -1;

    vk_widget_get_colors(VK_WIDGET(widget), &fg, &bg);

    return fg == want_fg && bg == want_bg;
}

/* the colours a cell of the widget's canvas was painted in, as RGB */
static void
painted(void *widget, uint32_t *fg, uint32_t *bg)
{
    vdk_cell_t  cell;

    memset(&cell, 0, sizeof(cell));
    vk_widget_read_row(VK_WIDGET(widget), 0, &cell, 1);

    *fg = cell.fg;
    *bg = cell.bg;
}

int
main(void)
{
    vk_screen_t     *screen;
    vk_widget_t     *plain;
    vk_window_t     *window;
    vk_box_t        *outer;
    vk_box_t        *inner;
    vk_label_t      *follower;
    vk_label_t      *loner;
    vk_label_t      *deep;
    vk_label_t      *half;
    vk_grid_t       *grid;
    vk_label_t      *cell;
    vk_frame_t      *frame;
    vk_label_t      *framed;
    vk_popup_t      *popup;
    vk_button_t     *button;
    vk_box_t        *bar;
    vk_label_t      *client;
    uint32_t        state;
    uint32_t        fg_a, bg_a;
    uint32_t        fg_b, bg_b;

    setenv("TERM", "xterm-256color", 1);
    setlocale(LC_ALL, "");

    screen = vk_screen_create_detached(120, 40);
    if(screen == NULL)
    {
        printf("FAIL  could not create a detached screen\n");
        return 1;
    }

    vdk_color_init();

    /* ---- properties ---- */

    plain = vk_widget_create(10, 3);

    check("prop", "a new widget is visible and nothing else",
        vk_widget_get_prop(plain, VK_PROP_VISIBLE)
        && !vk_widget_get_prop(plain, VK_PROP_FROZEN)
        && !vk_widget_get_prop(plain, VK_PROP_NORESIZE)
        && !vk_widget_get_prop(plain, VK_PROP_EXPAND)
        && !vk_widget_get_prop(plain, VK_PROP_INHERIT_FG)
        && !vk_widget_get_prop(plain, VK_PROP_INHERIT_BG)
        && !vk_widget_get_prop(plain, VK_PROP_INHERIT_ATTRS));

    check("prop", "setting one returns 0 and turns it on",
        vk_widget_set_prop(plain, VK_PROP_EXPAND, true) == 0
        && vk_widget_get_prop(plain, VK_PROP_EXPAND));
    check("prop", "and leaves the others as they were",
        vk_widget_get_prop(plain, VK_PROP_VISIBLE)
        && !vk_widget_get_prop(plain, VK_PROP_NORESIZE));

    vk_widget_set_prop(plain, VK_PROP_NORESIZE, true);
    vk_widget_set_prop(plain, VK_PROP_EXPAND, false);
    check("prop", "clearing one leaves the one set after it",
        !vk_widget_get_prop(plain, VK_PROP_EXPAND)
        && vk_widget_get_prop(plain, VK_PROP_NORESIZE)
        && vk_widget_get_prop(plain, VK_PROP_VISIBLE));

    check("prop", "no-resize does refuse a resize",
        vk_widget_resize(plain, 20, 5) == -1);
    vk_widget_set_prop(plain, VK_PROP_NORESIZE, false);
    check("prop", "and a resize works once it is cleared",
        vk_widget_resize(plain, 20, 5) == 0);

    check("prop", "two properties at once is refused",
        vk_widget_set_prop(plain, VK_PROP_VISIBLE | VK_PROP_EXPAND, true)
            == -1);
    check("prop", "something that is not a property is refused",
        vk_widget_set_prop(plain, (1UL << 31), true) == -1
        && vk_widget_set_prop(plain, 0, true) == -1
        && !vk_widget_get_prop(plain, (1UL << 30)));
    check("prop", "a NULL widget is refused",
        vk_widget_set_prop(NULL, VK_PROP_VISIBLE, true) == -1
        && !vk_widget_get_prop(NULL, VK_PROP_VISIBLE));

    /* the names that have always been there */
    vk_widget_hide(plain);
    check("prop", "vk_widget_hide", !vk_widget_is_visible(plain));
    vk_widget_show(plain);
    check("prop", "vk_widget_show", vk_widget_is_visible(plain));
    vk_widget_freeze(plain);
    check("prop", "vk_widget_freeze", vk_widget_is_frozen(plain)
        && (vk_widget_get_state(plain) & VK_STATE_FROZEN));
    vk_widget_thaw(plain);
    check("prop", "vk_widget_thaw", !vk_widget_is_frozen(plain));
    vk_widget_set_expand(plain);
    check("prop", "vk_widget_set_expand", vk_widget_is_expand(plain)
        && (vk_widget_get_state(plain) & VK_STATE_EXPAND));
    vk_widget_clear_expand(plain);
    check("prop", "vk_widget_clear_expand", !vk_widget_is_expand(plain));
    vk_widget_set_noresize(plain);
    check("prop", "vk_widget_set_noresize", vk_widget_is_noresize(plain));
    vk_widget_clear_noresize(plain);
    check("prop", "vk_widget_clear_noresize", !vk_widget_is_noresize(plain));

    /* the raw state word still works, and cannot reach the bits the
       library keeps for itself (whatever lies outside the public ones) */
    {
        uint32_t    known = VK_STATE_VISIBLE | VK_STATE_FROZEN
                        | VK_STATE_NORESIZE | VK_STATE_EXPAND
                        | VK_STATE_INHERIT_FG | VK_STATE_INHERIT_BG
                        | VK_STATE_INHERIT_ATTRS;

        vk_widget_set_colors(plain, COLOR_RED, COLOR_BLUE);
        vk_widget_set_expand(plain);
        state = vk_widget_get_state(plain);

        vk_widget_set_state(plain, VK_STATE_VISIBLE);
        check("prop", "vk_widget_set_state still sets what it is given",
            vk_widget_is_visible(plain) && !vk_widget_is_expand(plain));
        check("prop", "but leaves the library's own bits alone",
            (state & ~known) != 0
            && (vk_widget_get_state(plain) & ~known) == (state & ~known));

        vk_widget_set_state(plain, VK_STATE_VISIBLE | (1UL << 31));
        check("prop", "and cannot turn them on either",
            (vk_widget_get_state(plain) & ~known) == (state & ~known));
        vk_widget_set_state(plain, VK_STATE_VISIBLE);
    }

    vk_widget_destroy(plain);

    /* ---- inheritance: built bottom-up, the hard way round ---- */

    /* the leaves are attached before any container has its colours,
       and the window is coloured last of all */
    follower = vk_label_create(12);
    vk_label_set_text(follower, "follower");
    loner = vk_label_create(12);
    vk_label_set_text(loner, "loner");
    vk_widget_set_colors(VK_WIDGET(loner), COLOR_RED, COLOR_WHITE);
    deep = vk_label_create(12);
    vk_label_set_text(deep, "deep");

    inner = vk_box_create(30, 2, VK_BOX_VERTICAL, 1);
    vk_box_set_widget(inner, 0, VK_WIDGET(deep), VK_INHERIT_AUTO);

    outer = vk_box_create(30, 6, VK_BOX_VERTICAL, 3);
    vk_box_set_widget(outer, 0, VK_WIDGET(follower), VK_INHERIT_AUTO);
    vk_box_set_widget(outer, 1, VK_WIDGET(loner), VK_INHERIT_AUTO);
    vk_box_set_widget(outer, 2, VK_WIDGET(inner), VK_INHERIT_AUTO);

    window = vk_window_create(32, 8);
    vk_window_set_child(window, VK_WIDGET(outer), VK_INHERIT_AUTO);
    vk_widget_set_colors(VK_WIDGET(window), COLOR_WHITE, COLOR_BLUE);

    check("inherit", "AUTO marks an uncoloured widget as following",
        vk_widget_get_prop(VK_WIDGET(follower), VK_PROP_INHERIT_FG)
        && vk_widget_get_prop(VK_WIDGET(follower), VK_PROP_INHERIT_BG));
    check("inherit", "and leaves a coloured one alone",
        !vk_widget_get_prop(VK_WIDGET(loner), VK_PROP_INHERIT_FG)
        && !vk_widget_get_prop(VK_WIDGET(loner), VK_PROP_INHERIT_BG));
    check("inherit", "AUTO does not follow text attributes",
        !vk_widget_get_prop(VK_WIDGET(follower), VK_PROP_INHERIT_ATTRS));

    vk_window_redraw(window);

    check("inherit", "one redraw colours a child built before its window",
        colours(outer, COLOR_WHITE, COLOR_BLUE)
        && colours(follower, COLOR_WHITE, COLOR_BLUE));
    check("inherit", "and a widget two containers down",
        colours(inner, COLOR_WHITE, COLOR_BLUE)
        && colours(deep, COLOR_WHITE, COLOR_BLUE));
    check("inherit", "a widget with colours of its own keeps them",
        colours(loner, COLOR_RED, COLOR_WHITE));

    /* it is painted in them, not just holding the numbers */
    painted(follower, &fg_a, &bg_a);
    painted(loner, &fg_b, &bg_b);
    check("inherit", "the follower is painted in the window's colours",
        bg_a != bg_b && fg_a != fg_b);
    painted(deep, &fg_b, &bg_b);
    check("inherit", "so is the one two containers down",
        fg_a == fg_b && bg_a == bg_b);

    /* a later change reaches everything that follows */
    vk_widget_set_colors(VK_WIDGET(window), COLOR_YELLOW, COLOR_MAGENTA);
    vk_window_redraw(window);
    check("inherit", "recolouring the window recolours its followers",
        colours(follower, COLOR_YELLOW, COLOR_MAGENTA)
        && colours(deep, COLOR_YELLOW, COLOR_MAGENTA)
        && colours(loner, COLOR_RED, COLOR_WHITE));

    /* giving a follower its own colours takes it out */
    vk_widget_set_colors(VK_WIDGET(follower), COLOR_GREEN, COLOR_BLACK);
    vk_widget_set_colors(VK_WIDGET(window), COLOR_WHITE, COLOR_BLUE);
    vk_window_redraw(window);
    check("inherit", "vk_widget_set_colors makes the colours its own",
        colours(follower, COLOR_GREEN, COLOR_BLACK)
        && !vk_widget_get_prop(VK_WIDGET(follower), VK_PROP_INHERIT_BG)
        && colours(deep, COLOR_WHITE, COLOR_BLUE));

    /* and a property puts it back */
    vk_widget_set_prop(VK_WIDGET(follower), VK_PROP_INHERIT_FG, true);
    vk_widget_set_prop(VK_WIDGET(follower), VK_PROP_INHERIT_BG, true);
    vk_window_redraw(window);
    check("inherit", "the inherit properties make it follow again",
        colours(follower, COLOR_WHITE, COLOR_BLUE));

    /* one colour its own, the other followed */
    half = vk_label_create(12);
    vk_label_set_text(half, "half");
    vk_widget_set_colors(VK_WIDGET(half), COLOR_CYAN, COLOR_BLACK);
    vk_box_set_widget(outer, 1, VK_WIDGET(half), VK_INHERIT_BG);
    vk_window_redraw(window);
    check("inherit", "VK_INHERIT_BG follows the background and keeps its foreground",
        colours(half, COLOR_CYAN, COLOR_BLUE));

    /* NONE changes nothing about the child */
    vk_box_set_widget(outer, 1, NULL, VK_INHERIT_NONE);
    vk_widget_set_colors(VK_WIDGET(half), COLOR_CYAN, COLOR_BLACK);
    vk_box_set_widget(outer, 1, VK_WIDGET(half), VK_INHERIT_NONE);
    vk_window_redraw(window);
    check("inherit", "VK_INHERIT_NONE leaves the child standing alone",
        colours(half, COLOR_CYAN, COLOR_BLACK));

    /* attributes follow only when asked */
    vk_widget_set_attrs(VK_WIDGET(window), A_BOLD);
    vk_window_redraw(window);
    check("inherit", "bold on the window does not reach an AUTO child",
        vk_widget_get_attrs(VK_WIDGET(deep)) == 0);
    vk_widget_set_prop(VK_WIDGET(outer), VK_PROP_INHERIT_ATTRS, true);
    vk_widget_set_prop(VK_WIDGET(follower), VK_PROP_INHERIT_ATTRS, true);
    vk_window_redraw(window);
    check("inherit", "it does reach one told to follow attributes",
        vk_widget_get_attrs(VK_WIDGET(follower)) == A_BOLD);
    vk_widget_set_attrs(VK_WIDGET(follower), A_UNDERLINE);
    vk_window_redraw(window);
    check("inherit", "vk_widget_set_attrs makes the attributes its own",
        vk_widget_get_attrs(VK_WIDGET(follower)) == A_UNDERLINE);

    vk_box_set_widget(outer, 1, NULL, VK_INHERIT_NONE);
    vk_label_destroy(half);
    vk_label_destroy(loner);
    vk_window_destroy(window);

    /* ---- the other containers ---- */

    grid = vk_grid_create(30, 4, 2, 2);
    cell = vk_label_create(10);
    vk_grid_set_widget(grid, 0, 0, VK_WIDGET(cell), VK_INHERIT_AUTO);
    vk_widget_set_colors(VK_WIDGET(grid), COLOR_BLACK, COLOR_CYAN);
    vk_grid_redraw(grid);
    check("inherit", "a grid hands its colours to its cells",
        colours(cell, COLOR_BLACK, COLOR_CYAN));
    vk_grid_destroy(grid);

    frame = vk_frame_create(20, 4);
    framed = vk_label_create(10);
    vk_frame_set_child(frame, VK_WIDGET(framed), VK_INHERIT_AUTO);
    vk_widget_set_colors(VK_WIDGET(frame), COLOR_WHITE, COLOR_RED);
    vk_frame_redraw(frame);
    check("inherit", "a frame hands its colours to its child",
        colours(framed, COLOR_WHITE, COLOR_RED));
    vk_frame_destroy(frame);

    /* ---- the old one-time copy still happens at attach ---- */

    outer = vk_box_create(20, 2, VK_BOX_VERTICAL, 1);
    vk_widget_set_colors(VK_WIDGET(outer), COLOR_WHITE, COLOR_GREEN);
    follower = vk_label_create(10);
    vk_box_set_widget(outer, 0, VK_WIDGET(follower), VK_INHERIT_COLOR);
    check("inherit", "VK_INHERIT_COLOR gives the colours at attach, before any redraw",
        colours(follower, COLOR_WHITE, COLOR_GREEN));
    vk_box_destroy(outer);

    /* ---- AUTO is the default: it is what no flags at all means ---- */

    outer = vk_box_create(20, 3, VK_BOX_VERTICAL, 3);
    vk_widget_set_colors(VK_WIDGET(outer), COLOR_WHITE, COLOR_GREEN);
    follower = vk_label_create(10);
    loner = vk_label_create(10);
    deep = vk_label_create(10);
    vk_box_set_widget(outer, 0, VK_WIDGET(follower), 0);
    vk_box_set_widget(outer, 1, VK_WIDGET(loner), VK_INHERIT_NONE);
    vk_box_set_widget(outer, 2, VK_WIDGET(deep), VK_INHERIT_ATTRS);
    vk_widget_set_attrs(VK_WIDGET(outer), A_BOLD);
    vk_box_redraw(outer);
    check("default", "VK_INHERIT_AUTO is 0", VK_INHERIT_AUTO == 0);
    check("default", "attaching with 0 follows the container's colours",
        colours(follower, COLOR_WHITE, COLOR_GREEN));
    check("default", "but not its text attributes",
        vk_widget_get_attrs(VK_WIDGET(follower)) == 0);
    check("default", "VK_INHERIT_NONE has to be asked for, and stands alone",
        VK_INHERIT_NONE != 0 && colours(loner, COLOR_BLACK, COLOR_WHITE));
    check("default", "naming ATTRS follows attributes and nothing else",
        vk_widget_get_attrs(VK_WIDGET(deep)) == A_BOLD
        && colours(deep, COLOR_BLACK, COLOR_WHITE));
    vk_box_destroy(outer);

    /* ---- the header says which library it belongs to ---- */

    {
        char    text[32];

        snprintf(text, sizeof(text), "%d.%d.%d", VDK_VERSION_MAJOR,
            VDK_VERSION_MINOR, VDK_VERSION_PATCH);
        check("version", "VDK_VERSION is the three numbers as text",
            strcmp(text, VDK_VERSION) == 0);
        check("version", "this release is at least itself",
            VDK_VERSION_AT_LEAST(VDK_VERSION_MAJOR, VDK_VERSION_MINOR,
                VDK_VERSION_PATCH));
        check("version", "and at least any earlier one",
            VDK_VERSION_AT_LEAST(11, 2, 0) && VDK_VERSION_AT_LEAST(12, 0, 0)
            && VDK_VERSION_AT_LEAST(0, 99, 99));
        check("version", "but not a later patch, minor or major",
            !VDK_VERSION_AT_LEAST(VDK_VERSION_MAJOR, VDK_VERSION_MINOR,
                VDK_VERSION_PATCH + 1)
            && !VDK_VERSION_AT_LEAST(VDK_VERSION_MAJOR,
                VDK_VERSION_MINOR + 1, 0)
            && !VDK_VERSION_AT_LEAST(VDK_VERSION_MAJOR + 1, 0, 0));
    }

    /* ---- a popup's parts follow the popup ---- */

    popup = vk_popup_create(40, 9, VK_BORDER_SINGLE, "Okay", "Cancel", NULL);
    client = vk_label_create(30);
    vk_label_set_text(client, "a message");
    vk_popup_set_client(popup, VK_WIDGET(client));
    vk_widget_set_colors(VK_WIDGET(popup), COLOR_WHITE, COLOR_BLUE);
    vk_popup_redraw(popup);

    button = vk_popup_get_button(popup, 0);
    bar = vk_popup_get_button_bar(popup);
    check("inherit", "colouring a popup colours its button bar and buttons",
        colours(bar, COLOR_WHITE, COLOR_BLUE)
        && colours(button, COLOR_WHITE, COLOR_BLUE));
    check("inherit", "and the client it was given",
        colours(client, COLOR_WHITE, COLOR_BLUE));

    /* the popup's own colour calls still decide for the parts they name */
    vk_popup_set_button_colors(popup, COLOR_YELLOW, COLOR_RED);
    vk_popup_redraw(popup);
    check("inherit", "vk_popup_set_button_colors still wins for the buttons",
        colours(button, COLOR_YELLOW, COLOR_RED)
        && colours(client, COLOR_WHITE, COLOR_BLUE));

    vk_popup_destroy(popup);

    vk_screen_destroy(screen);

    printf("%d of %d passed\n", passed, passed + failed);

    return (failed == 0) ? 0 : 1;
}
