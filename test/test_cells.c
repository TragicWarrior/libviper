/*
    test_cells -- headless checks of vdk_cells_read_row() and its
    vk_widget / vk_screen wrappers.

    Cells are written into a canvas with known characters, attributes
    and colours, read back as portable cells, and compared with what
    they should come back as.  Needs no terminal: the screen is created
    detached, on /dev/null.

    Exit status 0 when every check passes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

#include "vdk.h"

/* pair numbers borrowed for the test: see where they are defined */
#define PAIR_PALETTE        30000
#define PAIR_REDEFINED      30001
#define PAIR_DEFAULTS       30002

static int  passed = 0;
static int  failed = 0;

/* record one check, showing the cell it was made on */
static void
check(const char *what, int ok, const vdk_cell_t *cell)
{
    if(ok) passed++;
    else failed++;

    printf("%s  %-50s U+%04X fg=%06X bg=%06X attrs=%u flags=%u\n",
        ok ? "PASS" : "FAIL", what, (unsigned)cell->ch,
        (unsigned)cell->fg, (unsigned)cell->bg,
        (unsigned)cell->attrs, (unsigned)cell->flags);
}

/* put one character at column x of row 0 with the given attributes and
   pair.  the pair goes in as an int so that numbers beyond a short work */
static void
put(WINDOW *win, int x, chtype ch, attr_t attrs, int pair)
{
    wattr_set(win, attrs, 0, &pair);
    mvwaddch(win, 0, x, ch);
}

