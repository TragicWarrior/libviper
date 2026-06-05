#include <stdlib.h>
#include <limits.h>

#include "vdk.h"

static void
_vdk_color_init_extended(void)
{
    int     fg, bg;
    int     fg_half;
    int     pair_idx;

    for(fg = 0; fg < 256; fg += 2)
    {
        fg_half = fg >> 1;

        for(bg = 0; bg < 256; bg++)
        {
            pair_idx = 64 + (bg * 128) + fg_half;

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
}

short
vdk_color_pair(short fg, short bg)
{
    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    if(fg > 7 || bg > 7)
    {
        int idx = 64 + (bg * 128) + (fg >> 1);
        if(idx > 0 && idx <= SHRT_MAX) return (short)idx;
        return 0;
    }

    return (bg * VDK_COLOR_COUNT) + (VDK_COLOR_COUNT - fg - 1);
}
