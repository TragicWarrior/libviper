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

        if(matrix[i].fg == COLOR_WHITE && matrix[i].bg == COLOR_BLACK)
            hard_pair = i;
    }

    if(hard_pair != -1)
    {
        fg = matrix[0].fg;
        bg = matrix[0].bg;
        matrix[hard_pair].fg = fg;
        matrix[hard_pair].bg = bg;
    }

    for(i = 1; i < max_colors; i++)
        init_pair(i, matrix[i].fg, matrix[i].bg);

    free(matrix);
}
