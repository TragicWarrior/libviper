#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>

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

/* Button-1 tracking used by _vk_kmio_correct_sgr_motion to undo
   ncurses' SGR misclassification.  Three states:

     IDLE         - button known to be up
     HELD         - press in flight (we've seen a PRESSED, no matching
                    RELEASED yet)
     PROVISIONAL  - we saw a RELEASED while HELD but the press looked
                    stale (idle long + cursor jumped) -- defer the
                    decision until the next event tells us whether
                    the release was real (next event is also a
                    RELEASED-shaped hover) or the cursor just came
                    back mid-drag with the terminal having dropped
                    the implicit grab (next event is a PRESSED-shaped
                    drag).

   vk_kmio_btn1_last_{time,x,y} capture the moment + position of the
   last event in which button 1 was known to be down (press or drag),
   used to judge RELEASED-while-HELD as stale or real. */
enum vk_kmio_btn1_state_e
{
    VK_KMIO_BTN1_IDLE = 0,
    VK_KMIO_BTN1_HELD,
    VK_KMIO_BTN1_PROVISIONAL
};

static enum vk_kmio_btn1_state_e    vk_kmio_btn1_state = VK_KMIO_BTN1_IDLE;
static struct timespec              vk_kmio_btn1_last_time = {0, 0};
static int                          vk_kmio_btn1_last_x = -1;
static int                          vk_kmio_btn1_last_y = -1;

/* A RELEASED arriving while HELD is treated as a stale press (and
   therefore deferred via PROVISIONAL) iff BOTH:
     - more than this many milliseconds have passed since the last
       button-down event, and
     - the cursor has moved at least this many cells from where it
       was last seen with button 1 down.
   Both conditions are required so a deliberate press-and-hold-still-
   then-release stays recognised as a real click. */
#define VK_KMIO_BTN1_STALE_MS       750
#define VK_KMIO_BTN1_STALE_DIST     4

static void
_vk_kmio_btn1_reset(void)
{
    vk_kmio_btn1_state = VK_KMIO_BTN1_IDLE;
    vk_kmio_btn1_last_time.tv_sec = 0;
    vk_kmio_btn1_last_time.tv_nsec = 0;
    vk_kmio_btn1_last_x = -1;
    vk_kmio_btn1_last_y = -1;
}

