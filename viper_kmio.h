#ifndef _VIPER_KMIO_H_
#define _VIPER_KMIO_H_

#include <glib.h>

#ifdef _VIPER_WIDE
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

#if !defined(_NO_GPM) && defined(__linux)
#include <gpm.h>
#endif

enum
{
   EVENTMODE_IDLE=0,
   EVENTMODE_MOVE,
   EVENTMODE_RESIZE
};

#if !defined(_NO_GPM) && defined(__linux)

gint     viper_kmio_gpm(MEVENT *mouse_event,guint cmd);

#define  CMD_GPM_CLOSE        (1<<1)

#define  X_GPM_RAW            0
#define  X_GPM_COOKED         1
#define  GPM_RAW_MASK         0x0f
#define  GPM_COOKED_BITS      (GPM_SINGLE | GPM_DOUBLE | GPM_TRIPLE)
#define  GPM_CLICK(x)         ((x & (GPM_COOKED_BITS)) && (x & GPM_UP))
#define  GPM_CLICK_STRICT(x)  ((GPM_CLICK(x)) && !(x & GPM_MFLAG))

#endif

VIPER_WKEY_FUNC   viper_window_get_key_func(WINDOW *window);

#endif
