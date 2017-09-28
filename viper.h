#ifndef _VIPER_H
#define _VIPER_H

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>

#include <sys/types.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#include <ncursesw/menu.h>
#include <ncursesw/form.h>
#else
#include <curses.h>
#include <menu.h>
#include <form.h>
#endif

#define LIBVIPER_VERSION            "1.6.0"

#define VECTOR_TOP_TO_BOTTOM        1
#define VECTOR_BOTTOM_TO_TOP        -1

#define WSIZE_MIN                   -1
#define WSIZE_DEFAULT               (WSIZE_MIN)
#define WSIZE_UNCHANGED             -2
#define WSIZE_MAX                   -3
#define WSIZE_FULLSCREEN            (WSIZE_MAX)

#define WPOS_UNCHANGED              -1
#define WPOS_STAGGERED              -2
#define WPOS_DEFAULT                (WPOS_STAGGERED)
#define WPOS_CENTERED               -3

#define KMIO_HOOK_ENTER             0
#define KMIO_HOOK_LEAVE             1

#define CURS_RIGHT                  0x1U        /*  0001    */
#define CURS_LEFT                   0x2U        /*  0010    */
#define CURS_TOP                    0x4U        /*  0100    */
#define CURS_BOTTOM                 0x8U        /*  1000    */
#define CURS_EDGE                   0xFU        /*  1111    */
#define CURS_LOWER_RIGHT            (CURS_RIGHT | CURS_BOTTOM)
#define CURS_UPPER_RIGHT            (CURS_RIGHT | CURS_TOP)
#define CURS_LOWER_LEFT             (CURS_LEFT | CURS_BOTTOM)
#define CURS_UPPER_LEFT             (CURS_LEFT | CURS_TOP)

#define STATE_VISIBLE               (1UL << 1)
#define STATE_FOCUS                 (1UL << 2)
#define STATE_MANAGED               (1UL << 3)
#define STATE_UNMANAGED             (1UL << 4)
#define STATE_SHADOWED              (1UL << 5)
#define STATE_EMINENT               (1UL << 6)
#define STATE_NORESIZE              (1UL << 7)
#define STATE_UNSET                 (1UL << 31)

#define MSGBOX_ICON_INFO            (1UL << 1)
#define MSGBOX_ICON_WARN            (1UL << 2)
#define MSGBOX_ICON_ERROR           (1UL << 3)
#define MSGBOX_ICON_QUESTION        (1UL << 4)
#define MSGBOX_TYPE_OK              (1UL << 10)
#define MSGBOX_TYPE_YESNO           (1UL << 11)
#define MSGBOX_FLAG_EMINENT         (1UL << 20)

#define FORM_CURSOR_BLOCK           0
#define FORM_CURSOR_ULINE           (1 << 1)
#define FORM_CURSOR_NONE            (1 << 2)
#define FORM_COLORIZE               (1 << 4)

#define FILEDLG_OPEN                0
#define FILEDLG_SAVE                (1UL << 1)
#define FILEDLG_SORT_DIRS           (1UL << 2)
#define FILEDLG_SHOW_FILES          (1UL << 3)
#define FILEDLG_SHOW_DIRS           (1UL << 4)
#define FILEDLG_SHOW_HIDDEN         (1UL << 5)
#define FILEDLG_SHOW_SIZE           (1UL << 6)
#define FILEDLG_SHOW_CTIME          (1UL << 7)
#define FILEDLG_SHOW_PRIV           (1UL << 8)
#define FILEDLG_MULTISELECT         (1UL << 16)
#define FILEDLG_BASIC                 \
            (FILEDLG_SHOW_FILES | FILEDLG_SHOW_DIRS | FILEDLG_SORT_DIRS)
#define FILEDLG_STANDARD            (FILEDLG_BASIC | FILEDLG_SHOW_SIZE)
#define FILEDLG_EXTENDED            (FILEDLG_STANDARD | FILEDLG_SHOW_CTIME)
#define FILEDLG_FULL                (FILEDLG_EXTENDED | FILEDLG_SHOW_PRIV)
#define FILEDLG_COMPLETE            (FILEDLG_FULL | FILEDLG_SHOW_HIDDEN)

#define REDRAW_MOUSE                (1 << 1)
#define REDRAW_WINDOWS_MANAGED      (1 << 2)
#define REDRAW_WINDOWS_UNMANAGED    (1 << 3)
#define REDRAW_DECK                       \
            (REDRAW_WINDOWS_MANAGED | REDRAW_WINDOWS_UNMANAGED)
#define REDRAW_WORKSPACE            (1 << 4)
#define REDRAW_BACKGROUND           (1 << 5)
#define REDRAW_ALL                       \
            (REDRAW_MOUSE | REDRAW_DECK | REDRAW_WORKSPACE)

