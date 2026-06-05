#ifndef _VKMIO_H_
#define _VKMIO_H_

#include <inttypes.h>

#undef  NCURSES_OPAQUE
#define NCURSES_OPAQUE 0
#include <ncursesw/curses.h>

/* vk_kmio_init flags */
#define VK_KMIO_MOUSE          (1 << 0)
#define VK_KMIO_MOUSE_HOVER    (1 << 1)
#define VK_KMIO_GPM_SIGIO      (1 << 2)

/* kmio return codes */
#define KMIO_HANDLED            0
#define KMIO_ERROR             -1
#define KMIO_NONE              -1

/* GPM commands */
#define VK_GPM_CMD_CLOSE       (1 << 1)

int         vk_kmio_init(uint32_t flags);
void        vk_kmio_shutdown(void);
int32_t     vk_kmio_fetch(MEVENT *mouse_event);
MEVENT*     vk_kmio_get_mouse_event(void);

#if !defined(_NO_GPM) && defined(__linux)
int         vk_kmio_gpm(MEVENT *mouse_event, uint16_t cmd);
#endif

#endif
