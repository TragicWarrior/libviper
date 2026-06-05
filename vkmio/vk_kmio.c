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

int
vk_kmio_init(uint32_t flags)
{
    vk_kmio_flags = flags;

    if(flags & VK_KMIO_MOUSE)
    {
        mmask_t mouse_mask = ALL_MOUSE_EVENTS;

        if(flags & VK_KMIO_MOUSE_HOVER)
            mouse_mask |= REPORT_MOUSE_POSITION;

        mousemask(mouse_mask, NULL);

        if(flags & VK_KMIO_MOUSE_HOVER)
        {
            mouseinterval(0);
            printf("\033[?1003h");
            fflush(stdout);
        }
    }

    return 0;
}

void
vk_kmio_shutdown(void)
{
#if !defined(_NO_GPM) && defined(__linux)
    vk_kmio_gpm(NULL, VK_GPM_CMD_CLOSE);
#endif

    if(vk_kmio_flags & VK_KMIO_MOUSE_HOVER)
    {
        printf("\033[?1003l");
        fflush(stdout);
    }

    vk_kmio_flags = 0;
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
            if(key_code == KEY_MOUSE) getmouse(mouse_event);
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

    if(poll(&mio_poll, 1, 1) < 1) return -1;
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

    if(g_event.buttons == GPM_B_UP || g_event.buttons == GPM_B_FOURTH)
    {
        mouse_event->bstate = BUTTON4_PRESSED;
    }
    else if(g_event.buttons == GPM_B_DOWN)
    {
        mouse_event->bstate = BUTTON5_PRESSED;
    }
    else if((g_event.type & GPM_DRAG) || (g_event.type & GPM_MOVE))
    {
        mouse_event->bstate = REPORT_MOUSE_POSITION;
    }

    if(mouse_event->bstate == 0) return -1;

    return 0;
}
#endif