/* keystroke definitions */
#ifndef KEY_TAB
#define KEY_TAB                     9
#endif
#ifndef KEY_CRLF
#define KEY_CRLF                    10
#endif

#define VIPER_FASTCOLOR             (1 << 1)
#define VIPER_GPM_SIGIO             (1 << 2)

/* callback definitions */
typedef int     (*VIPER_FUNC)(WINDOW *window, void *arg);
typedef int     (*VIPER_KEY_FUNC)(int32_t keystroke, void *anything);
typedef int     (*VIPER_WKEY_FUNC)(int32_t keystroke, WINDOW *window);
typedef int32_t (*VIPER_KMIO_HOOK)(int32_t keystroke);

typedef struct _viper_s             VIPER;
typedef struct _viper_wnd_s         VIPER_WND;
typedef struct _viper_event_s       VIPER_EVENT;
typedef struct _viper_event_s       viper_event_t;


/* basic window routines    */
WINDOW*             window_create(WINDOW *parent, int x, int y,
                        int width, int height);
void                window_decorate(WINDOW *wnd, char *title, bool border);
void                window_modify_border(WINDOW *window,
                        int attrs, short colors);
WINDOW*             window_create_shadow(WINDOW *window, WINDOW *window_below);

#ifdef _VIPER_WIDE
void                window_fill(WINDOW *window, cchar_t *ch,
                        short color, attr_t attr);
#else
void                window_fill(WINDOW *window, chtype ch,
                        short color, attr_t attr);
#endif

void                window_write_to_eol(WINDOW *window,
                        int x, int y, chtype ch);
void                window_get_center(WINDOW *window, int *x, int *y);
int                 window_check_width(WINDOW *window);
int                 window_check_height(WINDOW *window);
void                window_get_size_scaled(WINDOW *refrence,
                        int *width, int *height,
                        float hscale, float vscale);
int                 window_move_rel(WINDOW *window,
                        int vector_x, int vector_y);
void                subwin_move_realign(WINDOW *subwin);

/*
    basic cursor routines
    these functions all return -1 if the condition is false.
    if the condidtion is true, the function retuns the coordinate of the
    cursor in the other axis--except corner checks and is_curs_edge() which
    return 0 when true.
*/
int             is_cursor_at(WINDOW *window, uint16_t mask);
#define         is_curs_at_left(x)          is_cursor_at(x, CURS_LEFT)
#define         is_curs_at_right(x)         (is_cursor_at(x, CURS_RIGHT))
#define         is_curs_at_top(x)           is_cursor_at(x, CURS_TOP)
#define         is_curs_at_bottom(x)        (is_cursor_at(x, CURS_BOTTOM))
#define         is_curs_at_edge(x)          is_cursor_at(x, CURS_EDGE)
#define         is_curs_at_upper_left(x)    is_cursor_at(x, CURS_UPPER_LEFT)
#define         is_curs_at_lower_left(x)    is_cursor_at(x, CURS_LOWER_LEFT)
#define         is_curs_at_upper_right(x)   is_cursor_at(x, CURS_UPPER_RIGHT)
#define         is_curs_at_lower_right(x)   is_cursor_at(x, CURS_LOWER_RIGHT)

/* initialization facilities  */
VIPER*          viper_init(uint32_t flags);
void            viper_end(void);
void            viper_set_border_agent(VIPER_FUNC agent, int id);

/* viper screen facilities */
WINDOW*         viper_screen_get_wallpaper();
void            viper_screen_set_wallpaper(WINDOW *wallpaper,
                    VIPER_FUNC agent, void *arg);
void            viper_screen_redraw(uint32_t update_mask);

/* viper color facilities  */
short           viper_color_pair(short fg, short bg);
#define         VIPER_COLORS(fg,bg)    (COLOR_PAIR(viper_color_pair(fg, bg)))

/* window construction and destruction */
WINDOW*         viper_window_create(char *title, float x, float y,
                    float width, float height, bool managed);
void            viper_window_set_class(WINDOW *window, void *classid);
void            viper_window_set_title(WINDOW *window, const char *title);
const char*     viper_window_get_title(WINDOW *window);
int             viper_window_set_limits(WINDOW *window,
                    int min_width, int min_height,
                    int max_width, int max_height);
void            viper_window_modify_border(WINDOW *window,
                    int attrs, short colors);
#define         viper_window_close(window) \
                    (viper_event_run(window, "window-close"))
int             viper_window_destroy(WINDOW *window);

/* special construction:  a message dialog box  */
WINDOW*         viper_msgbox_create(char *title, float x, float y,
                    int width, int height, char *msg, uint32_t flags);

