#ifndef _VDK_H_
#define _VDK_H_

#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#define NCURSES_OPAQUE 0
#include <ncursesw/curses.h>

#include "vdk_color.h"

/* size / position sentinels */
#define WSIZE_UNCHANGED             -2
#define WPOS_UNCHANGED              -1

/* widget state flags */
#define VK_STATE_VISIBLE            (1UL << 1)
#define VK_STATE_FROZEN             (1UL << 3)
#define VK_STATE_NORESIZE           (1UL << 7)

/* frame / border styles */
#define VK_FRAME_NONE               0
#define VK_FRAME_SINGLE             1
#define VK_FRAME_DOUBLE             2
#define VK_FRAME_ASCII              3

/* separator styles */
#define VK_SEPARATOR_BLANK          1
#define VK_SEPARATOR_SINGLE         2
#define VK_SEPARATOR_DOUBLE         3

/* text justification */
#define VK_JUSTIFY_LEFT             0
#define VK_JUSTIFY_RIGHT            1
#define VK_JUSTIFY_CENTER           2

/* box orientation */
#define VK_BOX_HORIZONTAL           0
#define VK_BOX_VERTICAL             1

/* scrollbar flags */
#define VK_SCROLLBAR_NONE           0
#define VK_SCROLLBAR_VERTICAL       (1 << 0)
#define VK_SCROLLBAR_HORIZONTAL     (1 << 1)
#define VK_SCROLLBAR_BOTH           (VK_SCROLLBAR_VERTICAL | VK_SCROLLBAR_HORIZONTAL)

/* marquee scroll direction */
#define VK_SCROLL_LEFT              0
#define VK_SCROLL_RIGHT             1
#define VK_SCROLL_LOOP              2

/* listbox flags */
#define VK_FLAG_ALLOW_WRAP          (1 << 1)

/* item flags */
#define VK_ITEM_CHECKED             (1 << 0)

/* selectbox modes */
#define VK_SELECTBOX_CHECKBOX       0
#define VK_SELECTBOX_RADIO          1

/* vector / direction */
#define VK_VECTOR_LEFT              1
#define VK_VECTOR_RIGHT             -1

/* deck widget position */
#define VK_DECK_TOP                 0
#define VK_DECK_BOTTOM              1

/* keystroke definitions */
#ifndef KEY_TAB
#define KEY_TAB                     9
#endif
#ifndef KEY_CRLF
#define KEY_CRLF                    10
#endif

/* vk klass typedefs */
typedef struct  _vk_object_s        vk_object_t;
typedef struct  _vk_screen_s        vk_screen_t;
typedef struct  _vk_surface_s       vk_surface_t;
typedef struct  _vk_widget_s        vk_widget_t;
typedef struct  _vk_container_s     vk_container_t;
typedef struct  _vk_listbox_s       vk_listbox_t;
typedef struct  _vk_selectbox_s     vk_selectbox_t;
typedef struct  _vk_frame_s         vk_frame_t;
typedef struct  _vk_scroller_s      vk_scroller_t;
typedef struct  _vk_window_s        vk_window_t;
typedef struct  _vk_box_s           vk_box_t;
typedef struct  _vk_label_s         vk_label_t;
typedef struct  _vk_textbox_s       vk_textbox_t;
typedef struct  _vk_marquee_s       vk_marquee_t;
typedef struct  _vk_deck_s          vk_deck_t;

/* callback typedefs */
typedef int         (*VkWidgetFunc)(vk_widget_t *widget, void *anything);
typedef void        (*VkScrollInfoFunc)(vk_widget_t *child,
                        int *content_h, int *content_w,
                        int *scroll_y, int *scroll_x);
typedef void        (*VkSurfaceBkgdFunc)(vk_screen_t *screen,
                        int surface_id, WINDOW *canvas);
typedef void        (*VkWindowDecorateFunc)(vk_window_t *window,
                        WINDOW *canvas, void *data);

