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

#define LIBVIPER_VERSION            "1.7.0"

#define MAX_SCREENS                 4

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
#define STATE_SHADOWED              (1UL << 5)
#define STATE_NORESIZE              (1UL << 7)
#define STATE_UNSET                 (1UL << 31)

#define MSGBOX_ICON_INFO            (1UL << 1)
#define MSGBOX_ICON_WARN            (1UL << 2)
#define MSGBOX_ICON_ERROR           (1UL << 3)
#define MSGBOX_ICON_QUESTION        (1UL << 4)
#define MSGBOX_TYPE_OK              (1UL << 10)
#define MSGBOX_TYPE_YESNO           (1UL << 11)

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
#define REDRAW_WINDOWS              (1 << 2)
#define REDRAW_WORKSPACE            (1 << 4)
#define REDRAW_BACKGROUND           (1 << 5)
#define REDRAW_ALL                       \
            (REDRAW_MOUSE | REDRAW_WINDOWS | REDRAW_WORKSPACE)

/* keystroke definitions */
#ifndef KEY_TAB
#define KEY_TAB                     9
#endif
#ifndef KEY_CRLF
#define KEY_CRLF                    10
#endif

#define VIPER_FASTCOLOR             (1 << 1)
#define VIPER_GPM_SIGIO             (1 << 2)

typedef struct _viper_s             VIPER;
typedef struct _viper_ctx_s         vctx_t;
typedef struct _viper_wnd_s         VIPER_WND;          // legacy ref
typedef struct _viper_wnd_s         vwnd_t;
typedef struct _viper_event_s       VIPER_EVENT;
typedef struct _viper_event_s       viper_event_t;

/* callback definitions */
typedef int         (*ViperFunc)(vwnd_t *vwnd, void *arg);
typedef int         (*ViperKeyFunc)(int32_t keystroke, void *anything);
typedef int         (*ViperWkeyFunc)(int32_t keystroke, vwnd_t *vwnd);
typedef int32_t     (*ViperKmioHook)(int32_t keystroke);
typedef void        (*ViperBkgdFunc)(int screen_id);

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
void            viper_set_border_agent(ViperFunc agent, int id);

/* viper screen facilities */
WINDOW*         viper_get_screen_window(int screen_id);
int             viper_get_active_screen(void);
#define         CURRENT_SCREEN          viper_get_screen_window(-1)
#define         CURRENT_SCREEN_ID       viper_get_active_screen()

WINDOW*         viper_screen_get_wallpaper(int screen_id);
void            viper_screen_set_wallpaper(int screen_id, WINDOW *wallpaper,
                    ViperBkgdFunc agent);

void            viper_screen_redraw(int screen_id, uint32_t update_mask);

/* viper color facilities  */
short           viper_color_pair(short fg, short bg);
#define         VIPER_COLORS(fg,bg)    (COLOR_PAIR(viper_color_pair(fg, bg)))

/* window construction and destruction */
vwnd_t*         viper_window_create(int screen_id, bool managed, char *title,
                    float x, float y, float width, float height);
#define         VWINDOW(wnd)              (*(WINDOW**)wnd)


WINDOW*         viper_window_get_frame(vwnd_t *wnd);
#define         WINDOW_FRAME(wnd)       (viper_window_get_frame(wnd))

void            viper_window_set_class(vwnd_t *wnd, void *classid);
void            viper_window_set_title(vwnd_t *wnd, const char *title);
const char*     viper_window_get_title(vwnd_t *wnd);
int             viper_window_set_limits(vwnd_t *wnd,
                    int min_width, int min_height,
                    int max_width, int max_height);
void            viper_window_modify_border(vwnd_t *wnd,
                    int attrs, short colors);
void            viper_window_close(vwnd_t *vwnd);

/* special construction:  a message dialog box  */
vwnd_t*         viper_msgbox_create(int screen_id, char *title,
                    float x, float y, int width, int height,
                    char *msg, uint32_t flags);

/* special construction:  a file/directory load/save dialog box   */
/*
WINDOW*         viper_filedlg_create(WINDOW *parent, char *title,
                    float x, float y, float width, float height,
                    char *dir, uint32_t flags);
*/

/* window placement */
vwnd_t*         viper_window_get_top(int screen_id, bool managed);
bool            viper_window_set_top(vwnd_t *wnd);
int             viper_mvwin_rel(vwnd_t *wnd, int vector_x, int vector_y);
int             viper_mvwin_abs(vwnd_t *wnd, int x, int y);
int             viper_wresize(vwnd_t *wnd, int width, int height);
#define         viper_wresize_abs(wnd, width, height) \
                    viper_wresize(wnd, width, height)
int             viper_wresize_rel(vwnd_t *wind, int vector_x, int vector_y);

#define         TOPMOST_MANAGED     (viper_window_get_top(-1, TRUE))
#define         TOPMOST_UNMANGED    (viper_window_get_top(-1, FALSE))

/* window search facilities */
vwnd_t*         viper_window_find_by_class(int screen_id, bool managed, void *classid);
vwnd_t*         viper_window_find_by_title(int screen_id, bool managed, char *title);

/* window display and state modification */
uint32_t        viper_window_get_state(vwnd_t *wnd);
void            viper_window_set_shadow(vwnd_t *wnd, bool value);
void            viper_window_set_visible(vwnd_t *wnd, bool value);
void            viper_window_set_resizable(vwnd_t *wnd, bool value);
bool            viper_window_set_focus(vwnd_t *wnd);
#define         viper_window_show(wnd) \
                    viper_window_set_visible(wnd, TRUE);
#define         viper_window_hide(wnd) \
                    viper_window_set_visible(wnd, FALSE);
void            viper_window_set_border_agent(vwnd_t *wnd,
                    ViperFunc agent, int id);
void            viper_window_touch(vwnd_t *wnd);
void            viper_window_redraw(vwnd_t *wnd);

/* kmio faclilities (keyboard & mouse i/o)   */
int32_t         viper_kmio_fetch(MEVENT *mouse_event);
void            viper_kmio_dispatch(int32_t keystroke, MEVENT *mouse_event);
void            viper_kmio_dispatch_set_hook(int sequence, ViperKmioHook hook);
void            viper_window_set_key_func(vwnd_t *wnd, ViperWkeyFunc func);

/* event handling */
int             viper_event_set(vwnd_t *wnd, char *event,
                    ViperFunc func, void *arg);
int             viper_event_del(vwnd_t *wnd, char *event);
int             viper_event_exec(vwnd_t *wnd, char *event, void *anything);
#define         viper_event_run(wnd, event) \
                    (viper_event_exec(wnd, event, NULL))
#define         VIPER_EVENT_BROADCAST           ((vwnd_t*)"ALL_VIPER_WINDOWS")
void            viper_window_for_each(int screen_id, bool managed, int vector,
                    ViperFunc func, void *arg);

/* viper window deck functions */
void            viper_deck_cycle(int screen_id, bool managed, int vector);
vwnd_t*         viper_deck_hit_test(int screen_id, bool managed, int x, int y);
char**          viper_deck_get_wndlist(int screen_id, bool managed);

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
void            viper_window_set_userptr(vwnd_t *wnd, void *anything);
void*           viper_window_get_userptr(vwnd_t *wnd);

#endif