/* special construction:  a file/directory load/save dialog box   */
/*
WINDOW*         viper_filedlg_create(WINDOW *parent, char *title,
                    float x, float y, float width, float height,
                    char *dir, uint32_t flags);
*/

/* window placement */
WINDOW*         viper_window_get_top(uint32_t state_mask);
void            viper_window_set_top(WINDOW *window);
int             viper_mvwin_rel(WINDOW *window, int vector_x, int vector_y);
int             viper_mvwin_abs(WINDOW *window, int x, int y);
int             viper_wresize(WINDOW *window,
                    int width, int height, uint8_t flags);
#define         viper_wresize_abs(window, width, height) \
                    (viper_wresize(window, width, height, 0))
int             viper_wresize_rel(WINDOW *window, int vector_x, int vector_y);
#define         TOPMOST_WINDOW (viper_window_get_top(STATE_VISIBLE))

/* window search facilities */
WINDOW*         viper_window_find_by_class(void *classid);
WINDOW*         viper_window_find_by_title(char *title);

/* window display and state modification */
void            viper_window_set_state(WINDOW *window, uint32_t state);
uint32_t        viper_window_get_state(WINDOW *window);
void            viper_window_set_border_agent(WINDOW *window,
                    VIPER_FUNC agent, int id);
void            viper_window_show(WINDOW *window);
void            viper_window_touch(WINDOW *window);
void            viper_window_redraw(WINDOW *window);
#define         viper_window_unhide(window)   \
                    (viper_window_set_state(window, STATE_VISIBLE))
#define         viper_window_focus(window)    \
                    (viper_window_set_state(window, STATE_FOCUS))
#define         viper_window_hide(window)     \
                    (viper_window_set_state(window, STATE_UNSET | STATE_VISIBLE))

/* kmio faclilities (keyboard & mouse i/o)   */
int32_t         viper_kmio_fetch(MEVENT *mouse_event);
void            viper_kmio_dispatch(int32_t keystroke, MEVENT *mouse_event);
void            viper_kmio_dispatch_set_hook(int sequence, VIPER_KMIO_HOOK hook);
void            viper_window_set_key_func(WINDOW *window, VIPER_WKEY_FUNC func);

/* event handling */
int             viper_event_set(WINDOW *window, char *event,
                    VIPER_FUNC func, void *arg);
int             viper_event_del(WINDOW *window, char *event);
int             viper_event_exec(WINDOW *window, char *event, void *anything);
#define         viper_event_run(window, event) \
                    (viper_event_exec(window, event, NULL))
#define         VIPER_EVENT_BROADCAST           ((WINDOW*)"ALL_VIPER_WINDOWS")
#define         VIPER_EVENT_WINDOW_DESIST       (viper_window_destroy(window))
#define         VIPER_EVENT_WINDOW_PERSIST      0
void            viper_window_for_each(VIPER_FUNC func, void *arg, int vector);

/* viper window deck functions */
void            viper_deck_cycle(int vector);
WINDOW*         viper_deck_hit_test(int x, int y);
char**          viper_deck_get_wndlist(void);

/* menu helpers */
MENU*           viper_menu_create(char **items);
void            viper_menu_items_add(MENU *menu, char **items);
void            viper_menu_items_change(MENU *menu, char **items);
WINDOW*         viper_menu_bind(MENU *menu, WINDOW *parent, float x, float y,
                    float width, float height);
void            viper_menu_destroy(MENU *menu, bool free_windows);
#define         CURRENT_MENU_ITEM(menu)    (item_index(current_item(menu)))

/* form helpers */
void            viper_form_colorize(FORM *form, chtype field_active,
                    chtype field_normal, chtype text_active, chtype text_normal);
#define         viper_form_normalize(form, fcolors, tcolors)   \
                    (viper_form_colorize(form, fcolors, fcolors, tcolors, tcolors))
void            viper_form_destroy(FORM *form, bool free_windows);
int             viper_form_driver(FORM *form, int request, uint32_t flags,
                    chtype active, chtype normal, short cursor_color);
#define         CURRENT_FORM_ITEM(form)    (field_index(current_field(form)))

/* miscellaneous functions */
void            viper_window_set_userptr(WINDOW *window, void *anything);
void*           viper_window_get_userptr(WINDOW *window);
#define         WINDOW_FRAME(window)          (viper_get_window_frame(window))


/* INTERNAL USE ONLY:  core */
VIPER_WND*      viper_get_viper_wnd(WINDOW *window);
VIPER_EVENT*    viper_get_viper_event(WINDOW *window, char *event);
WINDOW*         viper_get_window_frame(WINDOW *window);

#endif
