#include <stdlib.h>

#include "vdk_color.h"

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
    }

    // pair 0 can't be init_pair'd so white-on-black (which maps to 0)
    // is unreachable.  steal the green-on-black slot for white-on-black.
    hard_pair = vdk_color_pair(COLOR_GREEN, COLOR_BLACK);
    if(hard_pair > 0)
    {
        matrix[hard_pair].fg = COLOR_WHITE;
        matrix[hard_pair].bg = COLOR_BLACK;
    }

    for(i = 1; i < max_colors; i++)
        init_pair(i, matrix[i].fg, matrix[i].bg);

    free(matrix);
}
