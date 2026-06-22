#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <utmpx.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_screen.h"
#include "vk_event.h"

static int
_vk_screen_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_screen_dtor(vk_object_t *object);

static vk_surface_t*
_vk_surface_create(SCREEN *term, int width, int height);

static void
_vk_surface_destroy(vk_surface_t *surface);

static pid_t
_vk_screen_evict_pty(const char *pty);

declare_klass(VK_SCREEN_KLASS)
{
    .size = KLASS_SIZE(vk_screen_t),
    .name = KLASS_NAME(vk_screen_t),
    .ctor = _vk_screen_ctor,
    .dtor = _vk_screen_dtor,
};

inline vk_screen_t*
vk_screen_create(void)
{
    vk_screen_t *screen;

    screen = (vk_screen_t *)vk_object_construct(VK_SCREEN_KLASS);

    return screen;
}

inline int
vk_screen_add_surface(vk_screen_t *screen)
{
    vk_surface_t    *surface;
    vk_surface_t    **new_array;
    int             id;

    if(screen == NULL) return -1;

    surface = _vk_surface_create(screen->term, screen->width, screen->height);
    if(surface == NULL) return -1;

    id = screen->surface_count;

    new_array = realloc(screen->surfaces,
        (id + 1) * sizeof(vk_surface_t *));
    if(new_array == NULL)
    {
        _vk_surface_destroy(surface);
        return -1;
    }

    screen->surfaces = new_array;
    screen->surfaces[id] = surface;
    screen->surface_count++;

    return id;
}

inline int
vk_screen_del_surface(vk_screen_t *screen, int id)
{
    vk_surface_t    **new_array;
    int             i;

    if(screen == NULL) return -1;

    if(id < 0 || id >= screen->surface_count) return -1;

    if(screen->surface_count == 1) return -1;

    _vk_surface_destroy(screen->surfaces[id]);

    for(i = id; i < screen->surface_count - 1; i++)
        screen->surfaces[i] = screen->surfaces[i + 1];

    screen->surface_count--;

    new_array = realloc(screen->surfaces,
        screen->surface_count * sizeof(vk_surface_t *));
    if(new_array != NULL)
        screen->surfaces = new_array;

    if(screen->active_surface >= screen->surface_count)
        screen->active_surface = screen->surface_count - 1;

    return 0;
}

inline int
vk_screen_set_surface(vk_screen_t *screen, int id)
{
    if(screen == NULL) return -1;

    if(id < 0 || id >= screen->surface_count) return -1;

    if(id == screen->active_surface) return 0;

    screen->active_surface = id;

    /*
        push the newly-active surface's bkgd onto stdscr.  stdscr is the
        only WINDOW ncurses converts into the byte stream that the outer
        terminal renders -- when wrefresh emits scroll / insert-line
        optimizations for a heavy redraw, any cell the outer terminal
        exposes during those operations takes stdscr's bkgd.  Without
        this, exposed cells flash as the terminal's default (black)
        between escape sequences.
    */
    vk_screen_apply_stdscr_bkgd(screen);

    vk_object_emit(VK_OBJECT(screen), VK_EVENT_ON_SURFACE_CHANGE);

    return 0;
}

inline int
vk_screen_get_active_surface(vk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return screen->active_surface;
}

inline int
vk_screen_get_surface_count(vk_screen_t *screen)
{
    if(screen == NULL) return -1;

    return screen->surface_count;
}

