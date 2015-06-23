#ifndef _VDK_SCREEN_H_
#define _VDK_SCREEN_H_

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

typedef struct _vdk_screen_s    vdk_screen_t;
#define VDK_SCREEN(x)           ((vdk_screen_t *)x)

#define VDK_SCREEN_PAUSED       (1UL << 0)
#define VDK_SCREEN_COLORIZED    (1UL << 1)
#define VDK_SCREEN_FAST_COLOR   (1UL << 2)


vdk_screen_t*   vdk_screen_create(char *term,FILE *f_in,FILE *f_out);

void            vdk_screen_init_color(vdk_screen_t *screen,bool fast);

short           vdk_screen_get_color_pair(vdk_screen_t *screen,
                    short fg,short bg);

int             vdk_screen_set_colors(vdk_screen_t *screen,short color_pair);

short           vdk_screen_get_fg(vdk_screen_t *screen);

short           vdk_screen_get_bg(vdk_screen_t *screen);

int             vdk_screen_get_height(vdk_screen_t *screen);

int             vdk_screen_get_width(vdk_screen_t *screen);

void            vdk_screen_clear(vdk_screen_t *screen);

void            vdk_screen_fill(vdk_screen_t *screen,chtype ch);

int             vdk_screen_pause(vdk_screen_t *screen);

int             vdk_screen_resume(vdk_screen_t *screen);

int             vdk_screen_draw(vdk_screen_t *screen);

int             vdk_screen_destroy(vdk_screen_t *screen);

#endif
