#include <string.h>
#include <inttypes.h>
#include "viper.h"
#include "private.h"
#include "viper_kmio.h"

static void viper_kmio_show_mouse(MEVENT *mouse_event);

void
viper_kmio_dispatch(int32_t keystroke, MEVENT *mouse_event)
{
    static vwnd_t           *event_wnd = NULL;
    vwnd_t                  *keystroke_wnd = NULL;
    static MEVENT           previous_mouse_event;
    static ViperWkeyFunc    func;
    static int              event_mode = 0;
    int                     beg_x,beg_y;
    int                     max_x,max_y;
    MEVENT                  *new_mouse = NULL;     /* strictly for      */
    MEVENT                  *old_mouse = NULL;     /* for readability   */
#if !defined(_NO_GPM) && defined(__linux)
	extern int        		gpm_fd;
#endif

    // invalid / no keystroke
    if(keystroke == KMIO_NONE) return;

    // the unmanaged window deck always has priority
    if(keystroke != KEY_RESIZE && keystroke != KEY_MOUSE)
    {
        keystroke_wnd = TOPMOST_UNMANAGED;
        func = viper_window_get_key_func(keystroke_wnd);

        if(func != NULL)
            keystroke = func(keystroke, (void*)keystroke_wnd);

        keystroke_wnd = NULL;
    }

    // keystroke was handled
    if(keystroke == KMIO_NONE || keystroke == KMIO_HANDLED)
    {
        return;
    }

    if(keystroke == KEY_RESIZE)
    {
        viper_event_run(VIPER_EVENT_BROADCAST, "term-resized");

        /* todo event handle for screen window instead  */
        viper_screen_redraw(CURRENT_SCREEN_ID,
            REDRAW_ALL | REDRAW_BACKGROUND);
    }

    if(keystroke == KEY_MOUSE && mouse_event != NULL)
    {
        new_mouse = mouse_event;
        old_mouse = &previous_mouse_event;

        // Shift+press: bring window to top then pass to app for selection
        if((new_mouse->bstate & BUTTON_SHIFT) &&
            (new_mouse->bstate & BUTTON1_PRESSED))
        {
            event_wnd = viper_deck_hit_test(-1, TRUE,
                new_mouse->x, new_mouse->y);

            if(event_wnd != NULL)
            {
                viper_window_set_top(event_wnd);
                viper_window_redraw(event_wnd);
                viper_screen_redraw(CURRENT_SCREEN_ID, REDRAW_ALL);
            }

            event_wnd = NULL;
            goto mouse_to_app;
        }

        // Shift with other mouse events: pass directly to app
        if(new_mouse->bstate & BUTTON_SHIFT)
            goto mouse_to_app;

        if((new_mouse->bstate & REPORT_MOUSE_POSITION)
            && event_mode == EVENTMODE_MOVE)
        {
            viper_mvwin_rel(event_wnd, new_mouse->x - old_mouse->x,
                new_mouse->y - old_mouse->y);
            memcpy(old_mouse, new_mouse, sizeof(MEVENT));
        }

        if((new_mouse->bstate & REPORT_MOUSE_POSITION)
            && event_mode == EVENTMODE_RESIZE)
        {
            viper_wresize_rel(event_wnd, new_mouse->x - old_mouse->x,
                new_mouse->y - old_mouse->y);
            memcpy(old_mouse, new_mouse, sizeof(MEVENT));
        }

        /* check for a button press and a window hit */
        if((new_mouse->bstate & BUTTON1_PRESSED) && event_mode == EVENTMODE_IDLE)
        {
            event_wnd = viper_deck_hit_test(-1, TRUE, new_mouse->x, new_mouse->y);
            if(event_wnd != NULL)
            {
                viper_window_set_top(event_wnd);
                viper_window_redraw(event_wnd);
                viper_screen_redraw(CURRENT_SCREEN_ID, REDRAW_ALL);

                memcpy(old_mouse, new_mouse, sizeof(MEVENT));
                getbegyx(WINDOW_FRAME(event_wnd), beg_y, beg_x);
                getmaxyx(WINDOW_FRAME(event_wnd), max_y, max_x);

                // only border clicks start move/resize
                if(new_mouse->y == beg_y ||
                    new_mouse->y == beg_y + max_y - 1 ||
                    new_mouse->x == beg_x ||
                    new_mouse->x == beg_x + max_x - 1)
                {
                    if(new_mouse->x == (beg_x + max_x - 1) &&
                        new_mouse->y == (beg_y + max_y) - 1)
                    {
                        event_mode = EVENTMODE_RESIZE;
                    }
                    else
                    {
                        event_mode = EVENTMODE_MOVE;
                    }
                }
                else
                {
                    event_wnd = NULL;
                    event_mode = EVENTMODE_IDLE;
                }
            }
            else event_mode = EVENTMODE_IDLE;
        }

        if(new_mouse->bstate & BUTTON1_RELEASED)
        {
            if(!(new_mouse->bstate & REPORT_MOUSE_POSITION))
            {
                if(event_mode == EVENTMODE_MOVE) viper_mvwin_rel(event_wnd,
                    new_mouse->x - old_mouse->x,new_mouse->y - old_mouse->y);

                /* resize window  */
                if(event_mode == EVENTMODE_RESIZE)
                {
                    viper_wresize_rel(event_wnd, new_mouse->x - old_mouse->x,
                        new_mouse->y - old_mouse->y);
                }

                viper_screen_redraw(CURRENT_SCREEN_ID, REDRAW_ALL);
            }

            event_wnd = NULL;
            event_mode = EVENTMODE_IDLE;
        }

        if(new_mouse->bstate & BUTTON1_CLICKED)
        {
            event_wnd = viper_deck_hit_test(-1, TRUE, new_mouse->x, new_mouse->y);
            if(event_wnd != NULL)
            {

                viper_window_set_top(event_wnd);
                viper_window_redraw(event_wnd);

                getbegyx(WINDOW_FRAME(event_wnd), beg_y, beg_x);
                getmaxyx(WINDOW_FRAME(event_wnd), max_y, max_x);
                if(new_mouse->x == (beg_x + max_x - 2) && new_mouse->y == beg_y)
                {
                    viper_window_close(event_wnd);
                    keystroke = -1;
                }
                if(new_mouse->x == (beg_x + max_x - 4) && new_mouse->y == beg_y)
                {
                    viper_window_hide(event_wnd);
                    viper_deck_cycle(-1, TRUE, VECTOR_BOTTOM_TO_TOP);
                    keystroke =- 1;
                }
            }
            event_wnd = NULL;
            event_mode = EVENTMODE_IDLE;
        }

        if(new_mouse->bstate & BUTTON1_DOUBLE_CLICKED)
        {
            event_wnd = viper_deck_hit_test(-1, TRUE, new_mouse->x, new_mouse->y);
            if(event_wnd != NULL)
            {
                viper_window_set_top(event_wnd);
                viper_window_redraw(event_wnd);
            }

            event_wnd = NULL;
            event_mode = EVENTMODE_IDLE;
        }

    mouse_to_app: ;
    }

    // pass keystroke on to toplevel *managed* window
    if(keystroke != KEY_RESIZE)
    {
        if(keystroke != KMIO_HANDLED && keystroke != KMIO_NONE)

        keystroke_wnd = TOPMOST_MANAGED;
        func = viper_window_get_key_func(keystroke_wnd);

        if(func != NULL)
            keystroke = func(keystroke, (void*)keystroke_wnd);
    }

    // draw mouse
#if !defined(_NO_GPM) && defined(__linux)
	if(gpm_fd > 0)
	{
   	    viper_kmio_show_mouse(new_mouse);
   	    viper_screen_redraw(CURRENT_SCREEN_ID, REDRAW_ALL);
	}
#endif
}