/*
    Persist a wbkgdset value for the given surface.  The value lives on
    the surface so it can be reapplied to stdscr on active-surface
    change, on live color change, and after teleport (where stdscr is
    recreated by newterm() and would otherwise come up with the default
    bkgd).  stdscr is where the flicker fix actually matters --
    wrefresh's hardware-scroll / insert-delete-line optimizations on
    the outer terminal expose cells whose color is taken from stdscr's
    bkgd.  vk_screen_apply_stdscr_bkgd pushes the value down.

    The bkgd is intentionally NOT applied to surface->canvas itself.
    The surface canvas is fully overpainted every refresh
    (werase -> wallpaper_func paints every cell -> widgets composite),
    so its bkgd is never visible.  Worse, the deck's shadow code
    writes pair-0 cells (vdk_color_init maps white-on-black to pair 0
    for ncurses default-pair compatibility) and ncurses fills pair-0
    cells with the canvas bkgd's pair on write -- which would tint the
    shadow with the surface color.  Keeping wbkgdset off the surface
    canvas avoids that without losing the flicker fix.
*/
int
vk_screen_set_surface_bkgd(vk_screen_t *screen, int surface_id, chtype bkgd)
{
    vk_surface_t    *surface;

    if(screen == NULL) return -1;
    if(surface_id < 0 || surface_id >= screen->surface_count) return -1;

    surface = screen->surfaces[surface_id];
    if(surface == NULL) return -1;

    surface->bkgd = bkgd;

    if(surface_id == screen->active_surface)
        vk_screen_apply_stdscr_bkgd(screen);

    return 0;
}

/*
    Push the active surface's stored bkgd onto stdscr.  stdscr is what
    ncurses translates into the terminal byte stream during wrefresh;
    setting its bkgd ensures that any cell exposed by hardware scroll /
    insert-delete-line optimizations on the outer terminal renders in
    the desktop's color rather than the terminal's default background.
    Called automatically from vk_screen_set_surface (active change),
    vk_screen_set_surface_bkgd (live color change), and vk_screen_teleport
    (new SCREEN gets a fresh stdscr).
*/
int
vk_screen_apply_stdscr_bkgd(vk_screen_t *screen)
{
    vk_surface_t    *surface;

    if(screen == NULL) return -1;
    if(screen->active_surface < 0 ||
        screen->active_surface >= screen->surface_count) return -1;

    surface = screen->surfaces[screen->active_surface];
    if(surface == NULL) return -1;

    wbkgdset(stdscr, surface->bkgd);

    return 0;
}

inline WINDOW*
vk_screen_get_window(vk_screen_t *screen)
{
    vk_surface_t    *surface;

    if(screen == NULL) return NULL;

    if(screen->active_surface < 0 ||
        screen->active_surface >= screen->surface_count)
        return NULL;

    surface = screen->surfaces[screen->active_surface];

    return surface->canvas;
}

inline int
vk_screen_get_fd(vk_screen_t *screen)
{
    if(screen == NULL || screen->fd_out == NULL) return -1;

    return fileno(screen->fd_out);
}

inline int
vk_screen_attach_widget(vk_screen_t *screen, int surface_id,
    vk_widget_t *widget)
{
    vk_surface_t    *surface;
    vk_widget_t     **new_array;

    if(screen == NULL || widget == NULL) return -1;

    if(surface_id < 0 || surface_id >= screen->surface_count) return -1;

    surface = screen->surfaces[surface_id];

    if(surface->widget_count >= surface->widget_alloc)
    {
        int new_alloc = surface->widget_alloc == 0 ? 8 : surface->widget_alloc * 2;

        new_array = realloc(surface->widgets,
            new_alloc * sizeof(vk_widget_t *));
        if(new_array == NULL) return -1;

        surface->widgets = new_array;
        surface->widget_alloc = new_alloc;
    }

    surface->widgets[surface->widget_count] = widget;
    surface->widget_count++;

    vk_widget_set_surface(widget, surface->canvas);

    return 0;
}

inline int
vk_screen_detach_widget(vk_screen_t *screen, int surface_id,
    vk_widget_t *widget)
{
    vk_surface_t    *surface;
    int             i;
    bool            found = false;

    if(screen == NULL || widget == NULL) return -1;

    if(surface_id < 0 || surface_id >= screen->surface_count) return -1;

    surface = screen->surfaces[surface_id];

    for(i = 0; i < surface->widget_count; i++)
    {
        if(surface->widgets[i] == widget)
        {
            found = true;
            break;
        }
    }

    if(!found) return -1;

    for(; i < surface->widget_count - 1; i++)
        surface->widgets[i] = surface->widgets[i + 1];

    surface->widget_count--;

    widget->surface = NULL;

    return 0;
}