static bool
_vk_kmio_btn1_press_is_stale(int x, int y)
{
    struct timespec     now;
    long                elapsed_ms;
    int                 dx, dy;

    clock_gettime(CLOCK_MONOTONIC, &now);
    elapsed_ms = (now.tv_sec - vk_kmio_btn1_last_time.tv_sec) * 1000
        + (now.tv_nsec - vk_kmio_btn1_last_time.tv_nsec) / 1000000;
    dx = (vk_kmio_btn1_last_x < 0) ? 0 : (x - vk_kmio_btn1_last_x);
    dy = (vk_kmio_btn1_last_y < 0) ? 0 : (y - vk_kmio_btn1_last_y);

    return elapsed_ms > VK_KMIO_BTN1_STALE_MS
        && (abs(dx) >= VK_KMIO_BTN1_STALE_DIST
            || abs(dy) >= VK_KMIO_BTN1_STALE_DIST);
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
    vk_kmio_flags = flags;

    /* teleport / reattach can call shutdown+init mid-session; clear
       any stale press carried over from the previous fd. */
    _vk_kmio_btn1_reset();

    if(flags & VK_KMIO_MOUSE)
    {
        mmask_t mouse_mask = ALL_MOUSE_EVENTS;

        if(flags & VK_KMIO_MOUSE_HOVER)
            mouse_mask |= REPORT_MOUSE_POSITION;

        mousemask(mouse_mask, NULL);

        if(flags & VK_KMIO_MOUSE_HOVER)
        {
            mouseinterval(0);
            /* 1003h: report every motion event (hover + drag).
               1006h: SGR encoding -- without it the terminal falls
               back to legacy X10 mouse encoding, where no-button
               motion is reported with button code 3 (= release) plus
               the motion bit, which ncurses then surfaces as a
               BUTTON1_RELEASED instead of a clean
               REPORT_MOUSE_POSITION.  Over SSH that misclassification
               broke hover-highlight (and caused spurious clicks
               before vwm's dropdown was hardened against unarmed
               releases). */
            _vk_kmio_write(fd, "\033[?1003h\033[?1006h");
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

    if(vk_kmio_flags & VK_KMIO_MOUSE_HOVER)
        _vk_kmio_write(fd, "\033[?1006l\033[?1003l");

    vk_kmio_flags = 0;
}

/* ncurses (every version we've tested) parses the SGR motion
   sequence \E[<35;col;rowM and surfaces it as BUTTON1_RELEASED
   rather than REPORT_MOUSE_POSITION -- it masks the motion bit
   (32) off the button code and treats the leftover 3 as "release
   button 1", same misbehaviour the legacy X10 mouse encoding has.
   Drag events (encoded as button=32, i.e. motion bit + button 0)
   similarly come up as bare BUTTON1_PRESSED.

   Since we can't fix ncurses from out here, recover the real intent
   from the event stream.  Match the bits with bitwise AND so modifier
   flags (BUTTON_SHIFT/CTRL/ALT) on a motion/drag don't slip past the
   rewriter; when rewriting to motion, clear only the PRESSED/RELEASED
   bit and OR in REPORT_MOUSE_POSITION so modifier flags survive.

   The tricky case is the cursor leaving the terminal mid-drag.  Some
   terminals don't preserve the implicit grab and start sending plain
   motion (button=35, surfacing as BUTTON1_RELEASED) at re-entry even
   though the user is still holding the button.  Looking at any single
   event we can't tell that from a real lost-release-then-hover.  So
   when a RELEASED arrives while HELD and the press looks stale (idle
   long, cursor jumped), don't commit either way -- enter the
   PROVISIONAL state, emit motion for now, and let the next event
   resolve it:

     - PROVISIONAL + RELEASED -> the next event is also a hover, so
       the button really is up.  Pass the RELEASED through (close the
       drag from the widget's POV) and return to IDLE.
     - PROVISIONAL + PRESSED  -> the next event is a drag, so the
       button was still held all along.  Rewrite to motion (the widget
       already thinks it's mid-drag) and return to HELD.

   Without the deferral, an aggressive "stale = lost release" rewrite
   eats real releases at re-entry: the widget never sees a RELEASED,
   the drag-target window sticks to the cursor, and the next click
   intended to break out of the stuck state gets misread as a fresh
   press on whatever it lands on. */
static void
_vk_kmio_correct_sgr_motion(MEVENT *m)
{
    if(m == NULL) return;

    if(m->bstate & BUTTON1_RELEASED)
    {
        switch(vk_kmio_btn1_state)
        {
            case VK_KMIO_BTN1_IDLE:
                /* hover misclassified as release */
                m->bstate = (m->bstate & ~BUTTON1_RELEASED)
                    | REPORT_MOUSE_POSITION;
                break;

            case VK_KMIO_BTN1_HELD:
                if(_vk_kmio_btn1_press_is_stale(m->x, m->y))
                {
                    /* could be a lost release or a cursor-returned-
                       mid-drag with the terminal having dropped the
                       grab.  Defer; resolve on the next event. */
                    vk_kmio_btn1_state = VK_KMIO_BTN1_PROVISIONAL;
                    m->bstate = (m->bstate & ~BUTTON1_RELEASED)
                        | REPORT_MOUSE_POSITION;
                }
                else
                {
                    /* matches the prior press -- real release */
                    vk_kmio_btn1_state = VK_KMIO_BTN1_IDLE;
                }
                break;

            case VK_KMIO_BTN1_PROVISIONAL:
                /* second consecutive non-press event confirms the
                   release was actually lost.  Let this RELEASED reach
                   the widget so its drag handler can close out. */
                vk_kmio_btn1_state = VK_KMIO_BTN1_IDLE;
                break;
        }
    }
    else if(m->bstate & BUTTON1_PRESSED)
    {
        switch(vk_kmio_btn1_state)
        {
            case VK_KMIO_BTN1_IDLE:
                /* fresh press; let it through */
                vk_kmio_btn1_state = VK_KMIO_BTN1_HELD;
                break;

            case VK_KMIO_BTN1_HELD:
                /* drag event misclassified as a fresh press */
                m->bstate = (m->bstate & ~BUTTON1_PRESSED)
                    | REPORT_MOUSE_POSITION;
                break;

            case VK_KMIO_BTN1_PROVISIONAL:
                /* the deferred RELEASED was really the terminal
                   forgetting the button was held; this PRESSED is the
                   drag resuming.  The widget already thinks it's
                   mid-drag, so surface motion, not a fresh press. */
                vk_kmio_btn1_state = VK_KMIO_BTN1_HELD;
                m->bstate = (m->bstate & ~BUTTON1_PRESSED)
                    | REPORT_MOUSE_POSITION;
                break;
        }

        clock_gettime(CLOCK_MONOTONIC, &vk_kmio_btn1_last_time);
        vk_kmio_btn1_last_x = m->x;
        vk_kmio_btn1_last_y = m->y;
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

    key_code = getch();

    if(key_code != -1)
    {
        if(key_code != 27)
        {
            if(key_code == KEY_MOUSE)
            {
                getmouse(mouse_event);
                _vk_kmio_correct_sgr_motion(mouse_event);
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
