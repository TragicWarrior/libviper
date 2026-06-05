#ifndef _VIPER_KMIO_H_
#define _VIPER_KMIO_H_

#include <inttypes.h>

#include <ncursesw/curses.h>

#include "viper.h"
#include "vkmio.h"

enum
{
    EVENTMODE_IDLE = 0,
    EVENTMODE_MOVE,
    EVENTMODE_RESIZE
};

ViperWkeyFunc   viper_window_get_key_func(vwnd_t *vwnd);

#endif
