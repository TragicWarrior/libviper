#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>

#include <ncursesw/curses.h>

#include "vkmio.h"

#if !defined(_NO_GPM) && defined(__linux)
#include <gpm.h>

#define X_GPM_RAW           0
#define X_GPM_COOKED        1
#define GPM_RAW_MASK        0x0f
#define GPM_COOKED_BITS     (GPM_SINGLE | GPM_DOUBLE | GPM_TRIPLE)
#define GPM_CLICK(x)        ((x & (GPM_COOKED_BITS)) && (x & GPM_UP))
#define GPM_CLICK_STRICT(x) ((GPM_CLICK(x)) && !(x & GPM_MFLAG))

#define  X_GPM(a,b,c,d)    a,
static uint16_t x_gpm_mode[] = {
#include "vk_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    b,
static mmask_t x_ncurses_state[] = {
#include "vk_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    c,
static short x_gpm_button[] = {
#include "vk_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    d,
static unsigned short x_gpm_event[] = {
#include "vk_gpm.def"
};
#undef   X_GPM

#endif

static uint32_t     vk_kmio_flags = 0;
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
    vk_kmio_flags = flags;

    /* drop any half-read SGR body if init runs mid-session (teleport
       calls shutdown+init against the new fd) */
    sgr_in_body = false;
    sgr_len = 0;
    sgr_stall = 0;

    if(flags & VK_KMIO_MOUSE)
    {
        /* mask 0 on purpose: we decode SGR mouse reports ourselves off
           the bytes ncurses leaks (see vk_kmio_fetch).  With a non-zero
           mask ncurses instead cooks the report, and its decoder masks
           the motion bit (0x20) off the button code -- surfacing SGR
           motion as BUTTON1_RELEASED and SGR drag as BUTTON1_PRESSED,
           the misclassification that broke hover-highlight and drags
           over SSH.  1006h selects SGR encoding; 1003h (hover) reports
           every motion event, 1000h reports button events only. */
        mousemask(0, NULL);

        if(flags & VK_KMIO_MOUSE_HOVER)
            _vk_kmio_write(fd, "\033[?1003h\033[?1006h");
        else
            _vk_kmio_write(fd, "\033[?1000h\033[?1006h");
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
        if(vk_kmio_flags & VK_KMIO_MOUSE_HOVER)
            _vk_kmio_write(fd, "\033[?1006l\033[?1003l");
        else
            _vk_kmio_write(fd, "\033[?1006l\033[?1000l");
    }

    vk_kmio_flags = 0;
}

/* Decode an accumulated SGR mouse body into an MEVENT.  The body is
   the "Cb;Cx;Cy" between the (already-stripped) \033[< introducer and
   the terminator; the terminator ('M' = press/motion, 'm' = release)
   is passed separately.

   In the Cb button byte: bit 6 (0x40) marks a wheel event, bit 5
   (0x20) marks motion (hover or drag), the low two bits select the
   button (0=1, 1=2, 2=3), and bits 2/3/4 are shift/meta/ctrl.  Because
   the wire is self-describing -- motion is the 0x20 bit, release is the
   'm' terminator -- there is nothing to infer: no held-state tracking,
   no timestamps, no heuristics.  Cx/Cy are 1-based; MEVENT is 0-based.

   Returns true on a well-formed report. */
static bool
_vk_kmio_parse_sgr(const char *body, char term, MEVENT *m)
{
    int     cb, cx, cy;
    mmask_t bstate;

    if(m == NULL) return false;
    if(sscanf(body, "%d;%d;%d", &cb, &cx, &cy) != 3) return false;

    if(cb & 0x40)                   /* wheel: 0x40|0 = up, 0x40|1 = down */
        bstate = (cb & 0x01) ? BUTTON5_PRESSED : BUTTON4_PRESSED;
    else if(cb & 0x20)              /* motion -- hover or drag alike */
        bstate = REPORT_MOUSE_POSITION;
    else switch(cb & 0x03)          /* discrete press / release */
    {
        case 0:  bstate = (term == 'm') ? BUTTON1_RELEASED : BUTTON1_PRESSED; break;
        case 1:  bstate = (term == 'm') ? BUTTON2_RELEASED : BUTTON2_PRESSED; break;
        case 2:  bstate = (term == 'm') ? BUTTON3_RELEASED : BUTTON3_PRESSED; break;
        default: return false;      /* low2 == 3 without the motion bit */
    }

    if(cb & 0x04) bstate |= BUTTON_SHIFT;
    if(cb & 0x08) bstate |= BUTTON_ALT;
    if(cb & 0x10) bstate |= BUTTON_CTRL;

    memset(m, 0, sizeof(*m));
    m->bstate = bstate;
    m->x = cx - 1;
    m->y = cy - 1;
    return true;
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

        keystroke = 27;
        do
        {
            shift_op = shift_op << 1;
            key_code = getch();
            if(key_code == -1) break;
            keystroke |= (key_code << shift_op);
        }
        while(shift_op < 24);
    }

    return keystroke;
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

#if !defined(_NO_GPM) && defined(__linux)
int
vk_kmio_gpm(MEVENT *mouse_event, uint16_t cmd)
{
    extern int          gpm_tried;
    extern int          gpm_fd;
    struct pollfd       mio_poll;
    static int          mio_fd = -1;
    Gpm_Connect         gpm_connect;
    Gpm_Event           g_event;
    int                 array_sz;
    int                 i;
    int                 fflags;

    if(cmd == VK_GPM_CMD_CLOSE)
    {
        if(mio_fd > 0) Gpm_Close();
        mio_fd = -1;
        return 0;
    }

    if(mouse_event == NULL) return -1;

    if(gpm_fd == -2 || (gpm_fd == -1 && gpm_tried == TRUE)) return -1;

    memset(&g_event, 0, sizeof(g_event));

    if(mio_fd == -1)
    {
        if(gpm_fd >= 0) Gpm_Close();

        memset(&gpm_connect, 0, sizeof(gpm_connect));
        gpm_connect.defaultMask = 0;
        gpm_connect.eventMask = GPM_MOVE | GPM_UP | GPM_DOWN | GPM_DRAG;
        gpm_connect.maxMod = ~0;
        mio_fd = Gpm_Open(&gpm_connect, 0);

        if(mio_fd > 0 && (vk_kmio_flags & VK_KMIO_GPM_SIGIO))
        {
            fcntl(mio_fd, F_SETOWN, getpid());
            fflags = fcntl(mio_fd, F_GETFL);
            fcntl(mio_fd, F_SETFL, fflags | FASYNC);
        }
    }

    if(mio_fd == -1) return -1;

    memset(&mio_poll, 0, sizeof(mio_poll));
    mio_poll.events = POLLIN;
    mio_poll.fd = mio_fd;

    if(poll(&mio_poll, 1, (cmd == VK_GPM_CMD_DRAIN) ? 0 : 1) < 1) return -1;
    if(Gpm_GetEvent(&g_event) < 1) return -1;

    memset(mouse_event, 0, sizeof(MEVENT));
    mouse_event->bstate = g_event.modifiers;
    mouse_event->x = g_event.x - 1;
    mouse_event->y = g_event.y - 1;

    array_sz = sizeof(x_ncurses_state) / sizeof(x_ncurses_state[0]);

    if(!(GPM_CLICK_STRICT(g_event.type)))
    {
        for(i = 0; i < array_sz; i++)
        {
            if(x_gpm_mode[i] == X_GPM_COOKED) continue;
            if(!(g_event.type & x_gpm_event[i])) continue;
            if(g_event.buttons != x_gpm_button[i]) continue;

            mouse_event->bstate |= x_ncurses_state[i];
            break;
        }
    }

    if(GPM_CLICK_STRICT(g_event.type))
    {
        for(i = 0; i < array_sz; i++)
        {
            if(x_gpm_mode[i] == X_GPM_RAW) continue;
            if(!(g_event.type & x_gpm_event[i])) continue;
            if(g_event.buttons != x_gpm_button[i]) continue;

            mouse_event->bstate = x_ncurses_state[i];
            break;
        }
    }

    /* wheel: exps2/imps2 mice report scroll in wdy; the GPM_B_UP/DOWN
       buttons are the legacy ms3 mechanism (kept as a fallback) */
    if(g_event.wdy > 0 || g_event.buttons == GPM_B_UP ||
        g_event.buttons == GPM_B_FOURTH)
    {
        mouse_event->bstate = BUTTON4_PRESSED;      /* wheel up   */
    }
    else if(g_event.wdy < 0 || g_event.buttons == GPM_B_DOWN)
    {
        mouse_event->bstate = BUTTON5_PRESSED;      /* wheel down */
    }
    else if((g_event.type & GPM_DRAG) || (g_event.type & GPM_MOVE))
    {
        mouse_event->bstate = REPORT_MOUSE_POSITION;
    }

    if(mouse_event->bstate == 0) return -1;

    return 0;
}
#endif
