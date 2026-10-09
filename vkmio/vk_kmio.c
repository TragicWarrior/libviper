#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>

#include <ncursesw/curses.h>

#include "vkmio.h"

#if !defined(_NO_GPM) && defined(__linux)
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
    The gpm daemon's wire format.  vkmio talks to the daemon itself --
    there is no libgpm here -- so these mirror <gpm.h> rather than
    include it.  The protocol is two fixed-size records over a Unix
    stream socket: the client writes one connect record, the daemon then
    writes one event record per mouse event until either side closes.
*/
#define VK_GPM_SOCKET       "/dev/gpmctl"

/* event.buttons */
#define GPM_B_RIGHT         (1 << 0)    // 1
#define GPM_B_MIDDLE        (1 << 1)    // 2
#define GPM_B_LEFT          (1 << 2)    // 4
#define GPM_B_FOURTH        (1 << 3)    // 8
#define GPM_B_UP            (1 << 4)    // 16
#define GPM_B_DOWN          (1 << 5)    // 32

/* event.type, and the masks in the connect record */
#define GPM_MOVE            (1 << 0)    // 1
#define GPM_DRAG            (1 << 1)    // 2
#define GPM_DOWN            (1 << 2)    // 4
#define GPM_UP              (1 << 3)    // 8
/* (the daemon also sets single/double/triple-click bits 4/5/6 -- 16,
   32, 64 -- and a "moved during the click" bit 7 -- 128.  they are not
   read: a click is
   reported as its press and its release, as an xterm reports it, and
   double-clicks are a matter of timing for the caller -- vk_dblclick) */

/* event.modifiers: the kernel's shift-state bits */
#define VK_GPM_MOD_SHIFT    (1 << 0)    // 1
#define VK_GPM_MOD_CTRL     (1 << 2)    // 4
#define VK_GPM_MOD_ALT      (1 << 3)    // 8

struct vk_gpm_connect_s
{
    unsigned short  event_mask;     /* events we want delivered */
    unsigned short  default_mask;   /* events the daemon keeps handling */
    unsigned short  min_mod;        /* modifier range we take events for */
    unsigned short  max_mod;
    int             pid;
    int             vc;             /* virtual console, 1-63 */
};

struct vk_gpm_event_s
{
    unsigned char   buttons;
    unsigned char   modifiers;
    unsigned short  vc;
    short           dx, dy;         /* movement in this event */
    short           x, y;           /* position, 1-based */
    int             type;
    int             clicks;
    int             margin;
    short           wdx, wdy;       /* wheel movement in this event */
};

/* both records are read and written whole: a layout that drifts from
   the daemon's would garble every event */
_Static_assert(sizeof(struct vk_gpm_connect_s) == 16,
    "gpm connect record must be 16 bytes");
_Static_assert(sizeof(struct vk_gpm_event_s) == 28,
    "gpm event record must be 28 bytes");

#endif

static uint32_t     vk_kmio_flags = 0;
static int          vk_kmio_tty_fd = -1;    /* the fd given to vk_kmio_init */

/* GPM connection state (used by vk_kmio_gpm and vk_kmio_gpm_fd below).
   mio_fd is the daemon socket, or -1.  mio_off says "do not try": there
   is no console, no daemon, or the daemon dropped us.  It keeps a
   failure from being retried on every fetch -- this runs many times a
   second -- and is cleared by VK_GPM_CMD_CLOSE, which is how a changed
   terminal gets a fresh attempt. */
static int          mio_fd = -1;
static bool         mio_off = false;
static MEVENT       *last_mouse_event = NULL;

/* SGR mouse parser state.  Under mousemask(0) ncurses still returns
   KEY_MOUSE for the \033[< introducer but, with no mask armed, leaks
   the Cb;Cx;Cy(M|m) body as raw getch() bytes instead of cooking a
   (mis-decoded) event.  We accumulate that body here and decode it
   ourselves.  Touched only by vk_kmio_fetch on the cooperative input
   protothread, so it needs no locking. */
static char         sgr_buf[16];
static int          sgr_len = 0;
static int          sgr_stall = 0;
static bool         sgr_in_body = false;