/* cast macros */
#define VK_OBJECT(x)            ((vk_object_t *)x)
#define VK_SCREEN(x)            ((vk_screen_t *)x)
#define VK_WIDGET(x)            ((vk_widget_t *)x)
#define VK_CONTAINER(x)         ((vk_container_t *)x)
#define VK_LISTBOX(x)           ((vk_listbox_t *)x)
#define VK_SELECTBOX(x)         ((vk_selectbox_t *)x)
#define VK_FRAME(x)             ((vk_frame_t *)x)
#define VK_SCROLLER(x)          ((vk_scroller_t *)x)
#define VK_WINDOW(x)            ((vk_window_t *)x)
#define VK_BOX(x)               ((vk_box_t *)x)
#define VK_LABEL(x)             ((vk_label_t *)x)
#define VK_TEXTBOX(x)           ((vk_textbox_t *)x)
#define VK_MARQUEE(x)           ((vk_marquee_t *)x)
#define VK_DECK(x)              ((vk_deck_t *)x)

/* vk_object */
const char*     vk_object_get_klass_name(vk_object_t *object);
int             vk_object_push_keystroke(vk_object_t *object,
                    int32_t keystroke);
int             vk_object_destroy(vk_object_t *object);

/* vk_screen */
vk_screen_t*    vk_screen_create(void);
int             vk_screen_add_surface(vk_screen_t *screen);
int             vk_screen_del_surface(vk_screen_t *screen, int id);
int             vk_screen_switch_surface(vk_screen_t *screen, int id);
WINDOW*         vk_screen_get_window(vk_screen_t *screen);
int             vk_screen_attach_widget(vk_screen_t *screen,
                    int surface_id, vk_widget_t *widget);
int             vk_screen_detach_widget(vk_screen_t *screen,
                    int surface_id, vk_widget_t *widget);
int             vk_screen_resize(vk_screen_t *screen);
int             vk_screen_poll_resize(vk_screen_t *screen);
int             vk_screen_teleport(vk_screen_t *screen, const char *pty);
int             vk_screen_set_wallpaper(vk_screen_t *screen,
                    VkSurfaceBkgdFunc func);
int             vk_screen_paint_wallpaper(vk_screen_t *screen);
int             vk_screen_refresh(vk_screen_t *screen);
void            vk_screen_destroy(vk_screen_t *screen);

/* vk_widget */
vk_widget_t*    vk_widget_create(int width, int height);
int             vk_widget_set_surface(vk_widget_t *widget, WINDOW *window);
WINDOW*         vk_widget_get_surface(vk_widget_t *widget);
void            vk_widget_set_colors(vk_widget_t *widget, int fg, int bg);
void            vk_widget_set_attrs(vk_widget_t *widget, attr_t attrs);
short           vk_widget_get_fg(vk_widget_t *widget);
short           vk_widget_get_bg(vk_widget_t *widget);
int             vk_widget_get_metrics(vk_widget_t *widget,
                    int *width, int *height);
int             vk_widget_erase(vk_widget_t *widget);
int             vk_widget_resize(vk_widget_t *widget, int width, int height);
int             vk_widget_recreate(vk_widget_t *widget);
void            vk_widget_fill(vk_widget_t *widget, chtype ch);
int             vk_widget_draw(vk_widget_t *widget);
uint32_t        vk_widget_get_state(vk_widget_t *widget);
void            vk_widget_set_state(vk_widget_t *widget, uint32_t state);
#define         vk_widget_freeze(w) \
                    vk_widget_set_state(w, vk_widget_get_state(w) | VK_STATE_FROZEN)
#define         vk_widget_thaw(w) \
                    vk_widget_set_state(w, vk_widget_get_state(w) & ~VK_STATE_FROZEN)
#define         vk_widget_show(w) \
                    vk_widget_set_state(w, vk_widget_get_state(w) | VK_STATE_VISIBLE)
#define         vk_widget_hide(w) \
                    vk_widget_set_state(w, vk_widget_get_state(w) & ~VK_STATE_VISIBLE)
