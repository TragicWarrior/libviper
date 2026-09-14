#ifndef _VK_GRAPH_H_
#define _VK_GRAPH_H_

#include <stdarg.h>
#include <wchar.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

/*
    vk_graph -- a plot widget derived directly from vk_widget.  v1 draws a
    single vertical bar graph.  Bar glyphs come in three styles: BLOCK (the
    Unicode full block U+2588 with a 1/8-cell sub-cell top), BRAILLE (2x4
    dots per cell -- 4x vertical resolution) and ASCII ('#').

    Data is one y-value series with an implicit integer x index.  x_range
    selects the visible index window; y_range is the value axis.  unit_scale
    is a display multiplier applied to tick values (e.g. 0.001 for mW);
    unit_label is appended after a space (e.g. "W").  x_labels provide
    custom strings for the X-axis; when absent, bar indices are shown.

    Future-proofing: vk_histogram will derive from vk_graph and override the
    _bar_count / _bar_value virtuals to plot binned frequencies -- all
    geometry, ranges and glyph rendering stay here (mirrors vk_meter, which
    overrides only vk_progress's _fill_color).
*/
struct _vk_graph_s
{
    vk_widget_t         parent_klass;

    int                 graph_type;     /* VK_GRAPH_BAR                        */
    int                 bar_style;      /* VK_GRAPH_BAR_BLOCK|_BRAILLE|_ASCII  */
    int                 orientation;    /* reserved; vertical bars only in v1  */

    double              *data;          /* y-value series (owned copy)         */
    int                 data_count;
    int                 data_cap;

    double              x_min;          /* visible index window (x range)      */
    double              x_max;
    double              y_min;          /* value axis (y range)                */
    double              y_max;

    double              unit_scale;     /* display multiplier for tick values  */
    char                unit_label[VK_GRAPH_UNIT_MAX];

    const char        **x_labels;       /* owned array of x-axis label strings */
    int                 x_label_count;

    short               bar_fg;
    short               bar_bg;
    attr_t              bar_attrs;

    int                 (*ctor)         (vk_object_t *, va_list *, ...);
    int                 (*dtor)         (vk_object_t *);

    /*
        virtuals -- the derivation seam.  Base vk_graph plots data[index]
        directly; a subclass (vk_histogram) overrides these to expose bars
        computed from its own dataset.
    */
    int                 (*_bar_count)   (vk_graph_t *graph);
    double              (*_bar_value)   (vk_graph_t *graph, int index);
};

#endif
