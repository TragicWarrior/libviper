#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <utmpx.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include "viper.h"
#include "viper_color.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_screen.h"

static int
_vk_screen_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_screen_dtor(vk_object_t *object);

static vk_desktop_t*
_vk_desktop_create(SCREEN *term, int width, int height);

static void
_vk_desktop_destroy(vk_desktop_t *desktop);

static pid_t
_vk_screen_evict_pty(const char *pty);



declare_klass(VK_SCREEN_KLASS)
{
    .size = KLASS_SIZE(vk_screen_t),
    .name = KLASS_NAME(vk_screen_t),
    .ctor = _vk_screen_ctor,
    .dtor = _vk_screen_dtor,
};


vk_screen_t*
vk_screen_create(void)
{
    vk_screen_t *screen;

    screen = (vk_screen_t *)vk_object_construct(VK_SCREEN_KLASS);

    return screen;
}

int
vk_screen_add_desktop(vk_screen_t *screen)
{
    vk_desktop_t    *desktop;
    vk_desktop_t    **new_array;
    int             id;

    if(screen == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    desktop = _vk_desktop_create(screen->term, screen->width, screen->height);
    if(desktop == NULL) return -1;

    id = screen->desktop_count;

    new_array = realloc(screen->desktops,
        (id + 1) * sizeof(vk_desktop_t *));
    if(new_array == NULL)
    {
        _vk_desktop_destroy(desktop);
        return -1;
    }

    screen->desktops = new_array;
    screen->desktops[id] = desktop;
    screen->desktop_count++;

    return id;
}

int
vk_screen_del_desktop(vk_screen_t *screen, int id)
{
    vk_desktop_t    **new_array;
    int             i;

    if(screen == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(id < 0 || id >= screen->desktop_count) return -1;

    if(screen->desktop_count == 1) return -1;

    _vk_desktop_destroy(screen->desktops[id]);

    for(i = id; i < screen->desktop_count - 1; i++)
        screen->desktops[i] = screen->desktops[i + 1];

    screen->desktop_count--;

    new_array = realloc(screen->desktops,
        screen->desktop_count * sizeof(vk_desktop_t *));
    if(new_array != NULL)
        screen->desktops = new_array;

    if(screen->active_desktop >= screen->desktop_count)
        screen->active_desktop = screen->desktop_count - 1;

    return 0;
}

int
vk_screen_switch_desktop(vk_screen_t *screen, int id)
{
    if(screen == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(id < 0 || id >= screen->desktop_count) return -1;

    screen->active_desktop = id;

    return 0;
}

WINDOW*
vk_screen_get_window(vk_screen_t *screen)
{
    vk_desktop_t    *desktop;

    if(screen == NULL) return NULL;

    if(!vk_object_assert(screen, vk_screen_t)) return NULL;

    if(screen->active_desktop < 0 ||
        screen->active_desktop >= screen->desktop_count)
        return NULL;

    desktop = screen->desktops[screen->active_desktop];

    return desktop->canvas;
}

int
vk_screen_attach_widget(vk_screen_t *screen, int desktop_id,
    vk_widget_t *widget)
{
    vk_desktop_t    *desktop;
    vk_widget_t     **new_array;

    if(screen == NULL || widget == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(desktop_id < 0 || desktop_id >= screen->desktop_count) return -1;

    desktop = screen->desktops[desktop_id];

    if(desktop->widget_count >= desktop->widget_alloc)
    {
        int new_alloc = desktop->widget_alloc == 0 ? 8 : desktop->widget_alloc * 2;

        new_array = realloc(desktop->widgets,
            new_alloc * sizeof(vk_widget_t *));
        if(new_array == NULL) return -1;

        desktop->widgets = new_array;
        desktop->widget_alloc = new_alloc;
    }

    desktop->widgets[desktop->widget_count] = widget;
    desktop->widget_count++;

    vk_widget_set_surface(widget, desktop->canvas);

    return 0;
}

int
vk_screen_detach_widget(vk_screen_t *screen, int desktop_id,
    vk_widget_t *widget)
{
    vk_desktop_t    *desktop;
    int             i;
    bool            found = false;

    if(screen == NULL || widget == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(desktop_id < 0 || desktop_id >= screen->desktop_count) return -1;

    desktop = screen->desktops[desktop_id];

    for(i = 0; i < desktop->widget_count; i++)
    {
        if(desktop->widgets[i] == widget)
        {
            found = true;
            break;
        }
    }

    if(!found) return -1;

    for(; i < desktop->widget_count - 1; i++)
        desktop->widgets[i] = desktop->widgets[i + 1];

    desktop->widget_count--;

    widget->surface = NULL;

    return 0;
}

int
vk_screen_resize(vk_screen_t *screen)
{
    vk_desktop_t    *desktop;
    struct winsize  ws;
    int             i;

    if(screen == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(screen->fd_out != NULL &&
        ioctl(fileno(screen->fd_out), TIOCGWINSZ, &ws) == 0)
    {
        resize_term(ws.ws_row, ws.ws_col);
    }

    getmaxyx(stdscr, screen->height, screen->width);

    for(i = 0; i < screen->desktop_count; i++)
    {
        desktop = screen->desktops[i];
        wresize(desktop->canvas, screen->height, screen->width);
        werase(desktop->canvas);
    }

    return 0;
}

int
vk_screen_poll_resize(vk_screen_t *screen)
{
    struct winsize  ws;

    if(screen == NULL) return 0;

    if(!vk_object_assert(screen, vk_screen_t)) return 0;

    if(screen->fd_out == NULL) return 0;

    if(ioctl(fileno(screen->fd_out), TIOCGWINSZ, &ws) < 0) return 0;

    if(ws.ws_col == screen->width && ws.ws_row == screen->height)
        return 0;

    vk_screen_resize(screen);

    return 1;
}

int
vk_screen_refresh(vk_screen_t *screen)
{
    vk_desktop_t    *desktop;

    if(screen == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(screen->active_desktop < 0 ||
        screen->active_desktop >= screen->desktop_count)
        return -1;

    desktop = screen->desktops[screen->active_desktop];

    overwrite(desktop->canvas, stdscr);
    wrefresh(stdscr);

    return 0;
}

int
vk_screen_teleport(vk_screen_t *screen, const char *pty)
{
    SCREEN          *new_term;
    SCREEN          *old_term;
    FILE            *new_out;
    FILE            *new_in;
    FILE            *old_out;
    FILE            *old_in;
    vk_desktop_t    *desktop;
    int             i;
    int             j;

    if(screen == NULL || pty == NULL) return -1;

    if(!vk_object_assert(screen, vk_screen_t)) return -1;

    if(screen->evicted_pid > 0)
    {
        if(screen->has_saved_termios && screen->fd_out != NULL)
            tcsetattr(fileno(screen->fd_out), TCSANOW,
                &screen->saved_termios);

        kill(screen->evicted_pid, SIGCONT);
        kill(screen->evicted_pid, SIGINT);
    }

    screen->evicted_pid = _vk_screen_evict_pty(pty);

    new_out = fopen(pty, "w");
    if(new_out == NULL) return -1;

    new_in = fopen(pty, "r");
    if(new_in == NULL)
    {
        fclose(new_out);
        return -1;
    }

    if(tcgetattr(fileno(new_out), &screen->saved_termios) == 0)
        screen->has_saved_termios = true;
    else
        screen->has_saved_termios = false;

    new_term = newterm(NULL, new_out, new_in);
    if(new_term == NULL)
    {
        fclose(new_in);
        fclose(new_out);
        return -1;
    }

    set_term(new_term);

    viper_color_init();
    keypad(stdscr, TRUE);
    noecho();
    raw();
    curs_set(0);
    scrollok(stdscr, FALSE);

    getmaxyx(stdscr, screen->height, screen->width);

    old_term = screen->term;
    old_in = screen->fd_in;
    old_out = screen->fd_out;

    screen->term = new_term;
    screen->fd_in = new_in;
    screen->fd_out = new_out;

    for(i = 0; i < screen->desktop_count; i++)
    {
        desktop = screen->desktops[i];

        desktop->canvas = newwin(screen->height, screen->width, 0, 0);

        for(j = 0; j < desktop->widget_count; j++)
        {
            vk_widget_recreate(desktop->widgets[j]);
            desktop->widgets[j]->surface = desktop->canvas;
        }
    }

    set_term(old_term);
    endwin();

    if(old_in != stdin && old_in != NULL)
        fclose(old_in);

    if(old_out != stdout && old_out != NULL)
        fclose(old_out);

    set_term(screen->term);
    clearok(stdscr, TRUE);

    // drain stale terminal responses from newterm/keypad init
    wtimeout(stdscr, 100);
    while(wgetch(stdscr) != ERR)
        ;
    wtimeout(stdscr, -1);

    return 0;
}

static pid_t
_vk_find_session_leader_utmpx(const char *line)
{
    struct utmpx    *entry;

    setutxent();

    while((entry = getutxent()) != NULL)
    {
        if(entry->ut_type != USER_PROCESS) continue;

        if(strncmp(entry->ut_line, line, sizeof(entry->ut_line)) == 0)
        {
            pid_t pid = entry->ut_pid;
            endutxent();
            return pid;
        }
    }

    endutxent();
    return -1;
}

static pid_t
_vk_find_session_leader_proc(const char *pty)
{
    struct stat     pty_stat;
    DIR             *proc;
    struct dirent   *entry;
    char            path[280];
    char            buf[512];
    char            *cp;
    FILE            *fp;
    int             tty_nr;
    int             session;
    pid_t           pid;

    if(stat(pty, &pty_stat) < 0) return -1;

    proc = opendir("/proc");
    if(proc == NULL) return -1;

    while((entry = readdir(proc)) != NULL)
    {
        if(entry->d_name[0] < '0' || entry->d_name[0] > '9')
            continue;

        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);

        fp = fopen(path, "r");
        if(fp == NULL) continue;

        if(fgets(buf, sizeof(buf), fp) == NULL)
        {
            fclose(fp);
            continue;
        }

        fclose(fp);

        cp = strrchr(buf, ')');
        if(cp == NULL) continue;

        if(sscanf(cp + 1, " %*c %*d %*d %d %d", &session, &tty_nr) != 2)
            continue;

        pid = (pid_t)atoi(entry->d_name);

        if((dev_t)tty_nr == pty_stat.st_rdev && pid == (pid_t)session)
        {
            closedir(proc);
            return pid;
        }
    }

    closedir(proc);
    return -1;
}

static pid_t
_vk_find_session_leader(const char *pty)
{
    const char  *line;
    pid_t       pid;

    if(pty == NULL) return -1;

    line = (strncmp(pty, "/dev/", 5) == 0) ? pty + 5 : pty;

    pid = _vk_find_session_leader_utmpx(line);
    if(pid > 0) return pid;

    return _vk_find_session_leader_proc(pty);
}

static pid_t
_vk_screen_evict_pty(const char *pty)
{
    pid_t   sid;

    if(pty == NULL) return -1;

    sid = _vk_find_session_leader(pty);
    if(sid <= 1) return -1;

    if(kill(sid, SIGSTOP) < 0) return -1;

    return sid;
}

void
vk_screen_destroy(vk_screen_t *screen)
{
    if(screen == NULL) return;

    if(!vk_object_assert(screen, vk_screen_t)) return;

    screen->dtor(VK_OBJECT(screen));
}


static int
_vk_screen_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_screen_t     *screen;

    (void)argp;

    if(object == NULL) return -1;

    screen = VK_SCREEN(object);

    screen->fd_out = stdout;
    screen->fd_in = stdin;

    screen->term = newterm(NULL, screen->fd_out, screen->fd_in);
    if(screen->term == NULL) return -1;

    set_term(screen->term);

    viper_color_init();

    keypad(stdscr, TRUE);
    noecho();
    raw();
    curs_set(0);
    scrollok(stdscr, FALSE);

    getmaxyx(stdscr, screen->height, screen->width);

    screen->desktops = NULL;
    screen->desktop_count = 0;
    screen->active_desktop = 0;
    screen->evicted_pid = -1;
    screen->has_saved_termios = false;

    screen->ctor = _vk_screen_ctor;
    screen->dtor = _vk_screen_dtor;

    vk_screen_add_desktop(screen);

    return 0;
}

static int
_vk_screen_dtor(vk_object_t *object)
{
    vk_screen_t     *screen;
    int             i;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_screen_t)) return -1;

    screen = VK_SCREEN(object);

    for(i = 0; i < screen->desktop_count; i++)
        _vk_desktop_destroy(screen->desktops[i]);

    free(screen->desktops);
    screen->desktops = NULL;

    if(screen->term != NULL)
    {
        endwin();
        delscreen(screen->term);
        screen->term = NULL;
    }

    if(screen->fd_in != stdin && screen->fd_in != NULL)
        fclose(screen->fd_in);

    if(screen->fd_out != stdout && screen->fd_out != NULL)
    {
        if(screen->has_saved_termios)
            tcsetattr(fileno(screen->fd_out), TCSANOW,
                &screen->saved_termios);

        fclose(screen->fd_out);
    }

    if(screen->evicted_pid > 0)
    {
        kill(screen->evicted_pid, SIGCONT);
        kill(screen->evicted_pid, SIGINT);
    }

    vk_object_demote(object, vk_object_t);
    vk_object_destroy(object);

    return 0;
}

static vk_desktop_t*
_vk_desktop_create(SCREEN *term, int width, int height)
{
    vk_desktop_t    *desktop;
    SCREEN          *prev;

    if(term == NULL) return NULL;

    desktop = calloc(1, sizeof(vk_desktop_t));
    if(desktop == NULL) return NULL;

    prev = set_term(term);

    desktop->canvas = newwin(height, width, 0, 0);
    if(desktop->canvas == NULL)
    {
        set_term(prev);
        free(desktop);
        return NULL;
    }

    set_term(prev);

    desktop->widgets = NULL;
    desktop->widget_count = 0;
    desktop->widget_alloc = 0;

    return desktop;
}

static void
_vk_desktop_destroy(vk_desktop_t *desktop)
{
    if(desktop == NULL) return;

    if(desktop->canvas != NULL)
        delwin(desktop->canvas);

    free(desktop->widgets);
    free(desktop);
}