/* Upper bound on how many fetches a partially-read SGR body may wait for
   its remainder before we give up on it.  A genuine split read completes
   in a fetch or two; the cap only matters if a terminal sends half an
   escape and then nothing, so a stuck sequence can never starve the
   keyboard path. */
#define VK_KMIO_SGR_MAX_STALL   32

#define VK_KMIO_PASTE_MAX       (256 * 1024)

static char         *paste_buf = NULL;
static size_t       paste_len = 0;
static size_t       paste_cap = 0;
static int          paste_in_body = 0;
static char         paste_hold[6];
static int          paste_hold_len = 0;

static void
paste_reset(void)
{
    paste_in_body = 0;
    paste_hold_len = 0;
    paste_len = 0;
}

static int
paste_append(const char *p, size_t n)
{
    char    *neu;
    size_t  cap;

    if(n == 0) return 0;
    if(paste_len + n > VK_KMIO_PASTE_MAX) return -1;

    if(paste_len + n > paste_cap)
    {
        cap = paste_cap ? paste_cap * 2 : 4096;
        while(cap < paste_len + n) cap *= 2;
        if(cap > VK_KMIO_PASTE_MAX) cap = VK_KMIO_PASTE_MAX;

        neu = (char *)realloc(paste_buf, cap + 1);
        if(neu == NULL) return -1;

        paste_buf = neu;
        paste_cap = cap;
    }

    memcpy(paste_buf + paste_len, p, n);
    paste_len += n;
    paste_buf[paste_len] = '\0';
    return 0;
}

/*
    Feed one byte of a bracketed-paste body.  Returns 1 when ESC[201~
    completes the paste, 0 if more bytes are needed, -1 on overflow.
*/
static int
paste_feed(int c)
{
    /* hold ESC [ 2 0 1 until '~' confirms the terminator */
    if(c == 0x1b && paste_hold_len == 0)
    {
        paste_hold[paste_hold_len++] = (char)c;
        return 0;
    }

    if(paste_hold_len > 0)
    {
        static const char term[] = "\033[201~";

        if(paste_hold_len < 6 && (char)c == term[paste_hold_len])
        {
            paste_hold[paste_hold_len++] = (char)c;
            if(paste_hold_len == 6)
            {
                paste_hold_len = 0;
                return 1;
            }
            return 0;
        }

        /* false alarm -- hold was payload */
        if(paste_append(paste_hold, (size_t)paste_hold_len) != 0)
            return -1;
        paste_hold_len = 0;

        if(c == 0x1b)
        {
            paste_hold[paste_hold_len++] = (char)c;
            return 0;
        }
    }

    {
        char ch = (char)c;

        return paste_append(&ch, 1);
    }
}

static void
_vk_kmio_write(int fd, const char *esc)
{
    if(fd < 0 || esc == NULL) return;

    /* short, blocking write to a tty fd; ignore short-write since
       these escapes are tiny and partial delivery is unrecoverable. */
    (void)!write(fd, esc, strlen(esc));
}

int
vk_kmio_init(int fd, uint32_t flags)
{
    /* the GPM console is worked out from this terminal.  A different
       one (teleport, adopt) means the old verdict -- connected, or not
       available -- no longer holds: start over. */
    if(fd != vk_kmio_tty_fd)
    {
        vk_kmio_tty_fd = fd;
        vk_kmio_gpm_reset();
    }

    vk_kmio_flags = flags;

    /* drop any half-read SGR body if init runs mid-session (teleport
       calls shutdown+init against the new fd) */
    sgr_in_body = false;
    sgr_len = 0;
    sgr_stall = 0;
    paste_reset();

    if(flags & VK_KMIO_BRACKET_PASTE)
        _vk_kmio_write(fd, "\033[?2004h");

    if(flags & VK_KMIO_MOUSE)
    {
        const char  *kmous = tigetstr("kmous");

        /* mask 0 on purpose: we decode SGR mouse reports ourselves off
           the bytes ncurses leaks (see vk_kmio_fetch).  With a non-zero
           mask ncurses instead cooks the report, and its decoder masks
           the motion bit (bit 5) off the button code -- surfacing SGR
           motion as BUTTON1_RELEASED and SGR drag as BUTTON1_PRESSED,
           the misclassification that broke hover-highlight and drags
           over SSH.  1006h selects SGR encoding; 1003h (hover) reports
           every motion event, 1000h reports button events only. */
        mousemask(0, NULL);

        /* only ask for reports the current terminal type can hand back:
           fetch relies on ncurses turning the SGR introducer into
           KEY_MOUSE, which it does only when the type's kmous is
           exactly \033[< (the xterm family).  Any other type -- "linux"
           and screen/tmux/rxvt have kmous=\033[M -- would pass every
           report through as loose keystrokes, and with hover tracking
           that is a flood on each mouse move.  That mismatch is real
           after an adopt: a screen driven as "linux" can be sitting on
           an xterm.  No reports beats garbage. */
        if(kmous != NULL && kmous != (char *)-1
            && strcmp(kmous, "\033[<") == 0)
        {
            if(flags & VK_KMIO_MOUSE_HOVER)
                _vk_kmio_write(fd, "\033[?1003h\033[?1006h");
            else
                _vk_kmio_write(fd, "\033[?1000h\033[?1006h");
        }
    }

    return 0;
}

