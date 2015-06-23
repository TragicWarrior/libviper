#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "priv/klass_screen.h"

#include "vdk_object.h"
#include "vdk_context.h"
#include "vdk_screen.h"

static int
_vdk_screen_ctor(vdk_object_t *self,va_list *argp,...);

static int
_vdk_screen_dtor(vdk_object_t *self);

static int
_vdk_screen_move(vdk_object_t *self,int x,int y);

static int
_vdk_screen_resize(vdk_object_t *self,int width,int height);

static int
_vdk_screen_blit(vdk_context_t *self,vdk_context_t *target);

static void
_vdk_screen_update_metrics(vdk_object_t *object);

static void
_vdk_screen_init_colors_fast(vdk_screen_t *screen);

static void
_vdk_screen_init_colors_safe(vdk_screen_t *screen);


static vdk_object_t _VDK_Screen_Klass =
{
    .klass_name = "VDK_SCREEN",
    .size = sizeof(vdk_screen_t),
    .ctor = _vdk_screen_ctor,
    .dtor = _vdk_screen_dtor,
    .move = _vdk_screen_move,
    .resize = _vdk_screen_resize,
};

const void  *VDK_SCREEN_KLASS = &_VDK_Screen_Klass;



vdk_screen_t*
vdk_screen_create(char *term,FILE *f_in,FILE *f_out)
{
    vdk_screen_t    *screen;

    screen = (vdk_screen_t*)vdk_object_create(VDK_SCREEN_KLASS,
        term,f_in,f_out);

    return screen;
}

void
vdk_screen_init_color(vdk_screen_t *screen,bool fast)
{
    if(screen == NULL) return;

    // check to see if this screen (term) has already been colorized
    if(screen->state & VDK_SCREEN_COLORIZED) return;

    // check to see if this screen is active
    if(screen->state & VDK_SCREEN_PAUSED) return;

    start_color();

    if(fast == TRUE)
    {
        _vdk_screen_init_colors_fast(screen);
        screen->state |= VDK_SCREEN_FAST_COLOR;
    }
    else
    {
        _vdk_screen_init_colors_safe(screen);
        screen->state &= ~VDK_SCREEN_FAST_COLOR;
    }

    screen->state |= VDK_SCREEN_COLORIZED;

    return;
}

short
vdk_screen_get_color_pair(vdk_screen_t *screen,short fg,short bg)
{
    short       color_pair;
    short       fg_color;
    short       bg_color;
    short       i;

    if(screen == NULL) return -1;

    if(fg == COLOR_WHITE && bg == COLOR_BLACK) return 0;

    // use fast color indexing when possible
    if(screen->state & VDK_SCREEN_FAST_COLOR)
    {
        color_pair = (bg * COLORS) + (COLORS - fg -1);
        return color_pair;
    }

    // safe color indexing (slower)
    for(i = 1;i < COLOR_PAIRS;i++)
    {
        pair_content(i,&fg_color,&bg_color);
        if(fg_color == fg && bg_color == bg) break;
    }

    return i;
}

int
vdk_screen_set_colors(vdk_screen_t *screen,short color_pair)
{
    if(screen == NULL) return -1;
    if(screen->state & VDK_SCREEN_PAUSED) return -1;
    if(!(screen->state & VDK_SCREEN_COLORIZED)) return -1;
    if(color_pair < 0) return -1;

    wbkgdset(stdscr,COLOR_PAIR(color_pair));
    wcolor_set(stdscr,color_pair,NULL);
    pair_content(color_pair,&screen->fg,&screen->bg);

    return 0;
}

void
vdk_screen_clear(vdk_screen_t *screen)
{
    if(screen == NULL) return;
    if(screen->state & VDK_SCREEN_PAUSED) return;

    werase(stdscr);

    return;
}

int
vdk_screen_draw(vdk_screen_t *screen)
{
    int     retval;

    if(screen == NULL) return -1;

    // check to see if screen is paused
    retval = _vdk_screen_blit(VDK_CONTEXT(screen),NULL);

    return retval;
}

