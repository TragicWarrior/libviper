/*
    test_colors -- headless checks of vdk_color_pair() on terminals with
    8 and with 256 colours.

    On 8 colours a bright colour (8 - 15) has no pair of its own, so it
    is drawn as its plain counterpart; two colours asked for as different
    stay different, and two asked for as the same stay the same.  The
    colour picker's bright swatches, which have no bright background to
    use, are drawn as bold solid blocks in the plain colour.  On 256
    colours nothing is folded and the swatches are bright backgrounds.  Each terminal type gets its own process,
    because a detached screen is made once per program.

    Exit status 0 when every check passes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <locale.h>

#include "vdk.h"

static int  passed = 0;
static int  failed = 0;

/* check that fg on bg is drawn with the pair for want_fg on want_bg */
static void
check(short fg, short bg, short want_fg, short want_bg)
{
    short   pair = vdk_color_pair(fg, bg);
    short   got_fg = -1;
    short   got_bg = -1;
    int     ok;

    pair_content(pair, &got_fg, &got_bg);
    ok = (got_fg == want_fg && got_bg == want_bg);

    if(ok) passed++;
    else failed++;

    printf("%s  %s  %2d on %2d -> pair %3d = %d on %d (want %d on %d)\n",
        ok ? "PASS" : "FAIL", getenv("TERM"), fg, bg, pair,
        got_fg, got_bg, want_fg, want_bg);
}

/* record a check that is not about a single pair */
static void
report(int ok, const char *what, int value)
{
    if(ok) passed++;
    else failed++;

    printf("%s  %s  %s (%#x)\n", ok ? "PASS" : "FAIL", getenv("TERM"),
        what, value);
}

/*
    draw a colour picker and sort its cells: which colours its bold
    solid-block cells are drawn in, and which bright backgrounds appear
*/
static void
scan_picker(int *block_fgs, int *bright_bgs)
{
    vk_color_t  *picker;
    WINDOW      *canvas;
    wchar_t     block[CCHARW_MAX];
    attr_t      attrs;
    short       pair;
    int         w, h, x, y;

    *block_fgs = 0;
    *bright_bgs = 0;

    // the full block, not ncurses' BLOCK (U+25AE on UTF-8 terminals)
    block[0] = 0x2588;

    picker = vk_color_create(33, 5, 8, 2, VK_BORDER_NONE);
    vk_color_redraw(picker);
    canvas = vk_widget_get_canvas(VK_WIDGET(picker));
    getmaxyx(canvas, h, w);

    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
        {
            cchar_t     cc;
            wchar_t     wch[CCHARW_MAX];
            short       fg, bg;

            mvwin_wch(canvas, y, x, &cc);
            getcchar(&cc, wch, &attrs, &pair, NULL);
            pair_content(pair, &fg, &bg);

            // a bold block: a bright swatch drawn in its plain colour
            if(wch[0] == block[0] && (attrs & A_BOLD) && fg >= 0 && fg < 8)
                *block_fgs |= (1 << fg);

            if(bg >= 8 && bg <= 15) *bright_bgs |= (1 << (bg - 8));
        }
    }

    vk_color_destroy(picker);
}

/* the checks for one terminal type, run in a process of its own */
static int
run(const char *term)
{
    vk_screen_t     *screen;
    int             block_fgs;
    int             bright_bgs;

    setenv("TERM", term, 1);
    setlocale(LC_ALL, "");

    screen = vk_screen_create_detached(80, 24);
    if(screen == NULL)
    {
        printf("FAIL  %s: could not create a detached screen\n", term);
        return 1;
    }

    vdk_color_init();

    if(COLORS < 16)
    {
        // the host label case: bright white on magenta
        check(15, COLOR_MAGENTA, COLOR_WHITE, COLOR_MAGENTA);
        // a bright background folds too
        check(COLOR_WHITE, 12, COLOR_WHITE, COLOR_BLUE);
        // plain colours are untouched
        check(COLOR_CYAN, COLOR_BLUE, COLOR_CYAN, COLOR_BLUE);
        check(COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE);
        // different colours that fold into one: the text still shows
        check(15, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE);
        check(12, COLOR_BLUE, COLOR_WHITE, COLOR_BLUE);
        check(COLOR_CYAN, 14, COLOR_BLACK, COLOR_CYAN);
        // bright black on black stays readable (white on black: pair 0)
        check(8, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK);
        // the same colour asked for twice stays hidden
        check(15, 15, COLOR_WHITE, COLOR_WHITE);

        // the picker's bright swatches: bold blocks in all 8 colours
        scan_picker(&block_fgs, &bright_bgs);
        report(block_fgs == 0xFF && bright_bgs == 0,
            "picker: bright swatches are bold blocks in each plain colour",
            block_fgs);
    }
    else
    {
        // 256 colours: the bright pair exists and is used as it is
        check(15, COLOR_MAGENTA, 15, COLOR_MAGENTA);
        check(12, COLOR_BLUE, 12, COLOR_BLUE);
        check(COLOR_CYAN, COLOR_BLUE, COLOR_CYAN, COLOR_BLUE);

        // the picker's bright swatches are bright backgrounds, no blocks
        scan_picker(&block_fgs, &bright_bgs);
        report(block_fgs == 0 && bright_bgs == 0xFF,
            "picker: bright swatches are bright backgrounds",
            bright_bgs);
    }

    vk_screen_destroy(screen);

    printf("%s: %d passed, %d failed\n", term, passed, failed);

    return failed == 0 ? 0 : 1;
}

int
main(void)
{
    const char  *terms[] = { "linux", "xterm-256color" };
    int         result = 0;
    int         i;

    for(i = 0; i < 2; i++)
    {
        pid_t   pid = fork();
        int     status = 1;

        if(pid == 0) exit(run(terms[i]));
        if(pid < 0 || waitpid(pid, &status, 0) < 0) return 1;

        if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }

    return result;
}