void
vk_kmio_shutdown(int fd)
{
#if !defined(_NO_GPM) && defined(__linux)
    vk_kmio_gpm(NULL, VK_GPM_CMD_CLOSE);
#endif

    if(vk_kmio_flags & VK_KMIO_MOUSE)
    {
        /* Reset the WHOLE tracking family, not just the mode we set.
           init enables any-event tracking (?1003h), but on many
           terminals ?1003l only stops motion reports -- button and
           wheel reporting survive, so clicks/wheel still leak into the
           shell as glyphs after the host exits.  Clear 1000/1002/1003
           plus the 1006 SGR encoding so nothing is left armed however
           the terminal models the modes; resetting a mode that was
           never set is a harmless no-op. */
        _vk_kmio_write(fd, "\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
    }

    if(vk_kmio_flags & VK_KMIO_BRACKET_PASTE)
        _vk_kmio_write(fd, "\033[?2004l");

    paste_reset();
    vk_kmio_flags = 0;
}

/* the SGR button code ("Cb"): what one mouse event is.  the low two
   bits pick the button unless a wheel or motion bit says otherwise. */
#define VK_SGR_BUTTON_MASK  ((1 << 0) | (1 << 1))   // 3: 0, 1, 2 = button 1, 2, 3
#define VK_SGR_NO_BUTTON    ((1 << 0) | (1 << 1))   // 3: motion with none held
#define VK_SGR_WHEEL_DOWN   (1 << 0)                // 1: with VK_SGR_WHEEL: down, else up
#define VK_SGR_SHIFT        (1 << 2)                // 4
#define VK_SGR_ALT          (1 << 3)                // 8
#define VK_SGR_CTRL         (1 << 4)                // 16
#define VK_SGR_MOTION       (1 << 5)                // 32: hover or drag
#define VK_SGR_WHEEL        (1 << 6)                // 64

/*
    THE mouse decoder: one mouse event, in xterm's SGR terms, into an
    MEVENT.  Both sources end here -- an xterm's reports are parsed from
    text by _vk_kmio_parse_sgr just below, the console's gpm records are
    translated by _vk_kmio_gpm_translate -- so what a press, a release,
    a drag or a wheel notch means to the caller is decided in exactly
    one place, and the two cannot drift apart.

    cb is the SGR button code (the VK_SGR_* bits above): bit 6 marks a
    wheel event (bit 0 clear = up, set = down), bit 5 marks motion
    (hover or drag alike), otherwise the low two bits select the button
    (0 = 1, 1 = 2, 2 = 3) and `release` says which way it went.  Bits
    2/3/4 are shift/alt/ctrl.  The event describes itself, so there is nothing
    to infer: no held-button tracking, no timestamps, no click counting.
    cx/cy are 1-based; MEVENT is 0-based.

    Returns false for a code that is not a mouse event.
*/
static bool
_vk_kmio_decode_mouse(int cb, int cx, int cy, bool release, MEVENT *m)
{
    mmask_t bstate;

    if(m == NULL) return false;

    if(cb & VK_SGR_WHEEL)           /* a wheel notch, up or down */
        bstate = (cb & VK_SGR_WHEEL_DOWN) ? BUTTON5_PRESSED : BUTTON4_PRESSED;
    else if(cb & VK_SGR_MOTION)     /* motion -- hover or drag alike */
        bstate = REPORT_MOUSE_POSITION;
    else switch(cb & VK_SGR_BUTTON_MASK)    /* discrete press / release */
    {
        case 0:  bstate = release ? BUTTON1_RELEASED : BUTTON1_PRESSED; break;
        case 1:  bstate = release ? BUTTON2_RELEASED : BUTTON2_PRESSED; break;
        case 2:  bstate = release ? BUTTON3_RELEASED : BUTTON3_PRESSED; break;
        default: return false;      /* "no button" without the motion bit */
    }

    if(cb & VK_SGR_SHIFT) bstate |= BUTTON_SHIFT;
    if(cb & VK_SGR_ALT)   bstate |= BUTTON_ALT;
    if(cb & VK_SGR_CTRL)  bstate |= BUTTON_CTRL;

    memset(m, 0, sizeof(*m));
    m->bstate = bstate;
    m->x = cx - 1;
    m->y = cy - 1;
    return true;
}

/* The xterm front end: an accumulated SGR report body, "Cb;Cx;Cy" (the
   text between the already-stripped \033[< introducer and the
   terminator), plus the terminator itself -- 'M' for press or motion,
   'm' for release.  Returns true on a well-formed report. */
static bool
_vk_kmio_parse_sgr(const char *body, char term, MEVENT *m)
{
    int     cb, cx, cy;

    if(sscanf(body, "%d;%d;%d", &cb, &cx, &cy) != 3) return false;

    return _vk_kmio_decode_mouse(cb, cx, cy, term == 'm', m);
}

/* Accumulate the leaked SGR body bytes from the input queue into sgr_buf
   until the M/m terminator.  Pure accumulator: it owns sgr_buf/sgr_len but
   never touches sgr_in_body -- vk_kmio_fetch owns that lifecycle.  Returns
   1 when a complete report was parsed into mouse_event, 0 when input ran
   dry (the partial is retained in sgr_buf for a later resume), and -1 on a
   malformed or oversized body (the partial is discarded). */
static int
_vk_kmio_drain_sgr(MEVENT *mouse_event)
{
    int     c;

    for(;;)
    {
        c = getch();

        if(c == -1)
            return 0;               /* ran dry; keep the partial */

        if(c == 'M' || c == 'm')
        {
            int ok;

            sgr_buf[sgr_len] = '\0';
            ok = _vk_kmio_parse_sgr(sgr_buf, (char)c, mouse_event);
            sgr_len = 0;
            return ok ? 1 : -1;
        }

        if((c >= '0' && c <= '9') || c == ';')
        {
            if(sgr_len >= (int)sizeof(sgr_buf) - 1)
            {
                sgr_len = 0;                /* overflow -- discard */
                return -1;
            }
            sgr_buf[sgr_len++] = (char)c;
            continue;
        }

        sgr_len = 0;                        /* unexpected byte -- discard */
        return -1;
    }
}

/* Run the accumulator and resolve the body-read state.  Returns KEY_MOUSE
   when a report is ready, otherwise -1.  sgr_in_body is kept set ONLY when
   a real partial body is still arriving (sgr_len > 0) and we are under the
   stall cap; a KEY_MOUSE that leaks no body at all (e.g. another layer
   re-armed mousemask so ncurses cooked the event) is abandoned at once, so
   it can never wedge the input path. */
static int32_t
_vk_kmio_pump_sgr(MEVENT *mouse_event)
{
    switch(_vk_kmio_drain_sgr(mouse_event))
    {
        case 1:
            sgr_in_body = false;
            sgr_stall = 0;
            return KEY_MOUSE;

        case 0:
            if(sgr_len > 0 && ++sgr_stall < VK_KMIO_SGR_MAX_STALL)
                return -1;                  /* genuine split; resume later */
            /* fall through -- no body leaked, or stalled too long */

        default:                            /* malformed, or give-up above */
            sgr_in_body = false;
            sgr_len = 0;
            sgr_stall = 0;
            return -1;
    }
}

int32_t
vk_kmio_fetch(MEVENT *mouse_event)
{
    int32_t         keystroke = -1;
    int32_t         key_code = 0;
    uint8_t         shift_op = 4;

    last_mouse_event = mouse_event;

#if !defined(_NO_GPM) && defined(__linux)
    if(vk_kmio_gpm(mouse_event, 0) == 0)
        return KEY_MOUSE;
#endif

    /* finish a split SGR body left over from a previous call before
       reading any new input */
    if(sgr_in_body)
        return _vk_kmio_pump_sgr(mouse_event);

    /* finish a split bracketed-paste body */
    if(paste_in_body)
    {
        for(;;)
        {
            int c = getch();
            int rc;

            if(c == -1) return -1;

            rc = paste_feed(c);
            if(rc == 1)
            {
                paste_in_body = 0;
                return VK_KMIO_PASTE;
            }
            if(rc < 0)
            {
                paste_reset();
                return -1;
            }
        }
    }

    key_code = getch();

    if(key_code != -1)
    {
        if(key_code != 27)
        {
            if(key_code == KEY_MOUSE)
            {
                /* ncurses stripped the \033[< introducer and left the
                   Cb;Cx;Cy(M|m) body in the queue; decode it ourselves */
                sgr_in_body = true;
                sgr_len = 0;
                sgr_stall = 0;
                return _vk_kmio_pump_sgr(mouse_event);
            }
            return key_code;
        }

        /* ESC: look ahead for [200~ (bracketed-paste start) */
        if(vk_kmio_flags & VK_KMIO_BRACKET_PASTE)
        {
            int     tmp[5];
            int     n = 0;
            int     c;
            int     i;
            int     match = 1;
            static const char start[] = "[200~";

            for(i = 0; i < 5; i++)
            {
                c = getch();
                if(c == -1)
                {
                    match = 0;
                    break;
                }
                /* a key code (KEY_MOUSE, KEY_RESIZE, an arrow...) is
                   the start of the NEXT event, not more of this ESC:
                   with hover tracking a mouse report can land right
                   behind a lone ESC.  Hand it back so the next fetch
                   sees it whole; swallowing it here left the report's
                   body in the queue to be read as typed characters,
                   and turned the ESC into something else. */
                if(c > 255)
                {
                    ungetch(c);
                    match = 0;
                    break;
                }
                tmp[n++] = c;
                if(c != (unsigned char)start[i])
                {
                    match = 0;
                    break;
                }
            }

            if(match && n == 5)
            {
                paste_reset();
                paste_in_body = 1;
                /* drain whatever of the payload is already queued */
                for(;;)
                {
                    int rc;

                    c = getch();
                    if(c == -1) return -1;

                    rc = paste_feed(c);
                    if(rc == 1)
                    {
                        paste_in_body = 0;
                        return VK_KMIO_PASTE;
                    }
                    if(rc < 0)
                    {
                        paste_reset();
                        return -1;
                    }
                }
            }

            /* not a paste start -- pack ESC + whatever we consumed */
            keystroke = 27;
            shift_op = 4;
            for(i = 0; i < n; i++)
            {
                shift_op = shift_op << 1;
                if(shift_op >= 32) break;
                keystroke |= (tmp[i] << shift_op);
            }
            return keystroke;
        }

        keystroke = 27;
        do
        {
            shift_op = shift_op << 1;
            key_code = getch();
            if(key_code == -1) break;
            /* same as above: a key code belongs to the next event */
            if(key_code > 255)
            {
                ungetch(key_code);
                break;
            }
            keystroke |= (key_code << shift_op);
        }
        while(shift_op < 24);
    }

    return keystroke;
}

const char *
vk_kmio_get_paste(size_t *len)
{
    if(len != NULL) *len = paste_len;

    if(paste_buf == NULL || paste_len == 0)
        return NULL;

    return paste_buf;
}

MEVENT*
vk_kmio_get_mouse_event(void)
{
    return last_mouse_event;
}

int
vk_kmio_mouse_drain(MEVENT *mouse_event)
{
#if !defined(_NO_GPM) && defined(__linux)
    return vk_kmio_gpm(mouse_event, VK_GPM_CMD_DRAIN);
#else
    (void)mouse_event;
    return -1;
#endif
}

/* see vkmio.h -- the daemon socket for an event loop to wait on. */
int
vk_kmio_gpm_fd(void)
{
#if !defined(_NO_GPM) && defined(__linux)
    return mio_fd;
#else
    return -1;
#endif
}

/* see vkmio.h -- forget the GPM connection and why it was unavailable. */
void
vk_kmio_gpm_reset(void)
{
#if !defined(_NO_GPM) && defined(__linux)
    vk_kmio_gpm(NULL, VK_GPM_CMD_CLOSE);
#endif
}

#if !defined(_NO_GPM) && defined(__linux)
/*
    The virtual console to ask gpm for, or 0 when there is none.

    VK_GPM_VC (1-63) wins: under dtach or screen the screen is on a pty,
    and only a launcher that still sees the real console can name it.
    Otherwise the console is the terminal the screen is on, if that is
    /dev/ttyN.  Anything else -- an X terminal, an SSH login -- has no
    console, and we do not connect at all: the daemon would only drop a
    client that asks for a console it does not own.
*/
static int
_vk_kmio_gpm_vc(void)
{
    const char  *env = getenv("VK_GPM_VC");
    const char  *tty;
    char        *end;
    long        vc;

    if(env != NULL && *env != '\0')
    {
        vc = strtol(env, &end, 10);
        if(*end == '\0' && vc >= 1 && vc <= 63) return (int)vc;
    }

    tty = ttyname((vk_kmio_tty_fd >= 0) ? vk_kmio_tty_fd : STDIN_FILENO);
    if(tty == NULL || strncmp(tty, "/dev/tty", 8) != 0) return 0;
    if(tty[8] < '0' || tty[8] > '9') return 0;

    vc = strtol(tty + 8, &end, 10);
    if(*end != '\0' || vc < 1 || vc > 63) return 0;

    return (int)vc;
}

/*
    Connect to the gpm daemon as the client for console `vc`.  Returns
    the socket, non-blocking, or -1.

    The daemon accepts the connection first and judges it afterwards:
    it reads the connect record, and if the caller does not own
    /dev/tty<vc> it just closes the socket.  So success here is not yet
    acceptance; a refusal shows up as end-of-file on the first read.
*/
static int
_vk_kmio_gpm_connect(int vc)
{
    struct vk_gpm_connect_s conn;
    struct sockaddr_un      addr;
    int                     fd;
    int                     fflags;

    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VK_GPM_SOCKET, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        close(fd);              /* no daemon: gpm is not running */
        return -1;
    }

    /* take movement, drags and button changes for every modifier
       combination, and leave nothing for the daemon's own selection
       handling on this console */
    memset(&conn, 0, sizeof(conn));
    conn.event_mask = GPM_MOVE | GPM_DRAG | GPM_DOWN | GPM_UP;
    conn.default_mask = 0;
    conn.min_mod = 0;
    conn.max_mod = (unsigned short)~0;
    conn.pid = (int)getpid();
    conn.vc = vc;

    if(write(fd, &conn, sizeof(conn)) != (ssize_t)sizeof(conn))
    {
        close(fd);
        return -1;
    }

    fflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, fflags | O_NONBLOCK);

    /* optional: have the kernel raise SIGIO when an event is waiting */
    if(vk_kmio_flags & VK_KMIO_GPM_SIGIO)
    {
        fcntl(fd, F_SETOWN, getpid());
        fflags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, fflags | FASYNC);
    }

    return fd;
}

