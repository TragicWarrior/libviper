#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_graph.h"
#include "vk_histogram.h"

static int      _vk_histogram_ctor(vk_object_t *object, va_list *argp, ...);
static int      _vk_histogram_dtor(vk_object_t *object);
static int      _vk_histogram_bar_count(vk_graph_t *graph);
static double   _vk_histogram_bar_value(vk_graph_t *graph, int index);
static void     _vk_histogram_rebin(vk_histogram_t *hist);

require_klass(VK_GRAPH_KLASS);

declare_klass(VK_HISTOGRAM_KLASS)
{
    .size = KLASS_SIZE(vk_histogram_t),
    .name = KLASS_NAME(vk_histogram_t),
    .ctor = _vk_histogram_ctor,
    .dtor = _vk_histogram_dtor,
};

vk_histogram_t*
vk_histogram_create(int width, int height)
{
    vk_histogram_t  *hist;

    if(width < 1) width = 1;
    if(height < 1) height = 1;

    hist = (vk_histogram_t *)vk_object_create(VK_HISTOGRAM_KLASS, width, height);

    return hist;
}

static int
_vk_histogram_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_histogram_t  *hist;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* the graph base consumes width, height and sets the bar defaults */
    VK_GRAPH_KLASS->ctor(object, argp);

    va_end(args);

    hist = VK_HISTOGRAM(object);

    hist->samples      = NULL;
    hist->sample_count = 0;
    hist->sample_cap   = 0;

    hist->nbins        = 10;
    hist->bin_min      = 0.0;
    hist->bin_max      = 100.0;

    hist->bin_counts   = NULL;
    hist->bin_cap      = 0;

    hist->ctor         = _vk_histogram_ctor;
    hist->dtor         = _vk_histogram_dtor;

    /* override the graph's data-access virtuals to expose the binned counts;
       every bit of rendering / geometry stays in vk_graph */
    VK_GRAPH(object)->_bar_count = _vk_histogram_bar_count;
    VK_GRAPH(object)->_bar_value = _vk_histogram_bar_value;

    return 0;
}

static int
_vk_histogram_dtor(vk_object_t *object)
{
    vk_histogram_t  *hist;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_histogram_t)) return -1;

    hist = VK_HISTOGRAM(object);
    if(hist->samples != NULL)
    {
        free(hist->samples);
        hist->samples = NULL;
    }
    if(hist->bin_counts != NULL)
    {
        free(hist->bin_counts);
        hist->bin_counts = NULL;
    }

    /* demote to the base and run vk_graph's dtor (which frees its own state
       and chains down to vk_widget) */
    vk_object_demote(object, vk_graph_t);
    VK_GRAPH(object)->dtor(object);

    return 0;
}

/* overridden virtuals: expose the binned frequencies as the graph's bars */
static int
_vk_histogram_bar_count(vk_graph_t *graph)
{
    return VK_HISTOGRAM(graph)->nbins;
}

static double
_vk_histogram_bar_value(vk_graph_t *graph, int index)
{
    vk_histogram_t  *hist = VK_HISTOGRAM(graph);

    if(index < 0 || index >= hist->nbins || hist->bin_counts == NULL)
        return 0.0;

    return hist->bin_counts[index];
}

/*
    Recount the samples into nbins buckets over [bin_min, bin_max].  Samples
    outside that range are dropped; a sample exactly at bin_max lands in the
    last bin.
*/
static void
_vk_histogram_rebin(vk_histogram_t *hist)
{
    double  span;
    int     i, bin;

    if(hist->nbins < 1) hist->nbins = 1;

    if(hist->nbins > hist->bin_cap)
    {
        double *grown = (double *)realloc(hist->bin_counts,
                            (size_t)hist->nbins * sizeof(double));
        if(grown == NULL) return;
        hist->bin_counts = grown;
        hist->bin_cap = hist->nbins;
    }

    for(i = 0; i < hist->nbins; i++)
        hist->bin_counts[i] = 0.0;

    span = hist->bin_max - hist->bin_min;
    if(span <= 0.0) return;

    for(i = 0; i < hist->sample_count; i++)
    {
        double s = hist->samples[i];

        if(s < hist->bin_min || s > hist->bin_max) continue;

        bin = (int)((s - hist->bin_min) / span * (double)hist->nbins);
        if(bin >= hist->nbins) bin = hist->nbins - 1;
        if(bin < 0) bin = 0;

        hist->bin_counts[bin] += 1.0;
    }
}

/* Replace the raw sample set with a copy of count values. */
int
vk_histogram_set_samples(vk_histogram_t *hist, const double *values, int count)
{
    if(hist == NULL) return -1;
    if(count < 0) count = 0;

    if(count > hist->sample_cap)
    {
        double *grown = (double *)realloc(hist->samples,
                            (size_t)count * sizeof(double));
        if(grown == NULL) return -1;
        hist->samples = grown;
        hist->sample_cap = count;
    }

    if(count > 0 && values != NULL)
        memcpy(hist->samples, values, (size_t)count * sizeof(double));

    hist->sample_count = count;

    return 0;
}

int
vk_histogram_set_bins(vk_histogram_t *hist, int nbins)
{
    if(hist == NULL) return -1;
    if(nbins < 1) nbins = 1;

    hist->nbins = nbins;

    return 0;
}

int
vk_histogram_set_range(vk_histogram_t *hist, double min, double max)
{
    if(hist == NULL) return -1;

    hist->bin_min = min;
    hist->bin_max = max;

    return 0;
}

/*
    Rebin the samples, auto-scale the value (y) axis to the tallest bin, and
    render.  Call after changing the samples, bin count or range.
*/
int
vk_histogram_update(vk_histogram_t *hist)
{
    int     i;
    double  maxcount;

    if(hist == NULL) return -1;

    _vk_histogram_rebin(hist);

    maxcount = 1.0;
    if(hist->bin_counts != NULL)
    {
        for(i = 0; i < hist->nbins; i++)
        {
            if(hist->bin_counts[i] > maxcount)
                maxcount = hist->bin_counts[i];
        }
    }

    vk_graph_set_y_range(VK_GRAPH(hist), 0.0, maxcount);

    return vk_graph_update(VK_GRAPH(hist));
}

void
vk_histogram_destroy(vk_histogram_t *hist)
{
    if(hist == NULL) return;

    vk_object_destroy(VK_OBJECT(hist));
}
