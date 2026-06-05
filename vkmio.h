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

/* vk_dblclick */

typedef struct _vk_dblclick_s   vk_dblclick_t;

vk_dblclick_t*  vk_dblclick_create(int threshold_ms);
void            vk_dblclick_destroy(vk_dblclick_t *dbc);
void            vk_dblclick_reset(vk_dblclick_t *dbc);
int             vk_dblclick_test(vk_dblclick_t *dbc, int item);

/* vk_drag */

#define VK_DRAG_NONE    0
#define VK_DRAG_MOVE    1
#define VK_DRAG_RESIZE  2

typedef struct _vk_drag_s       vk_drag_t;

vk_drag_t*      vk_drag_create(void);
void            vk_drag_destroy(vk_drag_t *drag);
void            vk_drag_begin(vk_drag_t *drag, int mode,
                    int anchor_x, int anchor_y,
                    int orig_x, int orig_y, int orig_w, int orig_h);
void            vk_drag_end(vk_drag_t *drag);
int             vk_drag_get_mode(vk_drag_t *drag);
void            vk_drag_compute(vk_drag_t *drag, int cur_x, int cur_y,
                    int *out_x, int *out_y, int *out_w, int *out_h);

/* vk_zone */

typedef struct _vk_zonemap_s    vk_zonemap_t;

typedef struct
{
    int     zone_id;
    int     rx;
    int     ry;
} vk_zone_hit_t;

vk_zonemap_t*   vk_zonemap_create(void);
void            vk_zonemap_destroy(vk_zonemap_t *map);

int             vk_zonemap_add(vk_zonemap_t *map,
                    int zone_id, int x, int y, int w, int h);
int             vk_zonemap_add_child(vk_zonemap_t *map,
                    int zone_id, int x, int y, int w, int h,
                    vk_zonemap_t *child);
int             vk_zonemap_remove(vk_zonemap_t *map, int zone_id);
int             vk_zonemap_update(vk_zonemap_t *map,
                    int zone_id, int x, int y, int w, int h);
int             vk_zonemap_set_origin(vk_zonemap_t *map,
                    int origin_x, int origin_y);
int             vk_zonemap_test(vk_zonemap_t *map,
                    int x, int y, vk_zone_hit_t *hit);

#endif