/*
    The console front end: one gpm event record, restated as the SGR
    button code an xterm would have sent for the same thing, and handed
    to _vk_kmio_decode_mouse.  Returns false for a record that is not a
    mouse event we report (no button change, movement or wheel in it,
    or a change of several buttons at once).

    The order of the tests is the order of precedence: a wheel notch
    first (some drivers deliver it together with a movement), then
    movement, then a button going down or up.
*/
static bool
_vk_kmio_gpm_translate(const struct vk_gpm_event_s *ev, MEVENT *m)
{
    int     cb;
    bool    release = false;

    if(ev->wdy > 0 || ev->buttons == GPM_B_UP || ev->buttons == GPM_B_FOURTH)
    {
        /* wheel up.  exps2/imps2 mice report scroll in wdy; the
           GPM_B_UP/DOWN "buttons" are the legacy ms3 mechanism */
        cb = VK_SGR_WHEEL;
    }
    else if(ev->wdy < 0 || ev->buttons == GPM_B_DOWN)
    {
        cb = VK_SGR_WHEEL | VK_SGR_WHEEL_DOWN;
    }
    else if(ev->type & (GPM_MOVE | GPM_DRAG))
    {
        /* motion; the decoder does not ask which button is held */
        cb = VK_SGR_MOTION | VK_SGR_NO_BUTTON;
    }
    else if(ev->type & (GPM_DOWN | GPM_UP))
    {
        /* gpm names the one button that changed */
        switch(ev->buttons)
        {
            case GPM_B_LEFT:    cb = 0; break;
            case GPM_B_MIDDLE:  cb = 1; break;
            case GPM_B_RIGHT:   cb = 2; break;
            default:            return false;
        }

        release = (ev->type & GPM_UP) != 0;
    }
    else
        return false;

    /* modifier keys held at the time, into SGR's bits */
    if(ev->modifiers & VK_GPM_MOD_SHIFT) cb |= VK_SGR_SHIFT;
    if(ev->modifiers & VK_GPM_MOD_ALT)   cb |= VK_SGR_ALT;
    if(ev->modifiers & VK_GPM_MOD_CTRL)  cb |= VK_SGR_CTRL;

    /* gpm positions are 1-based, like SGR's */
    return _vk_kmio_decode_mouse(cb, ev->x, ev->y, release, m);
}