int
main(void)
{
    vk_screen_t     *screen;
    vk_widget_t     *widget;
    WINDOW          *canvas;
    vdk_cell_t      row[40];
    vdk_cell_t      shown[40];
    cchar_t         cc;
    wchar_t         wide[2] = { 0x65E5, 0 };    /* a double-width character */
    int             n;

    /* a terminal type with the full 256 colours, whatever ran the test */
    setenv("TERM", "xterm-256color", 1);
    setlocale(LC_ALL, "");

    screen = vk_screen_create_detached(80, 24);
    if(screen == NULL)
    {
        printf("FAIL  could not create a detached screen\n");
        return 1;
    }

    vdk_color_init();
    use_default_colors();

    widget = vk_widget_create(40, 4);
    canvas = vk_widget_get_canvas(widget);

    /*
        Three pairs of our own.  They take over numbers vdk_color_init
        already made (far up its 256-colour block) instead of adding new
        ones past the end: on the ncurses this was written against,
        growing the pair table after tens of thousands of pairs exist
        crashes inside its own lookup tree.  That also means a pair
        number too big for a short cannot be exercised here.
    */
    init_extended_pair(PAIR_PALETTE, 196, 21);  /* untouched palette colours */
    init_extended_color(110, 1000, 500, 0);     /* a redefined colour        */
    init_extended_pair(PAIR_REDEFINED, 110, 0);
    init_extended_pair(PAIR_DEFAULTS, -1, -1);  /* both defaults             */

    put(canvas, 0, 'A', A_NORMAL, PAIR_DEFAULTS);
    put(canvas, 1, 'B', A_NORMAL, PAIR_PALETTE);
    put(canvas, 2, 'C', A_BOLD, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    put(canvas, 3, 'D', A_NORMAL, PAIR_REDEFINED);
    put(canvas, 4, 'E', A_REVERSE, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    put(canvas, 5, 'F', A_DIM, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    put(canvas, 6, 'G', A_UNDERLINE, PAIR_DEFAULTS);
    put(canvas, 7, ACS_HLINE, A_NORMAL, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    setcchar(&cc, wide, A_NORMAL, vdk_color_pair(COLOR_RED, COLOR_BLUE), NULL);
    mvwadd_wch(canvas, 0, 9, &cc);
    put(canvas, 11, 'H', A_REVERSE, PAIR_DEFAULTS);
    put(canvas, 12, 'I', A_NORMAL, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    put(canvas, 13, 'J', A_NORMAL, vdk_color_pair(12, 9));

    n = vdk_cells_read_row(canvas, 0, row, 40);
    check("a row is read whole", n == 40, &row[0]);

    check("default colours are flagged as defaults",
        row[0].ch == 'A' && row[0].fg == VDK_CELL_DEFAULT_FG
        && row[0].bg == VDK_CELL_DEFAULT_BG
        && row[0].flags == (VDK_CELL_FG_DEFAULT | VDK_CELL_BG_DEFAULT), &row[0]);
    check("colours 196 on 21 are the standard palette's",
        row[1].fg == 0xFF0000 && row[1].bg == 0x0000FF && row[1].flags == 0, &row[1]);
    check("basic colours, with bold kept",
        row[2].fg == 0xCD0000 && row[2].bg == 0x0000EE
        && row[2].attrs == VDK_CELL_BOLD, &row[2]);
    check("a redefined colour is what it was redefined to",
        row[3].fg == 0xFF7F00 && row[3].bg == 0x000000, &row[3]);
    check("reverse swaps the colours and leaves no attribute",
        row[4].fg == 0x0000EE && row[4].bg == 0xCD0000 && row[4].attrs == 0, &row[4]);
    check("dim moves the foreground halfway to the background",
        row[5].fg == 0x660077 && row[5].bg == 0x0000EE && row[5].attrs == 0, &row[5]);
    check("underline is kept",
        row[6].attrs == VDK_CELL_UNDERLINE, &row[6]);
    check("a line-drawing cell becomes U+2500",
        row[7].ch == 0x2500, &row[7]);
    check("a double-width character: first cell flagged",
        row[9].ch == 0x65E5 && (row[9].flags & VDK_CELL_WIDE), &row[9]);
    check("a double-width character: second cell empty",
        row[10].ch == 0 && (row[10].flags & VDK_CELL_WIDE_TAIL)
        && !(row[10].flags & VDK_CELL_WIDE) && row[10].bg == row[9].bg, &row[10]);
    check("reversed defaults: the flags swap with the colours",
        row[11].fg == VDK_CELL_DEFAULT_BG && row[11].bg == VDK_CELL_DEFAULT_FG
        && row[11].flags == (VDK_CELL_FG_DEFAULT | VDK_CELL_BG_DEFAULT), &row[11]);
    check("the cell after a double-width character reads normally",
        row[12].ch == 'I' && row[12].flags == 0, &row[12]);
    check("bright colours (12 on 9) from the 16x16 pairs",
        row[13].fg == 0x5C5CFF && row[13].bg == 0xFF0000, &row[13]);
    check("a cell never written is blank",
        row[20].ch == ' ', &row[20]);

    n = vdk_cells_read_row(canvas, 9, row, 40);
    check("a row that does not exist is refused", n == -1, &row[0]);
    n = vdk_cells_read_row(NULL, 0, row, 40);
    check("no window is refused", n == -1, &row[0]);

    /* a short window read into a longer row: the rest is blank */
    n = vdk_cells_read_row(canvas, 0, row, 40);
    {
        vdk_cell_t  wide_row[60];

        n = vdk_cells_read_row(canvas, 0, wide_row, 60);
        check("columns past the window's edge come back blank",
            n == 60 && wide_row[50].ch == ' '
            && (wide_row[50].flags & VDK_CELL_BG_DEFAULT), &wide_row[50]);
    }

    /* the widget wrapper reads the same thing */
    n = vk_widget_read_row(widget, 0, shown, 40);
    check("vk_widget_read_row matches the canvas",
        n == 40 && memcmp(shown, row, sizeof(row)) == 0, &shown[2]);

    /* a frozen widget shows its snapshot, not what it draws afterwards */
    vk_widget_set_state(widget,
        vk_widget_get_state(widget) | VK_STATE_FROZEN);
    put(canvas, 2, 'Z', A_NORMAL, vdk_color_pair(COLOR_RED, COLOR_BLUE));
    vk_widget_read_row(widget, 0, shown, 40);
    check("a frozen widget is read as it is being shown",
        shown[2].ch == 'C', &shown[2]);
    vk_widget_set_state(widget,
        vk_widget_get_state(widget) & ~VK_STATE_FROZEN);
    vk_widget_read_row(widget, 0, shown, 40);
    check("unfrozen, the new drawing is read",
        shown[2].ch == 'Z', &shown[2]);

    /* the screen wrapper reads the surface the widget is composed onto */
    vk_widget_set_state(widget,
        vk_widget_get_state(widget) | VK_STATE_VISIBLE);
    vk_widget_move(widget, 5, 3);
    vk_screen_attach_widget(screen, vk_screen_get_active_surface(screen),
        widget);
    vk_widget_draw(widget);
    vk_screen_refresh(screen);
    {
        vdk_cell_t  screen_row[80];

        n = vk_screen_read_row(screen, 3, screen_row, 80);
        check("vk_screen_read_row finds the widget where it was placed",
            n == 80 && screen_row[5 + 1].ch == 'B'
            && screen_row[5 + 1].fg == 0xFF0000, &screen_row[5 + 1]);
    }
    check("NULL widget and NULL screen are refused",
        vk_widget_read_row(NULL, 0, shown, 40) == -1
        && vk_screen_read_row(NULL, 0, shown, 40) == -1, &shown[0]);

    check("a cell is 16 bytes", sizeof(vdk_cell_t) == 16, &row[0]);

    vk_screen_destroy(screen);

    printf("%d of %d passed\n", passed, passed + failed);

    return (failed == 0) ? 0 : 1;
}
