#ifndef _VIPER_FILEDLG
#define _VIPER_FILEDLG

#include <time.h>
#include <unistd.h>

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

typedef struct
{
    gchar   filename[NAME_MAX + 1];
    gchar   filepath[NAME_MAX + 1];
    mode_t  mode;
    off_t   size;
    time_t  ctime;
}
VIPER_FSTATS;

typedef struct
{
    WINDOW  *parent;
    WINDOW  *file_win;
    MENU    *file_menu;
    GSList  *fstats_list;
    gchar   **items;
    WINDOW  *path_win;
    FORM    *path_form;
    WINDOW  *ctrl_win;
    MENU    *ctrl_menu;
    gint32  flags;
}
VIPER_FILEDLG;

#ifndef S_IFDIR
#define S_IFDIR     0040000
#endif

#define FIELD_WIDTH_SIZE        7
#define FIELD_WIDTH_CTIME       20
#define FIELD_WIDTH_PRIV        11

/*    default kbd handlers    */
gint    viper_kbd_default_FILEDLG_MENU(gint32 keystroke, WINDOW *window);
gint    viper_kbd_default_FILEDLG_BUTTONS(gint32 keystroke, WINDOW *window);
gint    viper_kbd_default_FILEDLG_PATH(gint32 keystroke, WINDOW *window);

/* event handlers */
gint    viper_filedlg_ON_MOVE(WINDOW *window, gpointer arg);
gint    viper_filedlg_ON_CLOSE(WINDOW *window, gpointer arg);

/* file helpers   */
GSList* filedlg_readdir(gchar *dir,gint32 flags);
gchar** filedlg_format_items(GSList *fstats_list,gint32 flags,gint width);
void    filedlg_field_size(off_t size, gchar *buffer);
void    filedlg_field_ctime(time_t ctime, gchar *buffer);
void    filedlg_field_permissions(mode_t mode, gchar *buffer);

/* house-keeping functions */
void    g_slist_free_data(gpointer data, gpointer anything);
gint    g_slist_sort_func(gconstpointer a, gconstpointer b);

#endif