short
vdk_screen_get_fg(vdk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return screen->fg;
}

short
vdk_screen_get_bg(vdk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return screen->bg;
}

int
vdk_screen_get_height(vdk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return VDK_RASTER(screen)->height;
}

int
vdk_screen_get_width(vdk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return VDK_RASTER(screen)->width;
}

void
vdk_screen_fill(vdk_screen_t *screen,chtype ch)
{
    vdk_raster_t    *raster;
    long            i;

    if(screen == NULL) return;
    if(screen->state & VDK_SCREEN_PAUSED) return;

    raster = VDK_RASTER(screen);

    i = raster->height * raster->width;

    while(i)
    {
        waddch(stdscr,ch);
        i--;
    }

    wmove(stdscr,0,0);

    return;
}

static int
_vdk_screen_ctor(vdk_object_t *object,va_list *argp,...)
{
    va_list         args;
    char            *term;
    FILE            *f_in;
    FILE            *f_out;
    int             retval = 0;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args,argp);
        argp = &args;
    }

    term = va_arg(argp,char *);
    f_in = va_arg(argp,FILE *);
    f_out = va_arg(argp,FILE *);

    if(argp == &args) va_end(args);

    // use the template of the parent klass to invoke the base constructor
    VDK_OBJECT(VDK_CONTEXT_KLASS)->ctor(object,NULL);

    // rasterize the object
    retval = vdk_object_rasterize(object,
        VDK_RASTER_SCR,term,f_in,f_out,0,0);

    VDK_CONTEXT(object)->blit = _vdk_screen_blit;

    VDK_SCREEN(object)->fg = -1;
    VDK_SCREEN(object)->bg = -1;

    /*
        the first call to refresh() erases the screen so let's get it
        over with.
    */
    refresh();

    // store the screen dimenions
    _vdk_screen_update_metrics(object);

    return retval;
}

int
vdk_screen_pause(vdk_screen_t *screen)
{
    int             raster_type;

    if(screen == NULL) return -1;
    raster_type = vdk_object_get_raster_type(VDK_OBJECT(screen));

    if(raster_type != VDK_RASTER_SCR) return -1;

    // check to see if screen has already been paused
    if(screen->state & VDK_SCREEN_PAUSED) return -1;

    endwin();
    screen->state |= VDK_SCREEN_PAUSED;

    return 0;
}

int
vdk_screen_resume(vdk_screen_t *screen)
{
    int             raster_type;

    if(screen == NULL) return -1;
    raster_type = vdk_object_get_raster_type(VDK_OBJECT(screen));

    if(raster_type != VDK_RASTER_SCR) return -1;

    // check to see if screen is already active
    if(!(screen->state & VDK_SCREEN_PAUSED)) return -1;

    screen->state &= ~VDK_SCREEN_PAUSED;
    set_term(*(SCREEN**)screen);
    refresh();

    _vdk_screen_update_metrics(VDK_OBJECT(screen));

    return 0;
}


int
vdk_screen_destroy(vdk_screen_t *screen)
{
    vdk_object_t    *object;
    int             retval;

    if(screen == NULL) return -1;

    // require that a screen be paused first
    if(!(screen->state & VDK_SCREEN_PAUSED)) return -1;

    object = VDK_OBJECT(screen);
    retval = object->dtor(object);

    return retval;
}


static int
_vdk_screen_blit(vdk_context_t *self,vdk_context_t *target)
{
    vdk_raster_t    *raster;
    int             raster_type;

    if(self == NULL) return -1;

    raster_type = vdk_object_get_raster_type(VDK_OBJECT(self));
    if(raster_type != VDK_RASTER_SCR) return -1;

    raster = VDK_RASTER(self);

    if(raster->primitive.screen == NULL) return -1;

    wnoutrefresh(stdscr);
    doupdate();

    _vdk_screen_update_metrics(VDK_OBJECT(self));

    return 0;
}

