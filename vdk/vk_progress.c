#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_progress.h"
#include "vdk_private.h"

static int      _vk_progress_ctor(vk_object_t *object, va_list *argp, ...);
static int      _vk_progress_dtor(vk_object_t *object);
static int      _vk_progress_render(vk_widget_t *widget);
static int      _vk_progress_recreate(vk_widget_t *widget);
static void     _vk_progress_default_fill_color(vk_progress_t *progress,
                    short *fg, short *bg);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_PROGRESS_KLASS)
{
    .size = KLASS_SIZE(vk_progress_t),
    .name = KLASS_NAME(vk_progress_t),
    .ctor = _vk_progress_ctor,
    .dtor = _vk_progress_dtor,
};

/*
    length is the primary (fill) axis, thickness the cross axis.  A horizontal
    bar is length wide x thickness tall; a vertical bar thickness wide x length
    tall.  Default thickness is 1.
*/
vk_progress_t*
vk_progress_create(int orientation, int length, int thickness)
{
    vk_progress_t   *progress;
    int             w, h;

    if(length < 1) length = 1;
    if(thickness < 1) thickness = 1;

    if(orientation == VK_PROGRESS_VERTICAL)
    {
        w = thickness;
        h = length;
    }
    else
    {
        w = length;
        h = thickness;
    }

    progress = (vk_progress_t *)vk_object_create(VK_PROGRESS_KLASS, w, h,
        orientation, thickness);

    return progress;
}

static int
_vk_progress_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_progress_t   *progress;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    /* the widget base consumes width, height and creates the canvas */
    VK_WIDGET_KLASS->ctor(object, argp);

    progress = VK_PROGRESS(object);

    progress->orientation = va_arg(*argp, int);
    progress->thickness   = va_arg(*argp, int);

    va_end(args);

    progress->style        = VK_PROGRESS_UNICODE;
    progress->relief       = 0;
    progress->trough_style = VK_TROUGH_STIPPLE;

    progress->value        = 0.0;
    progress->range_min    = 0.0;
    progress->range_max    = 100.0;

    progress->fill_fg      = COLOR_GREEN;
    progress->fill_bg      = COLOR_BLACK;
    progress->fill_attrs   = A_NORMAL;

    progress->trough_fg    = COLOR_WHITE;
    progress->trough_bg    = COLOR_BLACK;
    progress->trough_ch    = 0x2591;            /* U+2591 LIGHT SHADE */

    progress->ctor = _vk_progress_ctor;
    progress->dtor = _vk_progress_dtor;
    progress->_fill_color = _vk_progress_default_fill_color;

    /* Follow the toolkit convention: _update-style rendering draws the bar
       into the widget's own canvas; the inherited base _draw composites that
       canvas onto the surface.  _recreate re-renders after a teleport/resize
       rebuilds the canvas (mirrors vk_label). */
    VK_WIDGET(object)->_recreate = _vk_progress_recreate;

    return 0;
}

static int
_vk_progress_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_progress_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static void
_vk_progress_default_fill_color(vk_progress_t *progress, short *fg, short *bg)
{
    *fg = progress->fill_fg;
    *bg = progress->fill_bg;
}