#define         vk_widget_is_visible(w) \
                    (vk_widget_get_state(w) & VK_STATE_VISIBLE)
int             vk_widget_move(vk_widget_t *widget, int x, int y);
void            vk_widget_destroy(vk_widget_t *widget);

/* vk_container */
vk_container_t* vk_container_create(int width, int height);
int             vk_container_add_widget(vk_container_t *container,
                    vk_widget_t *widget);
int             vk_container_remove_widget(vk_container_t *container,
                    vk_widget_t *widget);
int             vk_container_vacate(vk_container_t *container);
int             vk_container_destroy(vk_container_t *container);

/* vk_listbox */
vk_listbox_t*   vk_listbox_create(int width, int height);
int             vk_listbox_set_wrap(vk_listbox_t *listbox, bool allowed);
int             vk_listbox_set_title(vk_listbox_t *listbox, char *title);
int             vk_listbox_get_title(vk_listbox_t *listbox,
                    char *buf, int buf_sz);
int             vk_listbox_set_highlight(vk_listbox_t *listbox, int fg, int bg);
int             vk_listbox_add_item(vk_listbox_t *listbox,
                    char *item, VkWidgetFunc func, void *anything);
int             vk_listbox_set_item(vk_listbox_t *listbox, int idx,
                    char *item, VkWidgetFunc func, void *anything);
int             vk_listbox_remove_item(vk_listbox_t *listbox, int idx);
int             vk_listbox_get_item(vk_listbox_t *listbox, int idx,
                    char *buf, int buf_sz);
int             vk_listbox_get_item_count(vk_listbox_t *listbox);
int             vk_listbox_get_selected(vk_listbox_t *listbox);
int             vk_listbox_get_metrics(vk_listbox_t *listbox,
                    int *width, int *height);
int             vk_listbox_update(vk_listbox_t *listbox);
int             vk_listbox_reset(vk_listbox_t *listbox);
int             vk_listbox_add_separator(vk_listbox_t *listbox, int style);
void            vk_listbox_destroy(vk_listbox_t *listbox);

/* vk_selectbox */
vk_selectbox_t* vk_selectbox_create(int width, int height, int mode);
int             vk_selectbox_set_style(vk_selectbox_t *selectbox, int style);
int             vk_selectbox_set_wrap(vk_selectbox_t *selectbox, bool allowed);
int             vk_selectbox_set_highlight(vk_selectbox_t *selectbox,
                    int fg, int bg);
int             vk_selectbox_add_item(vk_selectbox_t *selectbox,
                    char *name, VkWidgetFunc func, void *anything);
int             vk_selectbox_toggle_item(vk_selectbox_t *selectbox, int idx);
bool            vk_selectbox_item_is_checked(vk_selectbox_t *selectbox,
                    int idx);
int             vk_selectbox_check_item(vk_selectbox_t *selectbox, int idx);
int             vk_selectbox_uncheck_item(vk_selectbox_t *selectbox, int idx);
int             vk_selectbox_uncheck_all(vk_selectbox_t *selectbox);
int             vk_selectbox_update(vk_selectbox_t *selectbox);
void            vk_selectbox_destroy(vk_selectbox_t *selectbox);

/* vk_frame */
vk_frame_t*     vk_frame_create(int width, int height);
int             vk_frame_set_border_style(vk_frame_t *frame, int style);
int             vk_frame_set_border_colors(vk_frame_t *frame,
                    short fg, short bg);
int             vk_frame_set_child(vk_frame_t *frame, vk_widget_t *child);
vk_widget_t*    vk_frame_get_child(vk_frame_t *frame);
int             vk_frame_update(vk_frame_t *frame);
void            vk_frame_destroy(vk_frame_t *frame);

/* vk_scroller */
vk_scroller_t*  vk_scroller_create(int flags);
int             vk_scroller_set_border_style(vk_scroller_t *scroller,
                    int style);
int             vk_scroller_set_border_colors(vk_scroller_t *scroller,
                    short fg, short bg);
