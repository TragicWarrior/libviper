#ifndef _VIPER_H
#define _VIPER_H

#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
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

#define  LIBVIPER_VERSION           "1.4.6"

#define  VECTOR_TOP_TO_BOTTOM       1
#define  VECTOR_BOTTOM_TO_TOP		   -1

#define  WSIZE_MIN                  -1
#define  WSIZE_DEFAULT              (WSIZE_MIN)
#define  WSIZE_UNCHANGED            -2
#define  WSIZE_MAX                  -3
#define  WSIZE_FULLSCREEN           (WSIZE_MAX)

#define  WPOS_UNCHANGED             -1
#define  WPOS_STAGGERED             -2
#define  WPOS_DEFAULT               (WPOS_STAGGERED)
#define  WPOS_CENTERED              -3

#define  KMIO_HOOK_ENTER            0
#define  KMIO_HOOK_LEAVE            1

#define	CURS_RIGHT						0x1U	/*	0001	*/
#define	CURS_LEFT						0x2U	/* 0010	*/
#define	CURS_TOP							0x4U	/* 0100	*/
#define	CURS_BOTTOM						0x8U	/*	1000	*/
#define	CURS_EDGE						0xFU	/*	1111	*/
#define	CURS_LOWER_RIGHT				(CURS_RIGHT | CURS_BOTTOM)
#define	CURS_UPPER_RIGHT				(CURS_RIGHT | CURS_TOP)
#define	CURS_LOWER_LEFT				(CURS_LEFT | CURS_BOTTOM)
#define	CURS_UPPER_LEFT				(CURS_LEFT | CURS_TOP)

#define  STATE_VISIBLE				   (1UL<<1)
#define  STATE_FOCUS					   (1UL<<2)
#define  STATE_MANAGED				   (1UL<<3)
#define  STATE_UNMANAGED				(1UL<<4)
#define  STATE_SHADOWED				   (1UL<<5)
#define  STATE_EMINENT				   (1UL<<6)
#define  STATE_NORESIZE             (1UL<<7)
#define  STATE_UNSET					   (1UL<<31)

#define  MSGBOX_ICON_INFO			   (1UL<<1)
#define  MSGBOX_ICON_WARN			   (1UL<<2)
#define  MSGBOX_ICON_ERROR			   (1UL<<3)
#define  MSGBOX_ICON_QUESTION       (1UL<<4)
#define  MSGBOX_TYPE_OK				   (1UL<<10)
#define  MSGBOX_TYPE_YESNO			   (1UL<<11)
#define  MSGBOX_FLAG_EMINENT        (1UL<<20)

#define  FORM_CURSOR_BLOCK          0
#define  FORM_CURSOR_ULINE          (1<<1)
#define  FORM_CURSOR_NONE           (1<<2)
#define  FORM_COLORIZE              (1<<4)

#define  FILEDLG_OPEN               0
#define  FILEDLG_SAVE               (1UL<<1)
#define  FILEDLG_SORT_DIRS          (1UL<<2)
#define  FILEDLG_SHOW_FILES         (1UL<<3)
#define  FILEDLG_SHOW_DIRS          (1UL<<4)
#define  FILEDLG_SHOW_HIDDEN        (1UL<<5)
#define  FILEDLG_SHOW_SIZE          (1UL<<6)
#define  FILEDLG_SHOW_CTIME         (1UL<<7)
#define  FILEDLG_SHOW_PRIV   			(1UL<<8)
#define  FILEDLG_MULTISELECT        (1UL<<16)
#define  FILEDLG_BASIC         		\
				(FILEDLG_SHOW_FILES | FILEDLG_SHOW_DIRS | FILEDLG_SORT_DIRS)
#define  FILEDLG_STANDARD      		(FILEDLG_BASIC | FILEDLG_SHOW_SIZE)
#define  FILEDLG_EXTENDED      		(FILEDLG_STANDARD | FILEDLG_SHOW_CTIME)
#define  FILEDLG_FULL          		(FILEDLG_EXTENDED | FILEDLG_SHOW_PRIV)
#define  FILEDLG_COMPLETE      		(FILEDLG_FULL | FILEDLG_SHOW_HIDDEN)

#define  REDRAW_MOUSE               (1<<1)
#define  REDRAW_WINDOWS_MANAGED	   (1<<2)
#define  REDRAW_WINDOWS_UNMANAGED	(1<<3)
#define  REDRAW_DECK					   \
				(REDRAW_WINDOWS_MANAGED | REDRAW_WINDOWS_UNMANAGED)
#define  REDRAW_WORKSPACE			   (1<<4)
#define  REDRAW_BACKGROUND			   (1<<5)
#define  REDRAW_ALL					   \
				(REDRAW_MOUSE | REDRAW_DECK | REDRAW_WORKSPACE)