/*
    The GPM half of the mouse input.  cmd 0 reads one event if there is
    one (waiting up to 1ms), VK_GPM_CMD_DRAIN reads one only if it is
    already queued, VK_GPM_CMD_CLOSE drops the connection.  Returns 0
    with *mouse_event filled in, or -1.
*/
int
vk_kmio_gpm(MEVENT *mouse_event, uint16_t cmd)
{
    struct pollfd           mio_poll;
    struct vk_gpm_event_s   g_event;
    ssize_t                 got;
    int                     vc;

    if(cmd == VK_GPM_CMD_CLOSE)
    {
        if(mio_fd >= 0) close(mio_fd);
        mio_fd = -1;
        mio_off = false;
        return 0;
    }

    if(mouse_event == NULL) return -1;

    if(mio_off) return -1;

    if(mio_fd == -1)
    {
        vc = _vk_kmio_gpm_vc();
        if(vc > 0) mio_fd = _vk_kmio_gpm_connect(vc);

        if(mio_fd == -1)
        {
            mio_off = true;
            return -1;
        }
    }

    memset(&mio_poll, 0, sizeof(mio_poll));
    mio_poll.events = POLLIN;
    mio_poll.fd = mio_fd;

    /* how long to wait for an event: not at all when draining or when
       the caller has its own event loop (VK_KMIO_NOWAIT); otherwise 1ms,
       which paces a caller that just loops on fetch */
    if(poll(&mio_poll, 1,
        (cmd == VK_GPM_CMD_DRAIN || (vk_kmio_flags & VK_KMIO_NOWAIT))
            ? 0 : 1) < 1)
        return -1;

    /* one event is one whole record.  The daemon writes each with a
       single write(), so anything else is the end of the conversation:
       0 is the daemon closing (it refused us, or it is shutting down),
       a short or failed read is a connection we cannot trust. */
    got = read(mio_fd, &g_event, sizeof(g_event));

    if(got != (ssize_t)sizeof(g_event))
    {
        if(got < 0 && (errno == EAGAIN || errno == EINTR)) return -1;

        close(mio_fd);
        mio_fd = -1;
        mio_off = true;
        return -1;
    }

    /* say what it was in the one vocabulary both sources share */
    if(!_vk_kmio_gpm_translate(&g_event, mouse_event)) return -1;

    return 0;
}
#endif