int             vk_scroller_set_scroll_info(vk_scroller_t *scroller,
                    VkScrollInfoFunc func);
int             vk_scroller_set_scroll_source(vk_scroller_t *scroller,
                    vk_widget_t *source);
int             vk_scroller_update(vk_scroller_t *scroller);
void            vk_scroller_destroy(vk_scroller_t *scroller);

int             vk_widget_attach_scroller(vk_widget_t *host,
                    vk_scroller_t *scroller);
int             vk_widget_detach_scroller(vk_widget_t *host,
                    vk_scroller_t *scroller);

/* vk_window */
vk_window_t*    vk_window_create(int width, int height);
int             vk_window_set_title(vk_window_t *window, const char *title);
const char*     vk_window_get_title(vk_window_t *window);
int             vk_window_set_title_justify(vk_window_t *window, int justify);
int             vk_window_set_decorate(vk_window_t *window,
                    VkWindowDecorateFunc func, void *data);
int             vk_window_set_border_style(vk_window_t *window, int style);
int             vk_window_set_border_colors(vk_window_t *window,
                    short fg, short bg);
int             vk_window_set_child(vk_window_t *window, vk_widget_t *child);
vk_widget_t*    vk_window_get_child(vk_window_t *window);
int             vk_window_update(vk_window_t *window);
void            vk_window_destroy(vk_window_t *window);

/* vk_box */
vk_box_t*       vk_box_create(int width, int height,
                    int orientation, int slots);
int             vk_box_set_widget(vk_box_t *box, int slot,
                    vk_widget_t *widget);
vk_widget_t*    vk_box_get_widget(vk_box_t *box, int slot);
int             vk_box_update(vk_box_t *box);
void            vk_box_destroy(vk_box_t *box);

/* vk_label */
vk_label_t*     vk_label_create(int width);
int             vk_label_set_text(vk_label_t *label, const char *text);
const char*     vk_label_get_text(vk_label_t *label);
int             vk_label_set_justify(vk_label_t *label, int justify);
int             vk_label_update(vk_label_t *label);
void            vk_label_destroy(vk_label_t *label);

/* vk_textbox */
vk_textbox_t*   vk_textbox_create(int width, int height);
int             vk_textbox_set_text(vk_textbox_t *textbox, const char *text);
const char*     vk_textbox_get_text(vk_textbox_t *textbox);
int             vk_textbox_set_word_wrap(vk_textbox_t *textbox, bool enabled);
int             vk_textbox_get_line_count(vk_textbox_t *textbox);
int             vk_textbox_update(vk_textbox_t *textbox);
void            vk_textbox_destroy(vk_textbox_t *textbox);

/* vk_marquee */
vk_marquee_t*   vk_marquee_create(int width);
int             vk_marquee_set_text(vk_marquee_t *marquee, const char *text);
const char*     vk_marquee_get_text(vk_marquee_t *marquee);
int             vk_marquee_set_direction(vk_marquee_t *marquee, int direction);
int             vk_marquee_set_speed(vk_marquee_t *marquee, int interval);
int             vk_marquee_set_pause(vk_marquee_t *marquee, int duration);
int             vk_marquee_set_repeat(vk_marquee_t *marquee, bool repeat);
int             vk_marquee_run(vk_marquee_t *marquee);
void            vk_marquee_destroy(vk_marquee_t *marquee);

/* vk_deck */
vk_deck_t*      vk_deck_create(void);
int             vk_deck_add_widget(vk_deck_t *deck,
                    vk_widget_t *widget, int position);
int             vk_deck_remove_widget(vk_deck_t *deck,
                    vk_widget_t *widget);
int             vk_deck_set_top(vk_deck_t *deck, vk_widget_t *widget);
vk_widget_t*    vk_deck_get_top(vk_deck_t *deck);
int             vk_deck_cycle(vk_deck_t *deck, int vector);
int             vk_deck_update(vk_deck_t *deck);
void            vk_deck_destroy(vk_deck_t *deck);

#endif
