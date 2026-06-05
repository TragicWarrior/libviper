#include <stdlib.h>

#include "vdk.h"

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

    // pair 0 can't be init_pair'd — swap white-on-black into index 0
    // so it maps to the ncurses default pair.
    if(hard_pair > 0)
    {
        matrix[hard_pair].fg = matrix[0].fg;
        matrix[hard_pair].bg = matrix[0].bg;
    }

    for(i = 1; i < max_colors; i++)
        init_pair(i, matrix[i].fg, matrix[i].bg);

    free(matrix);
}

short
vdk_color_pair(short fg, short bg)
{
    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    return (bg * VDK_COLOR_COUNT) + (VDK_COLOR_COUNT - fg - 1);
}
