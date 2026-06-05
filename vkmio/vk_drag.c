#include <stdlib.h>

#include "vk_drag.h"

struct _vk_drag_s
{
    int     mode;
    int     anchor_x;
    int     anchor_y;
    int     orig_x;
    int     orig_y;
    int     orig_w;
    int     orig_h;
};

vk_drag_t*
vk_drag_create(void)
{
    vk_drag_t *drag;

    drag = calloc(1, sizeof(vk_drag_t));

    return drag;
}

void
vk_drag_destroy(vk_drag_t *drag)
{
    free(drag);
}

void
vk_drag_begin(vk_drag_t *drag, int mode,
    int anchor_x, int anchor_y,
    int orig_x, int orig_y, int orig_w, int orig_h)
{
    if(drag == NULL) return;

    drag->mode = mode;
    drag->anchor_x = anchor_x;
    drag->anchor_y = anchor_y;
    drag->orig_x = orig_x;
    drag->orig_y = orig_y;
    drag->orig_w = orig_w;
    drag->orig_h = orig_h;
}

void
vk_drag_end(vk_drag_t *drag)
{
    if(drag == NULL) return;

    drag->mode = VK_DRAG_NONE;
    drag->anchor_x = 0;
    drag->anchor_y = 0;
    drag->orig_x = 0;
    drag->orig_y = 0;
    drag->orig_w = 0;
    drag->orig_h = 0;
}

int
vk_drag_get_mode(vk_drag_t *drag)
{
    if(drag == NULL) return VK_DRAG_NONE;

    return drag->mode;
}

void
vk_drag_compute(vk_drag_t *drag, int cur_x, int cur_y,
    int *out_x, int *out_y, int *out_w, int *out_h)
{
    int dx, dy;

    if(drag == NULL) return;

    dx = cur_x - drag->anchor_x;
    dy = cur_y - drag->anchor_y;

    if(out_x != NULL) *out_x = drag->orig_x + dx;
    if(out_y != NULL) *out_y = drag->orig_y + dy;
    if(out_w != NULL) *out_w = drag->orig_w + dx;
    if(out_h != NULL) *out_h = drag->orig_h + dy;
}
