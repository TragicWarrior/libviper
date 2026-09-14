#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_graph.h"
#include "vdk_private.h"

static int      _vk_graph_ctor(vk_object_t *object, va_list *argp, ...);
static int      _vk_graph_dtor(vk_object_t *object);
static int      _vk_graph_render(vk_widget_t *widget);
static int      _vk_graph_recreate(vk_widget_t *widget);
static int      _vk_graph_default_bar_count(vk_graph_t *graph);
static double   _vk_graph_default_bar_value(vk_graph_t *graph, int index);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_GRAPH_KLASS)
{
    .size = KLASS_SIZE(vk_graph_t),
    .name = KLASS_NAME(vk_graph_t),
    .ctor = _vk_graph_ctor,
    .dtor = _vk_graph_dtor,
};

/*
    A graph occupies a width x height cell area.  Bars are laid out across the
    width and grow up from the bottom over the height.
*/
vk_graph_t*
vk_graph_create(int width, int height)
{
    vk_graph_t  *graph;

    if(width < 1) width = 1;
    if(height < 1) height = 1;

    graph = (vk_graph_t *)vk_object_create(VK_GRAPH_KLASS, width, height);

    return graph;
}

static int
_vk_graph_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_graph_t  *graph;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* the widget base consumes width, height and creates the canvas */
    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    graph = VK_GRAPH(object);

    graph->graph_type    = VK_GRAPH_BAR;
    graph->bar_style     = VK_GRAPH_BAR_BLOCK;
    graph->orientation   = VK_PROGRESS_VERTICAL;

    graph->data          = NULL;
    graph->data_count    = 0;
    graph->data_cap      = 0;

    graph->x_min         = 0.0;
    graph->x_max         = 0.0;         /* max <= min => show every bar        */
    graph->y_min         = 0.0;
    graph->y_max         = 100.0;

    graph->unit_scale    = 1.0;
    graph->unit_label[0] = '\0';

    graph->bar_fg        = COLOR_GREEN;
    graph->bar_bg        = COLOR_BLACK;
    graph->bar_attrs     = A_NORMAL;

    graph->ctor          = _vk_graph_ctor;
    graph->dtor          = _vk_graph_dtor;

    graph->_bar_count    = _vk_graph_default_bar_count;
    graph->_bar_value    = _vk_graph_default_bar_value;

    /* re-render after a teleport/resize rebuilds the canvas (mirrors
       vk_progress); the inherited base _draw composites it onto the surface */
    VK_WIDGET(object)->_recreate = _vk_graph_recreate;

    return 0;
}

