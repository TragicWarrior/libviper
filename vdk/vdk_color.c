#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "vdk.h"
#include "vdk_private.h"

static void
_vdk_color_init_extended(void)
{
    int     fg, bg;
    int     pair_idx;

    for(fg = 0; fg < 16; fg++)
    {
        for(bg = 0; bg < 16; bg++)
        {
            if(fg <= 7 && bg <= 7) continue;

            pair_idx = 64 + (bg * 16) + fg;
            init_pair(pair_idx, fg, bg);
        }
    }

    for(fg = 0; fg < 256; fg += 2)
    {
        for(bg = 0; bg < 256; bg++)
        {
            pair_idx = 320 + (bg * 128) + (fg >> 1);

            if(pair_idx <= 0 || pair_idx > SHRT_MAX) continue;

            init_pair(pair_idx, fg, bg);
        }
    }
}

void
vdk_color_init(void)
{
    short   fg, bg;
    int     i;
    int     max_colors;
    int     hard_pair = -1;

    struct { short fg; short bg; } *matrix;

    start_color();

    max_colors = VDK_COLOR_COUNT * VDK_COLOR_COUNT;

    matrix = calloc(max_colors, sizeof(*matrix));
    if(matrix == NULL) return;

    for(i = 0; i < max_colors; i++)
    {
        fg = i / VDK_COLOR_COUNT;
        bg = VDK_COLOR_COUNT - (i % VDK_COLOR_COUNT) - 1;

        matrix[i].bg = fg;
        matrix[i].fg = bg;

        if(matrix[i].fg == COLOR_WHITE && matrix[i].bg == COLOR_BLACK)
            hard_pair = i;
    }

    if(hard_pair > 0)
    {
        matrix[hard_pair].fg = matrix[0].fg;
        matrix[hard_pair].bg = matrix[0].bg;
    }

    for(i = 1; i < max_colors; i++)
        init_pair(i, matrix[i].fg, matrix[i].bg);

    free(matrix);

    if(COLORS >= 256) _vdk_color_init_extended();

    /* nothing has redefined a colour yet on this SCREEN: record what
       ncurses says each one is (see vdk_cells.c) */
    _vdk_cells_colors_baseline();
}

/*
    on a terminal with only 8 colours a bright colour (8 - 15) has no
    pair of its own, and a pair asked for with one came out white on
    black.  Use its plain counterpart instead.  When that makes two
    colours asked for as different the same, the text becomes black or
    white, whichever shows against the background; colours asked for as
    the same stay the same, which is how text is hidden on purpose.
*/
static void
_vdk_color_fold(short *fg, short *bg)
{
    short   f = *fg;
    short   b = *bg;

    if(f >= 8 && f <= 15) f -= 8;
    if(b >= 8 && b <= 15) b -= 8;

    // two different colours became one:  make the text show
    if(f == b && *fg != *bg)
    {
        f = (b == COLOR_YELLOW || b == COLOR_CYAN || b == COLOR_WHITE)
            ? COLOR_BLACK : COLOR_WHITE;
    }

    *fg = f;
    *bg = b;
}

short
vdk_color_pair(short fg, short bg)
{
    // a terminal with 8 colours: fold the bright ones it lacks
    if(COLORS >= 8 && COLORS < 16) _vdk_color_fold(&fg, &bg);

    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    if(fg <= 7 && bg <= 7)
        return (bg * VDK_COLOR_COUNT) + (VDK_COLOR_COUNT - fg - 1);

    if(fg <= 15 && bg <= 15)
        return 64 + (bg * 16) + fg;

    {
        int idx = 320 + (bg * 128) + (fg >> 1);
        if(idx > 0 && idx <= SHRT_MAX) return (short)idx;
        return 0;
    }
}