inline int
vk_screen_resize(vk_screen_t *screen)
{
    vk_surface_t    *surface;
    struct winsize  ws;
    int             i;

    if(screen == NULL) return -1;

    if(screen->fd_out != NULL &&
        ioctl(fileno(screen->fd_out), TIOCGWINSZ, &ws) == 0)
    {
        resize_term(ws.ws_row, ws.ws_col);
    }

    getmaxyx(stdscr, screen->height, screen->width);

    for(i = 0; i < screen->surface_count; i++)
    {
        surface = screen->surfaces[i];
        wresize(surface->canvas, screen->height, screen->width);
        werase(surface->canvas);
    }

    /* KEY_RESIZE is also how a dtach / abduco reattach reaches us: the
       client sends SIGWINCH on attach and ncurses turns it into
       KEY_RESIZE.  After a reattach -- or after the user ran `reset` on
       the detached tty -- ncurses' model of the physical terminal is
       stale, and two parts of it have to be forced back into sync. */

    /* (1) screen contents: the optimized wrefresh in vk_screen_refresh
       emits a diff against a screen state that no longer holds, leaving
       the terminal showing garbage until something forces a full
       repaint.  Mark stdscr for a clear+repaint on the next wrefresh,
       exactly as the teleport path does (see vk_screen_teleport). */
    clearok(stdscr, TRUE);

    /* (2) cursor visibility: the hardware cursor was hidden once with
       curs_set(0) at startup, but the freshly attached terminal comes
       up showing it.  ncurses suppresses a redundant curs_set(0) (it
       believes the cursor is already hidden on this unchanged SCREEN),
       so toggle through visible to force the civis escape to be
       re-emitted to the new terminal.  The pair is emitted back to back
       before any refresh, so no cursor blink is visible. */
    curs_set(1);
    curs_set(0);

    /* (3) keypad / application cursor key mode: keypad(stdscr, TRUE) at
       startup sent smkx, putting the terminal in application keypad
       mode so arrow keys arrive as \EOA/\EOB/... -- the form ncurses'
       getch translates to KEY_UP/KEY_DOWN/...  The freshly attached
       terminal is in normal cursor mode and sends \E[A/\E[B/... which
       no key_* terminfo entry binds, so getch surfaces the raw bytes
       and arrow keys reach the focused widget as escape-sequence
       garbage.

       The natural recipe -- toggling keypad(stdscr, FALSE/TRUE) to
       coax ncurses into re-emitting rmkx/smkx -- is not reliable
       across ncurses versions: the emission is gated by ncurses'
       internal _keypad_on flag, which can defer or coalesce the
       transition on a SCREEN whose state ncurses believes is already
       correct.  Write smkx straight to the screen fd instead --
       same approach kmio uses for the mouse-enable escapes (which
       survive reattach for exactly this reason) and the teleport
       path uses for the mouse-disable on the outgoing fd. */
    {
        const char *smkx = tigetstr("smkx");
        if(smkx != NULL && smkx != (char *)-1 && screen->fd_out != NULL)
        {
            fputs(smkx, screen->fd_out);
            fflush(screen->fd_out);
        }
    }

    return 0;
}

inline int
vk_screen_set_wallpaper(vk_screen_t *screen, VkSurfaceBkgdFunc func)
{
    if(screen == NULL) return -1;

    screen->wallpaper_func = func;

    return 0;
}

inline int
vk_screen_set_overlay(vk_screen_t *screen, VkSurfaceBkgdFunc func)
{
    if(screen == NULL) return -1;

    screen->overlay_func = func;

    return 0;
}