/* keystroke definitions */
#ifndef  KEY_TAB
#define  KEY_TAB						   9
#endif
#ifndef  KEY_CRLF
#define  KEY_CRLF						   10
#endif

#define	VIPER_FASTCOLOR			   (1<<1)
#define	VIPER_GPM_SIGIO				(1<<2)

/* callback definitions */
typedef gint (*VIPER_FUNC)(WINDOW *window,gpointer arg);
typedef gint (*VIPER_KEY_FUNC)(gint32 keystroke,gpointer anything);
typedef gint (*VIPER_WKEY_FUNC)(gint32 keystroke,WINDOW *window);
typedef gint32 (*VIPER_KMIO_HOOK)(gint32 keystroke); 

typedef struct _viper_s			VIPER;
typedef struct _viper_wnd_s	VIPER_WND;
typedef struct _viper_event_s	VIPER_EVENT;


/* basic window routines	*/
WINDOW*			window_create(WINDOW *parent,gint x,gint y,
						gint width,gint height);
void				window_decorate(WINDOW *wnd,gchar *title,gboolean border);
void 				window_modify_border(WINDOW *window,gint attrs,gshort colors);
WINDOW*			window_create_shadow(WINDOW *window,WINDOW *window_below);

#ifdef _VIPER_WIDE
void				window_fill(WINDOW *window,cchar_t *ch,gshort color,attr_t attr);
#else
void				window_fill(WINDOW *window,chtype ch,gshort color,attr_t attr);
#endif

void 				window_write_to_eol(WINDOW *window,gint x,gint y,chtype ch);
void 				window_get_center(WINDOW *window,gint *x,gint *y);
gint				window_check_width(WINDOW *window);
gint				window_check_height(WINDOW *window);
void				window_get_size_scaled(WINDOW *refrence,
						gint *width,gint *height,gfloat hscale,gfloat vscale);
gint				window_move_rel(WINDOW *window,gint vector_x,gint vector_y);
void           subwin_move_realign(WINDOW *subwin);

/* basic cursor routines
	these functions all return -1 if the condition is false.
	if the condidtion is true, the function retuns the coordinate of the
	cursor in the other axis--except corner checks and is_curs_edge() which
	return 0 when true.		*/
gint				is_cursor_at(WINDOW *window,guint mask);
#define			is_curs_at_left(x)				is_cursor_at(x,CURS_LEFT)
#define			is_curs_at_right(x)				(is_cursor_at(x,CURS_RIGHT))
#define			is_curs_at_top(x)					is_cursor_at(x,CURS_TOP)
#define			is_curs_at_bottom(x)				(is_cursor_at(x,CURS_BOTTOM))
#define			is_curs_at_edge(x)				is_cursor_at(x,CURS_EDGE)
#define			is_curs_at_upper_left(x) 		is_cursor_at(x,CURS_UPPER_LEFT)
#define			is_curs_at_lower_left(x) 		is_cursor_at(x,CURS_LOWER_LEFT)
#define			is_curs_at_upper_right(x) 		is_cursor_at(x,CURS_UPPER_RIGHT)
#define			is_curs_at_lower_right(x) 		is_cursor_at(x,CURS_LOWER_RIGHT)

/* initialization facilities  */
VIPER*			viper_init(guint32 flags);
void           viper_end(void);
void           viper_set_border_agent(VIPER_FUNC agent,gint id);

/* viper thread saftey */
void				viper_thread_enter(void);
void				viper_thread_leave(void);

/* viper screen facilities */
WINDOW*			viper_screen_get_wallpaper();
void				viper_screen_set_wallpaper(WINDOW *wallpaper,
                  VIPER_FUNC agent,gpointer arg);
void				viper_screen_redraw(gint32 update_mask);

/* viper color facilities  */
gshort			viper_color_pair(gshort fg,gshort bg);
#define			VIPER_COLORS(fg,bg)	(COLOR_PAIR(viper_color_pair(fg,bg)))

/* window construction and destruction */
WINDOW* 			viper_window_create(gchar *title,gfloat x,gfloat y,
						gfloat width,gfloat height,gboolean managed);
void				viper_window_set_class(WINDOW *window,gpointer classid);
void				viper_window_set_title(WINDOW *window,const gchar *title);
const gchar*	viper_window_get_title(WINDOW *window);
gint				viper_window_set_limits(WINDOW *window,gint min_width,
                  gint min_height,gint max_width,gint max_height);
void				viper_window_modify_border(WINDOW *window,
						gint attrs,gshort colors);
#define			viper_window_close(window) \
                  (viper_event_run(window,"window-close"))
gint				viper_window_destroy(WINDOW *window);

/* special construction:  a message dialog box  */
WINDOW*			viper_msgbox_create(gchar *title,gfloat x,gfloat y,
						gint width,gint height,gchar *msg,gint32 flags);