static int
_vk_graph_dtor(vk_object_t *object)
{
    vk_graph_t  *graph;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_graph_t)) return -1;

    graph = VK_GRAPH(object);
    if(graph->data != NULL)
    {
        free(graph->data);
        graph->data = NULL;
    }
    if(graph->x_labels != NULL)
    {
        free(graph->x_labels);
        graph->x_labels = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

/* base virtuals: plot the stored series directly (see header) */
static int
_vk_graph_default_bar_count(vk_graph_t *graph)
{
    return graph->data_count;
}

static double
_vk_graph_default_bar_value(vk_graph_t *graph, int index)
{
    if(index < 0 || index >= graph->data_count) return 0.0;

    return graph->data[index];
}

static int
_vk_graph_render(vk_widget_t *widget)
{
    /* braille dot masks: fill k rows (0..4) from the cell bottom, both cols */
    static const unsigned char braille_fill[5] =
                    { 0x00, 0xC0, 0xE4, 0xF6, 0xFF };

    vk_graph_t  *graph;
    WINDOW      *canvas;
    int         pw, ph;                 /* widget width / height in cells    */
    int         inner_pw, inner_ph;     /* plot area dimensions              */
    int         nbars, first, last, vis;
    int         slot, gap, bw;
    int         j, index;
    double      span;
    short       bar_pair;
    short       color_pair;
    char        ybuf[32];               /* formatted tick value + unit       */

    if(widget == NULL || widget->canvas == NULL) return -1;

    graph  = VK_GRAPH(widget);
    canvas = widget->canvas;

    /* Apply widget fg/bg so werase is not default black (vk_widget_set_colors
       does not wbkgd). Match vk_frame. */
    color_pair = vdk_color_pair(widget->fg, widget->bg);
    wbkgd(canvas, COLOR_PAIR(color_pair));

    werase(canvas);

    pw = widget->width;
    ph = widget->height;
    if(pw < 1 || ph < 1) return 0;

    nbars = graph->_bar_count(graph);
    if(nbars < 1) return 0;

    /* visible index window from x_range; max <= min shows every bar */
    if(graph->x_max > graph->x_min)
    {
        first = (int)graph->x_min;
        last  = (int)graph->x_max;                  /* inclusive              */
        if(first < 0) first = 0;
        if(last > nbars - 1) last = nbars - 1;
    }
    else
    {
        first = 0;
        last  = nbars - 1;
    }
    vis = last - first + 1;
    if(vis < 1) return 0;

    /* Reserve a 4-col Y-axis gutter and a 1-row X-axis gutter.  If there is
       not enough room for both, fall back to the original full-width bars.
       The Y gutter holds right-aligned tick text (e.g. "1234 W").
       The X gutter holds tick labels centered under each bar slot.        */
    if(pw >= 5 && ph >= 3)
    {
        inner_pw = pw - 4;
        if(inner_pw < 1) inner_pw = 1;
    }
    else
    {
        inner_pw = pw;
    }

    if(pw >= 5 && ph >= 3)
    {
        inner_ph = ph - 1;
        if(inner_ph < 1) inner_ph = 1;
    }
    else
    {
        inner_ph = ph;
    }

    /* one slot per visible bar across the inner plot area; leave a 1-col
       gap when a slot has room */
    slot = inner_pw / vis;
    if(slot < 1) slot = 1;
    gap  = (slot >= 2) ? 1 : 0;
    bw   = slot - gap;
    if(bw < 1) bw = 1;

    bar_pair = vdk_color_pair(graph->bar_fg, graph->bar_bg);
    span     = graph->y_max - graph->y_min;

    for(j = 0; j < vis; j++)
    {
        double      value, frac;
        int         col0, k, c;
        cchar_t     cc_full, cc_part;
        wchar_t     wbuf[2];

        col0 = j * slot;
        if(col0 + bw > inner_pw) break;
        index = first + j;

        value = graph->_bar_value(graph, index);
        frac  = (span > 0.0) ? (value - graph->y_min) / span : 0.0;
        if(frac < 0.0) frac = 0.0;
        if(frac > 1.0) frac = 1.0;

        if(graph->bar_style == VK_GRAPH_BAR_BRAILLE)
        {
            int dots  = (int)(frac * (double)inner_ph * 4.0);
            int bfull = dots / 4;
            int bpart = dots % 4;

            if(bfull > inner_ph) { bfull = inner_ph; bpart = 0; }

            wbuf[0] = (wchar_t)0x28FF;                  /* full braille cell   */
            wbuf[1] = L'\0';
            setcchar(&cc_full, wbuf, graph->bar_attrs, bar_pair, NULL);

            for(k = 0; k < bfull; k++)
                for(c = 0; c < bw && col0 + c < inner_pw; c++)
                    mvwadd_wch(canvas, inner_ph - 1 - k, col0 + c, &cc_full);

            if(bpart > 0 && bfull < inner_ph)
            {
                wbuf[0] = (wchar_t)(0x2800 + braille_fill[bpart]);
                wbuf[1] = L'\0';
                setcchar(&cc_part, wbuf, graph->bar_attrs, bar_pair, NULL);
                for(c = 0; c < bw && col0 + c < inner_pw; c++)
                    mvwadd_wch(canvas, inner_ph - 1 - bfull, col0 + c, &cc_part);
            }
        }
        else if(graph->bar_style == VK_GRAPH_BAR_ASCII)
        {
            int eighths = (int)(frac * (double)inner_ph * 8.0);
            int cells   = eighths / 8;

            if((eighths % 8) >= 4 && cells < inner_ph) cells++;  /* round  */
            if(cells > inner_ph) cells = inner_ph;

            wbuf[0] = L'#';
            wbuf[1] = L'\0';
            setcchar(&cc_full, wbuf, graph->bar_attrs, bar_pair, NULL);

            for(k = 0; k < cells; k++)
                for(c = 0; c < bw && col0 + c < inner_pw; c++)
                    mvwadd_wch(canvas, inner_ph - 1 - k, col0 + c, &cc_full);
        }
        else /* VK_GRAPH_BAR_BLOCK */
        {
            int eighths = (int)(frac * (double)inner_ph * 8.0);
            int full    = eighths / 8;
            int partial = eighths % 8;

            if(full > inner_ph) { full = inner_ph; partial = 0; }

            wbuf[0] = (wchar_t)0x2588;                  /* U+2588 FULL BLOCK   */
            wbuf[1] = L'\0';
            setcchar(&cc_full, wbuf, graph->bar_attrs, bar_pair, NULL);

            for(k = 0; k < full; k++)
                for(c = 0; c < bw && col0 + c < inner_pw; c++)
                    mvwadd_wch(canvas, inner_ph - 1 - k, col0 + c, &cc_full);

            if(partial > 0 && full < inner_ph)
            {
                wbuf[0] = (wchar_t)(0x2580 + partial);  /* U+2581..U+2587      */
                wbuf[1] = L'\0';
                setcchar(&cc_part, wbuf, graph->bar_attrs, bar_pair, NULL);
                for(c = 0; c < bw && col0 + c < inner_pw; c++)
                    mvwadd_wch(canvas, inner_ph - 1 - full, col0 + c, &cc_part);
            }
        }
    }

    /* ---- Y-axis ticks: right-aligned in the gutter column at row 0 ------- */
    if(pw >= 5 && ph >= 3)
    {
        double      yv;
        int         yrow;
        int         len;
        static const double ytick_pos[3] =
                        { 1.0, 0.5, 0.0 };    /* y_max, mid, y_min  */

        for(j = 0; j < 3; j++)
        {
            yv = graph->y_min + ytick_pos[j] * span;

            if(graph->unit_scale != 1.0)
                snprintf(ybuf, sizeof(ybuf), "%.0f %s",
                         yv * graph->unit_scale, graph->unit_label);
            else if(graph->unit_label[0] != '\0')
                snprintf(ybuf, sizeof(ybuf), "%.0f %s",
                         yv, graph->unit_label);
            else
                snprintf(ybuf, sizeof(ybuf), "%.0f", yv);

            len = (int)strlen(ybuf);
            yrow = (int)(ytick_pos[j] * (double)inner_ph);
            if(yrow < 0) yrow = 0;
            if(yrow > inner_ph - 1) yrow = inner_ph - 1;

            wattron(canvas, vdk_color_pair(widget->fg, widget->bg));
            mvwaddnstr(canvas, yrow, 0, ybuf + (len > 1 ? len - 1 : 0), 1);
            wattroff(canvas, vdk_color_pair(widget->fg, widget->bg));
        }
    }

    /* ---- X-axis labels: centered under each bar slot, row inner_ph ------- */
    if(pw >= 5 && ph >= 3 && graph->x_label_count > 0)
    {
        const char *label;
        int         llen;
        int         center, left, right, label_col;
        int         i, idx;

        for(i = 0; i < vis && i < graph->x_label_count; i++)
        {
            idx = first + i;
            if(idx >= graph->x_label_count) break;

            label      = graph->x_labels[idx];
            if(label == NULL) continue;
            llen       = (int)strlen(label);

            center = i * slot + slot / 2;
            left   = center - llen / 2;
            right  = left + llen - 1;

            /* Skip if label doesn't fit within the inner plot area */
            if(left < 0 || right >= inner_pw) continue;

            /* Clamp left edge so label does not overlap the Y-axis gutter */
            if(left < 0) left = 0;
            label_col = left;

            mvwaddstr(canvas, inner_ph, label_col, label);
        }
    }
    else if(pw >= 5 && ph >= 3)
    {
        /* No custom labels -- show first/mid/last bar indices */
        char    idxbuf[16];
        int     mid = (first + last) / 2;
        int     indices[3];
        int     nidx = 0;
        int     k, idx, label_col, center;
        int     len;

        indices[nidx++] = first;
        if(mid != first && mid != last)
            indices[nidx++] = mid;
        indices[nidx++] = last;

        for(k = 0; k < nidx; k++)
        {
            idx = indices[k];
            j = idx - first;
            center = j * slot + slot / 2;
            label_col = center;

            if(label_col < 0 || label_col >= inner_pw) continue;

            snprintf(idxbuf, sizeof(idxbuf), "%d", idx);
            len = (int)strlen(idxbuf);
            if(len > 0 && label_col < len)
                label_col = len;           /* nudge right of Y gutter  */

            mvwaddstr(canvas, inner_ph, label_col, idxbuf);
        }
    }

    return 0;
}

static int
_vk_graph_recreate(vk_widget_t *widget)
{
    if(vdk_widget_reset_canvas(widget) < 0) return -1;

    return _vk_graph_render(widget);
}

int
vk_graph_set_type(vk_graph_t *graph, int type)
{
    if(graph == NULL) return -1;

    graph->graph_type = type;

    return 0;
}

int
vk_graph_set_bar_style(vk_graph_t *graph, int bar_style)
{
    if(graph == NULL) return -1;

    graph->bar_style = bar_style;

    return 0;
}

int
vk_graph_set_x_range(vk_graph_t *graph, double min, double max)
{
    if(graph == NULL) return -1;

    graph->x_min = min;
    graph->x_max = max;

    return 0;
}

int
vk_graph_set_y_range(vk_graph_t *graph, double min, double max)
{
    if(graph == NULL) return -1;

    graph->y_min = min;
    graph->y_max = max;

    return 0;
}

int
vk_graph_set_unit_scale(vk_graph_t *graph, double scale)
{
    if(graph == NULL) return -1;

    graph->unit_scale = scale;

    return 0;
}

int
vk_graph_set_unit_label(vk_graph_t *graph, const char *label)
{
    if(graph == NULL) return -1;

    if(label == NULL) label = "";
    strncpy(graph->unit_label, label, sizeof(graph->unit_label) - 1);
    graph->unit_label[sizeof(graph->unit_label) - 1] = '\0';

    return 0;
}

/*
    Replace the plotted series with a copy of count values.  The internal
    buffer only ever grows; a shorter set reuses it and updates the count.
*/
int
vk_graph_set_data(vk_graph_t *graph, const double *values, int count)
{
    if(graph == NULL) return -1;
    if(count < 0) count = 0;

    if(count > graph->data_cap)
    {
        double *grown = (double *)realloc(graph->data,
                            (size_t)count * sizeof(double));
        if(grown == NULL) return -1;
        graph->data = grown;
        graph->data_cap = count;
    }

    if(count > 0 && values != NULL)
        memcpy(graph->data, values, (size_t)count * sizeof(double));

    graph->data_count = count;

    return 0;
}

int
vk_graph_set_colors(vk_graph_t *graph, short fg, short bg)
{
    if(graph == NULL) return -1;

    graph->bar_fg = fg;
    graph->bar_bg = bg;

    return 0;
}

int
vk_graph_set_attrs(vk_graph_t *graph, attr_t attrs)
{
    if(graph == NULL) return -1;

    graph->bar_attrs = attrs;

    return 0;
}

/*
    Replace the X-axis label set with a copy of the pointer array.  The
    caller retains ownership of the individual strings (which must remain
    valid for the lifetime of the widget).  count == 0 or labels == NULL
    clears any previous labels.
*/
int
vk_graph_set_x_labels(vk_graph_t *graph, const char * const *labels, int count)
{
    if(graph == NULL) return -1;

    if(graph->x_labels != NULL)
    {
        free(graph->x_labels);
        graph->x_labels = NULL;
    }

    if(labels != NULL && count > 0)
    {
        const char **copy = (const char **)malloc((size_t)count * sizeof(char *));
        if(copy == NULL) return -1;
        memcpy(copy, labels, (size_t)count * sizeof(char *));
        graph->x_labels  = copy;
        graph->x_label_count = count;
    }
    else
    {
        graph->x_label_count = 0;
    }

    return 0;
}

int
vk_graph_update(vk_graph_t *graph)
{
    if(graph == NULL) return -1;

    /* render current state into the canvas; vk_screen_refresh's base _draw
       composites it onto the surface (mirrors vk_progress_update) */
    return _vk_graph_render(VK_WIDGET(graph));
}

void
vk_graph_destroy(vk_graph_t *graph)
{
    if(graph == NULL) return;

    vk_object_destroy(VK_OBJECT(graph));
}
