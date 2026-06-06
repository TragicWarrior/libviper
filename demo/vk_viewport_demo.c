/*
    vk_viewport_demo
    ----------------
    Minimal sanity demo for the vk_viewport widget.

    Synthesizes a logical canvas of 50 rows by 200 cols filled with
    coordinate labels, and shows it through a 15x60 viewport you can
    scroll with arrow keys, Page Up / Page Down, Home, End, or 'g'
    (jump to top) and 'G' (jump to bottom).  'q' or Esc quits.

    No other widget needs to know how the canvas is laid out -- the
    viewport just asks the source callback for one row at a time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "vdk.h"

#define CANVAS_ROWS     50
#define CANVAS_COLS     200

#define VP_ROWS         15
#define VP_COLS         60

/*
    The "logical canvas" here is computed on the fly -- nothing is
    actually stored.  Each cell shows a single character of the
    string "[rRR,cCCC]" laid out across columns so the user can see
    exactly which (row, col) region the viewport is showing.
*/
static int
demo_get_row(void *anything, int row, int col, cchar_t *out, int max_cols)
{
    char        line[CANVAS_COLS + 1];
    int         c;
    int         i;
    wchar_t     wch[2];
    short       pair;

    (void)anything;

    if(row < 0 || row >= CANVAS_ROWS) return 0;

    /*
        Fill the whole logical row with a repeating pattern that
        encodes the row number; each block of 10 cols carries
        "[rNN,cMMM] " (11 chars repeating).  This is purely for the
        demo, so it's fine to be hokey.
    */
    for(c = 0; c < CANVAS_COLS; c += 11)
    {
        int remaining = CANVAS_COLS - c;
        int n = (remaining < 11) ? remaining : 11;
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "[r%02d,c%03d] ", row, c);
        memcpy(&line[c], tmp, n);
    }
    line[CANVAS_COLS] = '\0';

    /* Pick a color for this row so scrolling is visually obvious. */
    pair = vdk_color_pair(
        (row % 7) + 1,                /* fg: cycle through 1..7 */
        COLOR_BLACK);

    for(i = 0; i < max_cols; i++)
    {
        int src_col = col + i;
        char ch;

        if(src_col < 0 || src_col >= CANVAS_COLS)
        {
            wch[0] = L' ';
        }
        else
        {
            ch = line[src_col];
            wch[0] = (wchar_t)ch;
        }
        wch[1] = L'\0';

        setcchar(&out[i], wch, A_NORMAL, pair, NULL);
    }

    return max_cols;
}

int
main(void)
{
    vk_screen_t             *screen;
    vk_window_t             *window;
    vk_viewport_t           *vp;
    vk_scroller_t           *vscr;
    vk_scroller_t           *hscr;
    vk_viewport_src_t    src;
    int                     max_y, max_x;
    int32_t                 key;
    int                     border = 2;     /* window border */

    screen = vk_screen_create();
    if(screen == NULL)
    {
        fprintf(stderr, "vk_screen_create failed\n");
        return 1;
    }

    vdk_color_init();

    getmaxyx(vk_screen_get_window(screen), max_y, max_x);

    if(max_x < VP_COLS + border + 2 || max_y < VP_ROWS + border + 2)
    {
        vk_screen_destroy(screen);
        fprintf(stderr,
            "Terminal too small (need %dx%d, have %dx%d)\n",
            VP_COLS + border + 2, VP_ROWS + border + 2, max_x, max_y);
        return 1;
    }

    /* Window framing the viewport. */
    window = vk_window_create(VP_COLS + 2 + 1, VP_ROWS + 2 + 1);
    vk_window_set_title(window, " vk_viewport demo ");
    vk_window_set_border_colors(window, COLOR_CYAN, COLOR_BLACK);
    vk_window_set_border_attrs(window, A_BOLD);

    /* The viewport itself. */
    vp = vk_viewport_create(VP_COLS, VP_ROWS);

    src.get_row = demo_get_row;
    src.rows    = CANVAS_ROWS;
    src.cols    = CANVAS_COLS;
    src.anything = NULL;
    vk_viewport_set_src(vp, &src);

    vk_window_set_child(window, VK_WIDGET(vp));

    /* Vertical + horizontal scrollers that drive off the viewport. */
    vscr = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(vscr, VK_BORDER_SINGLE);
    vk_scroller_set_border_colors(vscr, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_set_scroll_source(vscr, VK_WIDGET(vp));
    vk_scroller_set_scroll_info(vscr, vk_viewport_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(vp), vscr);

    hscr = vk_scroller_create(VK_SCROLLBAR_HORIZONTAL);
    vk_scroller_set_border_style(hscr, VK_BORDER_SINGLE);
    vk_scroller_set_border_colors(hscr, COLOR_CYAN, COLOR_BLACK);
    vk_scroller_set_scroll_source(hscr, VK_WIDGET(vp));
    vk_scroller_set_scroll_info(hscr, vk_viewport_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(window), hscr);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(window));
    vk_widget_move(VK_WIDGET(window),
        (max_x - (VP_COLS + 3)) / 2,
        (max_y - (VP_ROWS + 3)) / 2);

    vk_viewport_update(vp);
    vk_window_update(window);
    vk_screen_refresh(screen);

    for(;;)
    {
        key = getch();

        if(key == 'q' || key == 'Q' || key == 27) break;

        switch(key)
        {
            case KEY_UP:    vk_viewport_scroll_by(vp, -1, 0); break;
            case KEY_DOWN:  vk_viewport_scroll_by(vp,  1, 0); break;
            case KEY_LEFT:  vk_viewport_scroll_by(vp,  0, -1); break;
            case KEY_RIGHT: vk_viewport_scroll_by(vp,  0,  1); break;
            case KEY_PPAGE: vk_viewport_pgup(vp); break;
            case KEY_NPAGE: vk_viewport_pgdn(vp); break;
            case KEY_HOME:
            case 'g':       vk_viewport_set_scroll(vp, 0, 0); break;
            case KEY_END:
            case 'G':       vk_viewport_set_scroll(vp, CANVAS_ROWS, 0); break;
            default:
                continue;
        }

        vk_viewport_update(vp);
        vk_window_update(window);
        vk_screen_refresh(screen);
    }

    vk_screen_destroy(screen);
    return 0;
}