/* special construction:  a file/directory load/save dialog box   */
WINDOW*        viper_filedlg_create(WINDOW *parent,gchar *title,
                  gfloat x,gfloat y,gfloat width,gfloat height,
                  gchar *dir,gint32 flags);

/* window placement */
WINDOW*			viper_window_get_top(guint32 state_mask);
void				viper_window_set_top(WINDOW *window);
gint				viper_mvwin_rel(WINDOW *window,gint vector_x,gint vector_y);
gint				viper_mvwin_abs(WINDOW *window,gint x,gint y);
gint           viper_wresize(WINDOW *window,gint width,gint height,gint8 flags);
#define        viper_wresize_abs(window,width,height) \
                  (viper_wresize(window,width,height,0))
gint				viper_wresize_rel(WINDOW *window,gint vector_x,gint vector_y);
#define 			TOPMOST_WINDOW (viper_window_get_top(STATE_VISIBLE))

/* window search facilities */
WINDOW*			viper_window_find_by_class(gpointer classid);
WINDOW*			viper_window_find_by_title(gchar *title);

/* window display and state modification */
void				viper_window_set_state(WINDOW *window,guint32 state);
guint32			viper_window_get_state(WINDOW *window);
void           viper_window_set_border_agent(WINDOW *window,VIPER_FUNC agent,
                  gint id);
void				viper_window_show(WINDOW *window);
void				viper_window_touch(WINDOW *window);
void				viper_window_redraw(WINDOW *window);
#define			viper_window_unhide(window)   \
                  (viper_window_set_state(window,STATE_VISIBLE))
#define			viper_window_focus(window)    \
                  (viper_window_set_state(window,STATE_FOCUS))
#define			viper_window_hide(window)     \
                  (viper_window_set_state(window,STATE_UNSET | STATE_VISIBLE))

/* kmio faclilities (keyboard & mouse i/o)   */
gint32			viper_kmio_fetch(MEVENT *mouse_event);
void				viper_kmio_dispatch(gint32 keystroke,MEVENT *mouse_event);
void           viper_kmio_dispatch_set_hook(gint sequence,VIPER_KMIO_HOOK hook);
void				viper_window_set_key_func(WINDOW *window,VIPER_WKEY_FUNC func);

/* event handling */
gint				viper_event_set(WINDOW *window,gchar *event,
						VIPER_FUNC func,gpointer arg);
gint 				viper_event_del(WINDOW *window,gchar *event);
gint           viper_event_exec(WINDOW *window,gchar *event,gpointer anything);
#define        viper_event_run(window,event) \
                  (viper_event_exec(window,event,NULL))
#define			VIPER_EVENT_BROADCAST		   ((WINDOW*)"ALL_VIPER_WINDOWS")
#define 			VIPER_EVENT_WINDOW_DESIST	   (viper_window_destroy(window))
#define			VIPER_EVENT_WINDOW_PERSIST	   0
void 				viper_window_for_each(VIPER_FUNC func,gpointer arg,gint vector);

/* viper window deck functions */
void				viper_deck_cycle(gint vector);
WINDOW*			viper_deck_hit_test(gint x,gint y);
gchar**			viper_deck_get_wndlist(void);

/* menu helpers */
MENU*				viper_menu_create(gchar **items);
void           viper_menu_items_add(MENU *menu,gchar **items);
void           viper_menu_items_change(MENU *menu,gchar **items);
WINDOW*        viper_menu_bind(MENU *menu,WINDOW *parent,gfloat x,gfloat y,
						gfloat width,gfloat height);
void           viper_menu_destroy(MENU *menu,gboolean free_windows);
#define        CURRENT_MENU_ITEM(menu)    (item_index(current_item(menu)))

/* form helpers */
void           viper_form_colorize(FORM *form,chtype field_active,
                  chtype field_normal,chtype text_active,chtype text_normal);
#define        viper_form_normalize(form,fcolors,tcolors)   \
                  (viper_form_colorize(form,fcolors,fcolors,tcolors,tcolors))
void				viper_form_destroy(FORM *form,gboolean free_windows);
gint           viper_form_driver(FORM *form,gint request,guint32 flags,
                  chtype active,chtype normal,gshort cursor_color);
#define        CURRENT_FORM_ITEM(form)    (field_index(current_field(form)))

/* miscellaneous functions */
void				viper_window_set_userptr(WINDOW *window,gpointer anything);
gpointer			viper_window_get_userptr(WINDOW *window);
#define			WINDOW_FRAME(window)	      (viper_get_window_frame(window))


/* INTERNAL USE ONLY:  core */
VIPER_WND*     viper_get_viper_wnd(WINDOW *window);
VIPER_EVENT*	viper_get_viper_event(WINDOW *window,gchar *event);
WINDOW*			viper_get_window_frame(WINDOW *window);

#endif