static void
viper_kmio_show_mouse(MEVENT *mouse_event)
{
    extern VIPER     *viper;
    WINDOW           *screen_window;
    static chtype    color;
    short            fg, bg;

    screen_window = CURRENT_SCREEN;

    if(viper->console_mouse == NULL)
    {
        viper->console_mouse = newwin(1, 1, 0, 0);
        color = mvwinch(screen_window, 0, 0);
        viper_pair_content(PAIR_NUMBER(color & A_COLOR), &fg, &bg);

        if(bg == COLOR_RED || bg == COLOR_YELLOW || bg == COLOR_MAGENTA)
            color = VIPER_COLORS(COLOR_CYAN,COLOR_CYAN);

        if(bg == COLOR_CYAN || bg == COLOR_BLUE)
            color = VIPER_COLORS(COLOR_YELLOW,COLOR_YELLOW);
    }

    if(mouse_event != NULL)
    {
        color = mvwinch(screen_window, mouse_event->y, mouse_event->x);
        viper_pair_content(PAIR_NUMBER(color & A_COLOR), &fg, &bg);

        if(bg == COLOR_RED || bg == COLOR_YELLOW || bg == COLOR_MAGENTA)
            color = VIPER_COLORS(COLOR_CYAN,COLOR_CYAN);
        else
            color = VIPER_COLORS(COLOR_YELLOW,COLOR_YELLOW);

        mvwin(viper->console_mouse, mouse_event->y, mouse_event->x);
    }

    mvwaddch(viper->console_mouse, 0, 0, ' ' | color);

    return;
}