static int
_vdk_screen_dtor(vdk_object_t *object)
{
    vdk_raster_t    *raster;
    int             raster_type;
    int             retval;

    if(object == NULL) return -1;

    raster_type = vdk_object_get_raster_type(VDK_OBJECT(object));
    if(raster_type != VDK_RASTER_SCR) return -1;

    raster = VDK_RASTER(object);

    // in order to tear down the screen it must first be activated
    set_term(raster->primitive.screen);
    endwin();
    delscreen(raster->primitive.screen);

    // call the parent klass destructor
    retval = VDK_OBJECT(VDK_CONTEXT_KLASS)->dtor(object);

    return retval;
}

static int
_vdk_screen_move(vdk_object_t *self,int x,int y)
{
    /*
        a screen cannot be moved.  however, to keep the code from blowing
        up we will stub out a callback and throw a soft error.
    */

    return -1;
}

static int
_vdk_screen_resize(vdk_object_t *self,int width,int height)
{
    /*
        a screen cannot be resized by the user.  however, it can be resized
        by Xterm and others which will cause a SIGWINCH.  if nothing else,
        it is a good time to ask curses about the stdscr.
    */

    _vdk_screen_update_metrics(self);

    return 0;
}

static void
_vdk_screen_update_metrics(vdk_object_t *object)
{
    vdk_raster_t    *raster;

    if(object == NULL) return;

    raster = VDK_RASTER(object);

    getmaxyx(stdscr,raster->height,raster->width);

    return;
}

static void
_vdk_screen_init_colors_fast(vdk_screen_t *screen)
{
    int     max_colors;
    int     bg;
    int     fg;
    int     i;

    if(screen == NULL) return;

    // strange but not forbidden for the user to re-init colors in fast mode
    if(screen->matrix != NULL)
    {
        free(screen->matrix);
        return;
    }

    max_colors = (COLORS * COLORS);

    if(max_colors > COLOR_PAIRS) max_colors = COLOR_PAIRS;

    /*
        in order for fast color indexing to work properly, VDK must assume
        that COLOR_BLACK is always 0 and COLOR_WHITE is always 7.  if this is
        not true, we have to fall back to safe color.
    */
    for(i = 1;i < max_colors;i++)
    {
        bg = i / COLORS;
        fg = COLORS - (i % COLORS) - 1;
        init_pair(i,fg,bg);
    }

    return;
}

static void
_vdk_screen_init_colors_safe(vdk_screen_t *screen)
{
    int     hard_pair = -1;
    int     fg;
    int     bg;
    int     i;

    if(screen == NULL) return;

    // allow the user to re-init the color matrix.  odd but not forbidden.
    if(screen->matrix != NULL)
    {
        free(screen->matrix);
        screen->matrix = NULL;
    }

    screen->matrix = (struct _vdk_color_mtx_s*)calloc(COLOR_PAIRS,
        sizeof(struct _vdk_color_mtx_s));

    for(i = 0;i < COLOR_PAIRS;i++)
    {
        screen->matrix[i].fg = i / COLORS;
        screen->matrix[i].bg = i % COLORS;
        /*
            according to curses documentation, color pair 0 is assumed to
            be WHITE foreground on BLACK background.  when we discover
            this pair, we need to make sure it gets swapped into
            index 0 and whatever is in index 0 gets put into this location.
        */
        if(screen->matrix[i].fg == COLOR_WHITE)
        {
            if(screen->matrix[i].bg == COLOR_BLACK)
            {
                hard_pair = i;
            }
        }
    }

    /*
        if hard_pair is no longer -1 then we found the "hard pair" during
        our enumeration process and we need to do the swap.
    */
    if(hard_pair != -1)
    {
        fg = screen->matrix[0].fg;
        bg = screen->matrix[0].bg;
        screen->matrix[hard_pair].fg = fg;
        screen->matrix[hard_pair].bg = bg;
    }

    for(i = 1;i < COLOR_PAIRS;i++)
    {
        init_pair(i,screen->matrix[i].fg,screen->matrix[i].bg);
    }

    return;
}
