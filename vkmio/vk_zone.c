#include <stdlib.h>
#include <string.h>

#include "vk_zone.h"

#define VK_ZONE_INIT_CAP    8

typedef struct
{
    int             zone_id;
    int             x, y, w, h;
    vk_zonemap_t    *child;
} vk_zone_entry_t;

struct _vk_zonemap_s
{
    vk_zone_entry_t *zones;
    int             count;
    int             capacity;
    int             origin_x;
    int             origin_y;
};

static int
_vk_zonemap_find(vk_zonemap_t *map, int zone_id)
{
    int i;

    for(i = 0; i < map->count; i++)
    {
        if(map->zones[i].zone_id == zone_id)
            return i;
    }

    return -1;
}

static int
_vk_zonemap_grow(vk_zonemap_t *map)
{
    int             new_cap;
    vk_zone_entry_t *new_zones;

    new_cap = map->capacity * 2;
    new_zones = realloc(map->zones, new_cap * sizeof(vk_zone_entry_t));
    if(new_zones == NULL) return -1;

    map->zones = new_zones;
    map->capacity = new_cap;

    return 0;
}

vk_zonemap_t*
vk_zonemap_create(void)
{
    vk_zonemap_t *map;

    map = calloc(1, sizeof(vk_zonemap_t));
    if(map == NULL) return NULL;

    map->zones = calloc(VK_ZONE_INIT_CAP, sizeof(vk_zone_entry_t));
    if(map->zones == NULL)
    {
        free(map);
        return NULL;
    }

    map->capacity = VK_ZONE_INIT_CAP;

    return map;
}

void
vk_zonemap_destroy(vk_zonemap_t *map)
{
    if(map == NULL) return;

    free(map->zones);
    free(map);
}

int
vk_zonemap_add(vk_zonemap_t *map, int zone_id,
    int x, int y, int w, int h)
{
    return vk_zonemap_add_child(map, zone_id, x, y, w, h, NULL);
}

int
vk_zonemap_add_child(vk_zonemap_t *map, int zone_id,
    int x, int y, int w, int h, vk_zonemap_t *child)
{
    vk_zone_entry_t *entry;

    if(map == NULL) return -1;
    if(_vk_zonemap_find(map, zone_id) >= 0) return -1;

    if(map->count >= map->capacity)
    {
        if(_vk_zonemap_grow(map) != 0) return -1;
    }

    entry = &map->zones[map->count];
    entry->zone_id = zone_id;
    entry->x = x;
    entry->y = y;
    entry->w = w;
    entry->h = h;
    entry->child = child;

    map->count++;

    return 0;
}

int
vk_zonemap_remove(vk_zonemap_t *map, int zone_id)
{
    int idx;

    if(map == NULL) return -1;

    idx = _vk_zonemap_find(map, zone_id);
    if(idx < 0) return -1;

    if(idx < map->count - 1)
    {
        memmove(&map->zones[idx], &map->zones[idx + 1],
            (map->count - idx - 1) * sizeof(vk_zone_entry_t));
    }

    map->count--;

    return 0;
}

int
vk_zonemap_update(vk_zonemap_t *map, int zone_id,
    int x, int y, int w, int h)
{
    int idx;
    vk_zone_entry_t *entry;

    if(map == NULL) return -1;

    idx = _vk_zonemap_find(map, zone_id);
    if(idx < 0) return -1;

    entry = &map->zones[idx];
    entry->x = x;
    entry->y = y;
    entry->w = w;
    entry->h = h;

    return 0;
}

int
vk_zonemap_set_origin(vk_zonemap_t *map, int origin_x, int origin_y)
{
    if(map == NULL) return -1;

    map->origin_x = origin_x;
    map->origin_y = origin_y;

    return 0;
}

int
vk_zonemap_test(vk_zonemap_t *map, int x, int y, vk_zone_hit_t *hit)
{
    int             lx, ly;
    int             i;
    vk_zone_entry_t *entry;

    if(map == NULL) return -1;
    if(hit == NULL) return -1;

    lx = x - map->origin_x;
    ly = y - map->origin_y;

    for(i = 0; i < map->count; i++)
    {
        entry = &map->zones[i];

        if(lx < entry->x || lx >= entry->x + entry->w) continue;
        if(ly < entry->y || ly >= entry->y + entry->h) continue;

        if(entry->child != NULL)
        {
            int cx = lx - entry->x;
            int cy = ly - entry->y;

            if(vk_zonemap_test(entry->child, cx, cy, hit) == 0)
                return 0;
        }

        hit->zone_id = entry->zone_id;
        hit->rx = lx - entry->x;
        hit->ry = ly - entry->y;

        return 0;
    }

    return -1;
}
