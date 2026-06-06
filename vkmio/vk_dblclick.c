#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vk_dblclick.h"

#define VK_DBLCLICK_DEFAULT_MS  400

struct _vk_dblclick_s
{
    struct timespec last_time;
    int             last_item;
    int             threshold_ms;
};

vk_dblclick_t*
vk_dblclick_create(int threshold_ms)
{
    vk_dblclick_t *dbc;

    dbc = calloc(1, sizeof(vk_dblclick_t));
    if(dbc == NULL) return NULL;

    dbc->last_item = -1;
    dbc->threshold_ms = (threshold_ms > 0)
        ? threshold_ms : VK_DBLCLICK_DEFAULT_MS;

    return dbc;
}

void
vk_dblclick_destroy(vk_dblclick_t *dbc)
{
    free(dbc);
}

void
vk_dblclick_reset(vk_dblclick_t *dbc)
{
    if(dbc == NULL) return;

    memset(&dbc->last_time, 0, sizeof(dbc->last_time));
    dbc->last_item = -1;
}

int
vk_dblclick_test(vk_dblclick_t *dbc, int item)
{
    struct timespec now;
    int             result = 0;

    if(dbc == NULL) return 0;

    clock_gettime(CLOCK_MONOTONIC, &now);

    if(item == dbc->last_item)
    {
        long elapsed_ms;

        elapsed_ms = (now.tv_sec - dbc->last_time.tv_sec) * 1000
            + (now.tv_nsec - dbc->last_time.tv_nsec) / 1000000;

        if(elapsed_ms >= 0 && elapsed_ms < dbc->threshold_ms)
            result = 1;
    }

    dbc->last_time = now;
    dbc->last_item = item;

    return result;
}