static int
_vk_progress_render(vk_widget_t *widget)
{
    vk_progress_t   *progress;
    WINDOW          *canvas;
    int             ox, oy, iw, ih;         /* inner (post-relief) area */
    int             length, cross;
    int             vertical, ascii, underbar, subcell;
    double          span, frac;
    int             eighths, full, partial;
    short           ffg, fbg, fill_pair, trough_pair;
    cchar_t         cc_full, cc_part, cc_trough;
    wchar_t         wbuf[2];
    int             i, k;

    if(widget == NULL || widget->canvas == NULL) return -1;

    progress = VK_PROGRESS(widget);
    canvas   = widget->canvas;

    werase(canvas);

    vertical = (progress->orientation == VK_PROGRESS_VERTICAL);
    ascii    = (progress->style == VK_PROGRESS_ASCII);
    underbar = (progress->style == VK_PROGRESS_UNDERBAR);

    /*
        1/8 sub-cell fill: UNICODE full-block only, and only with a solid or
        absent trough.  A partial block's unfilled half is a solid rectangle,
        which blends against a solid trough (or blank) but not against a
        stipple.  ASCII and UNDERBAR are whole-cell.
    */
    subcell  = (!ascii && !underbar
                && progress->trough_style != VK_TROUGH_STIPPLE);

    ox = oy = 0;
    iw = widget->width;
    ih = widget->height;

    /* reserve a border for sunken relief when there's room for interior
       (bright highlight / plain shadow enforced inside vdk_draw_relief) */
    if((progress->relief & VK_RELIEF_SUNKEN) && iw >= 3 && ih >= 3)
    {
        vdk_draw_relief(widget, VK_RELIEF_SUNKEN, widget->bg, 0);
        ox = 1; oy = 1;
        iw -= 2; ih -= 2;
    }

    length = vertical ? ih : iw;
    cross  = vertical ? iw : ih;
    if(length < 1 || cross < 1) return 0;

    /* fill fraction over [range_min, range_max], clamped */
    span = progress->range_max - progress->range_min;
    frac = (span > 0.0) ? (progress->value - progress->range_min) / span : 0.0;
    if(frac < 0.0) frac = 0.0;
    if(frac > 1.0) frac = 1.0;

    eighths = (int)(frac * (double)length * 8.0);
    full    = eighths / 8;
    partial = eighths % 8;
    if(!subcell)
    {
        if(partial >= 4 && full < length) full++;   /* round to nearest cell */
        partial = 0;
    }
    if(full > length) { full = length; partial = 0; }

    /* current fill colour (base = fixed; meter = threshold-dependent) */
    progress->_fill_color(progress, &ffg, &fbg);
    fill_pair = vdk_color_pair(ffg, fbg);

    /* full fill cell.  UNDERBAR draws a reverse-video underscore -- the cell
       shows the fill colour with a thin baseline in the fill bg -- so the
       fill attrs carry A_REVERSE and the glyph is '_'. */
    if(underbar)      wbuf[0] = L'_';
    else if(ascii)    wbuf[0] = L'#';
    else              wbuf[0] = (wchar_t)0x2588;     /* U+2588 FULL BLOCK */
    wbuf[1] = L'\0';
    setcchar(&cc_full, wbuf,
        underbar ? (progress->fill_attrs | A_REVERSE) : progress->fill_attrs,
        fill_pair, NULL);

    /* trough cell */
    if(progress->trough_style == VK_TROUGH_NONE)
    {
        /* blank in the bar's own background so the partial cell can blend */
        wbuf[0] = L' ';
        trough_pair = fill_pair;
    }
    else
    {
        if(progress->trough_style == VK_TROUGH_SOLID)
            wbuf[0] = ascii ? L'#' : (wchar_t)0x2588;
        else /* VK_TROUGH_STIPPLE */
            wbuf[0] = ascii ? L'.' : progress->trough_ch;
        trough_pair = vdk_color_pair(progress->trough_fg, progress->trough_bg);
    }
    wbuf[1] = L'\0';
    setcchar(&cc_trough, wbuf, A_NORMAL, trough_pair, NULL);

    /*
        partial fill cell (sub-cell mode only).  Its unfilled half must match
        the neighbouring trough: the solid trough colour, or the fill bg when
        the trough is absent.
    */
    if(subcell && partial > 0)
    {
        short part_bg = (progress->trough_style == VK_TROUGH_SOLID)
                            ? progress->trough_fg : fbg;
        short part_pair = vdk_color_pair(ffg, part_bg);

        wbuf[0] = vertical ? (wchar_t)(0x2580 + partial)    /* U+2581..U+2587 */
                           : (wchar_t)(0x2590 - partial);   /* U+258F..U+2589 */
        wbuf[1] = L'\0';
        setcchar(&cc_part, wbuf, progress->fill_attrs, part_pair, NULL);
    }

    for(i = 0; i < length; i++)
    {
        cchar_t *cell;
        int     row, col;

        if(i < full)                                    cell = &cc_full;
        else if(subcell && i == full && partial > 0)    cell = &cc_part;
        else                                            cell = &cc_trough;

        for(k = 0; k < cross; k++)
        {
            if(vertical) { row = oy + (length - 1 - i); col = ox + k; }
            else         { row = oy + k;                col = ox + i; }

            mvwadd_wch(canvas, row, col, cell);
        }
    }

    return 0;
}

/*
    After a teleport/resize rebuilds the canvas it is empty -- re-render the
    bar so the new SCREEN shows current state instead of a blank slot.
*/
static int
_vk_progress_recreate(vk_widget_t *widget)
{
    if(widget == NULL) return -1;

    if(widget->composer != widget->canvas)
        delwin(widget->composer);

    widget->canvas = newwin(widget->height, widget->width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

    if(widget->canvas == NULL) return -1;

    return _vk_progress_render(widget);
}

int
vk_progress_set_range(vk_progress_t *progress, double min, double max)
{
    if(progress == NULL) return -1;

    progress->range_min = min;
    progress->range_max = max;

    return 0;
}

int
vk_progress_set_value(vk_progress_t *progress, double value)
{
    if(progress == NULL) return -1;

    progress->value = value;

    return 0;
}

double
vk_progress_get_value(vk_progress_t *progress)
{
    if(progress == NULL) return 0.0;

    return progress->value;
}

int
vk_progress_set_style(vk_progress_t *progress, int style)
{
    if(progress == NULL) return -1;

    progress->style = style;

    return 0;
}

int
vk_progress_set_colors(vk_progress_t *progress, short fill_fg, short fill_bg)
{
    if(progress == NULL) return -1;

    progress->fill_fg = fill_fg;
    progress->fill_bg = fill_bg;

    return 0;
}

int
vk_progress_set_attrs(vk_progress_t *progress, attr_t attrs)
{
    if(progress == NULL) return -1;

    progress->fill_attrs = attrs;

    return 0;
}

int
vk_progress_set_relief(vk_progress_t *progress, int relief)
{
    if(progress == NULL) return -1;

    progress->relief = relief;

    return 0;
}

int
vk_progress_set_trough(vk_progress_t *progress, int trough_style,
    short fg, short bg)
{
    if(progress == NULL) return -1;

    progress->trough_style = trough_style;
    progress->trough_fg = fg;
    progress->trough_bg = bg;

    return 0;
}

int
vk_progress_set_thickness(vk_progress_t *progress, int thickness)
{
    vk_widget_t *widget;

    if(progress == NULL) return -1;
    if(thickness < 1) thickness = 1;

    progress->thickness = thickness;

    widget = VK_WIDGET(progress);

    if(progress->orientation == VK_PROGRESS_VERTICAL)
        vk_widget_resize(widget, thickness, widget->height);
    else
        vk_widget_resize(widget, widget->width, thickness);

    return 0;
}

int
vk_progress_update(vk_progress_t *progress)
{
    if(progress == NULL) return -1;

    /* render current state into the canvas; vk_screen_refresh's base _draw
       composites it onto the surface (mirrors vk_label_update). */
    return _vk_progress_render(VK_WIDGET(progress));
}

void
vk_progress_destroy(vk_progress_t *progress)
{
    if(progress == NULL) return;

    vk_object_destroy(VK_OBJECT(progress));
}
