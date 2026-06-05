#ifndef _VDK_COLOR_H_
#define _VDK_COLOR_H_

#include <ncursesw/curses.h>

#define VDK_COLOR_COUNT     8

void    vdk_color_init(void);

static inline short
vdk_color_pair(short fg, short bg)
{
    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    return (bg * VDK_COLOR_COUNT) + (VDK_COLOR_COUNT - fg - 1);
}

#define VDK_COLORS(fg, bg)  (COLOR_PAIR(vdk_color_pair(fg, bg)))

#endif
