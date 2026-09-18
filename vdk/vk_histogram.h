#ifndef _VK_HISTOGRAM_H_
#define _VK_HISTOGRAM_H_

#include <stdarg.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_graph.h"

/*
    vk_histogram derives from vk_graph.  It stores a set of raw sample values,
    bins them into nbins frequency buckets over [bin_min, bin_max], and plots
    the per-bin counts by overriding vk_graph's _bar_count / _bar_value
    virtuals -- all geometry, bar style and colour are inherited.

    Use the vk_graph_* setters via VK_GRAPH() for bar style, colours, the
    visible bin (x) window, etc.  vk_histogram_update() rebins and auto-scales
    the value (y) axis to the tallest bin, then renders.  (Mirrors how vk_meter
    overrides only vk_progress's _fill_color.)
*/
struct _vk_histogram_s
{
    vk_graph_t          parent_klass;

    double              *samples;       /* raw observations (owned copy)       */
    int                 sample_count;
    int                 sample_cap;

    int                 nbins;          /* number of frequency bins            */
    double              bin_min;        /* value domain that is binned         */
    double              bin_max;

    double              *bin_counts;    /* frequency per bin (owned)           */
    int                 bin_cap;

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);
};

#endif