inline int
vk_screen_refresh(vk_screen_t *screen)
{
    vk_surface_t    *surface;
    int             i;

    if(screen == NULL) return -1;

    if(screen->active_surface < 0 ||
        screen->active_surface >= screen->surface_count)
        return -1;

    surface = screen->surfaces[screen->active_surface];

    werase(surface->canvas);

    if(screen->wallpaper_func != NULL)
        screen->wallpaper_func(screen, screen->active_surface,
            surface->canvas);

    for(i = 0; i < surface->widget_count; i++)
        vk_widget_draw(surface->widgets[i]);

    overwrite(surface->canvas, stdscr);

    if(screen->overlay_func != NULL)
        screen->overlay_func(screen, screen->active_surface, stdscr);

    wrefresh(stdscr);

    return 0;
}

inline int
vk_screen_teleport(vk_screen_t *screen, const char *pty)
{
    SCREEN          *new_term;
    SCREEN          *old_term;
    FILE            *new_out;
    FILE            *new_in;
    FILE            *old_out;
    FILE            *old_in;
    vk_surface_t    *surface;
    int             i;
    int             j;

    if(screen == NULL || pty == NULL) return -1;

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

    for(i = 0; i < screen->surface_count; i++)
    {
        surface = screen->surfaces[i];

        surface->canvas = newwin(screen->height, screen->width, 0, 0);

        for(j = 0; j < surface->widget_count; j++)
        {
            vk_widget_recreate(surface->widgets[j]);
            surface->widgets[j]->surface = surface->canvas;
        }
    }

    /* turn off mouse tracking on the OLD terminal before we walk away
       from it -- the escape was sent via raw stdio at startup so
       ncurses' endwin() doesn't know to disable it, and the shell that
       takes over would otherwise show mouse activity as garbage chars
       (\033[M... / \033[<...).  Clear the whole tracking family
       (1000/1002/1003 + 1006 SGR): ?1003l alone leaves button/wheel
       reporting armed on many terminals.  Mirrors vk_kmio_shutdown. */
    if(old_out != NULL)
    {
        fputs("\033[?1000l\033[?1002l\033[?1003l\033[?1006l", old_out);
        fflush(old_out);
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

    /* stdscr is brand-new on the post-teleport SCREEN -- reapply the
       active surface's bkgd so wrefresh on this stdscr generates terminal
       output that fills exposed cells with the desktop color. */
    vk_screen_apply_stdscr_bkgd(screen);

    vk_object_emit(VK_OBJECT(screen), VK_EVENT_ON_TELEPORT);

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

inline void
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

    setlocale(LC_CTYPE, "");

    screen->term = newterm(NULL, screen->fd_out, screen->fd_in);
    if(screen->term == NULL) return -1;

    set_term(screen->term);

    keypad(stdscr, TRUE);
    noecho();
    raw();
    curs_set(0);
    scrollok(stdscr, FALSE);

    getmaxyx(stdscr, screen->height, screen->width);

    screen->surfaces = NULL;
    screen->surface_count = 0;
    screen->active_surface = 0;
    screen->evicted_pid = -1;
    screen->has_saved_termios = false;

    screen->ctor = _vk_screen_ctor;
    screen->dtor = _vk_screen_dtor;

    vk_screen_add_surface(screen);

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

    for(i = 0; i < screen->surface_count; i++)
        _vk_surface_destroy(screen->surfaces[i]);

    free(screen->surfaces);
    screen->surfaces = NULL;

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

static vk_surface_t*
_vk_surface_create(SCREEN *term, int width, int height)
{
    vk_surface_t    *surface;
    SCREEN          *prev;

    if(term == NULL) return NULL;

    surface = calloc(1, sizeof(vk_surface_t));
    if(surface == NULL) return NULL;

    prev = set_term(term);

    surface->canvas = newwin(height, width, 0, 0);
    if(surface->canvas == NULL)
    {
        set_term(prev);
        free(surface);
        return NULL;
    }

    set_term(prev);

    surface->widgets = NULL;
    surface->widget_count = 0;
    surface->widget_alloc = 0;

    return surface;
}

static void
_vk_surface_destroy(vk_surface_t *surface)
{
    if(surface == NULL) return;

    if(surface->canvas != NULL)
        delwin(surface->canvas);

    free(surface->widgets);
    free(surface);
}

