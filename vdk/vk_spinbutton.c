#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_spinbutton.h"
#include "vk_event.h"

static int
_vk_spinbutton_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_spinbutton_dtor(vk_object_t *object);

static int
_vk_spinbutton_update(vk_spinbutton_t *spin);

static int
_vk_spinbutton_kmio(vk_object_t *object, int32_t keystroke);

/* ── geometry helpers (kept in sync between draw and hit-test) ────── */

static int
_vk_spinbutton_field_start(vk_spinbutton_t *spin)
{
    return (spin->relief_style == VK_BUTTON_BASIC) ? 2 : 1;
}

static int
_vk_spinbutton_text_row(vk_spinbutton_t *spin)
{
    return (spin->relief_style == VK_BUTTON_BASIC) ? 0 : 1;
}

static int
_vk_spinbutton_field_width(vk_spinbutton_t *spin)
{
    vk_widget_t *widget = VK_WIDGET(spin);

    if(spin->relief_style == VK_BUTTON_BASIC)
        return widget->width - 4;

    return widget->width - 2;
}

/* the right of the field holds a vertical separator and the two arrows:
   [ value ... ] [gap] [sep] [up] [gap] [down]
   the separator hugs the up arrow and the down arrow hugs the right
   border, so the arrow pair is framed symmetrically */
static void
_vk_spinbutton_arrow_cols(vk_spinbutton_t *spin, int *up_col, int *down_col)
{
    int field_start = _vk_spinbutton_field_start(spin);
    int field_width = _vk_spinbutton_field_width(spin);
    int down = field_start + field_width - 1;

    if(down_col != NULL) *down_col = down;
    if(up_col != NULL)   *up_col   = down - 2;
}

/* column of the vertical separator; it sits immediately left of the up
   arrow, with a single gap cell between it and the value field */
static int
_vk_spinbutton_sep_col(vk_spinbutton_t *spin)
{
    return _vk_spinbutton_field_start(spin)
        + _vk_spinbutton_field_width(spin) - 4;
}

static int
_vk_spinbutton_value_width(vk_spinbutton_t *spin)
{
    /* field minus the 5 cells reserved for [gap][sep][up][gap][down] */
    int w = _vk_spinbutton_field_width(spin) - 5;
    return (w > 0) ? w : 0;
}

static void
_vk_spinbutton_format(vk_spinbutton_t *spin, char *buf, size_t n)
{
    int dp = spin->precision;
    if(dp < 0) dp = 0;
    snprintf(buf, n, "%.*f", dp, spin->value);
}

/* clamp to [min, max], assign, and fire change notifications if it moved */
static int
_vk_spinbutton_apply(vk_spinbutton_t *spin, double v, bool notify)
{
    if(spin->max >= spin->min)
    {
        if(v < spin->min) v = spin->min;
        if(v > spin->max) v = spin->max;
    }

    if(v == spin->value) return 0;

    spin->value = v;

    if(notify)
    {
        vk_object_emit(VK_OBJECT(spin), VK_EVENT_ON_CHANGE);
        if(spin->on_change != NULL)
            spin->on_change(VK_WIDGET(spin), spin->on_change_arg);
    }

    return 1;
}

/* fold an in-progress edit buffer back into the numeric value */
static void
_vk_spinbutton_commit(vk_spinbutton_t *spin)
{
    char    *end = NULL;
    double  v;

    if(!spin->editing) return;

    spin->editing = false;
    spin->edit_scroll = 0;

    v = strtod(spin->edit_buf, &end);

    /* parse only counts if it consumed something real (not "" / "-" / ".") */
    if(end != NULL && end != spin->edit_buf)
        _vk_spinbutton_apply(spin, v, true);
}

static void
_vk_spinbutton_begin_edit(vk_spinbutton_t *spin, bool seed_from_value)
{
    if(spin->editing) return;

    spin->editing = true;

    if(seed_from_value)
        _vk_spinbutton_format(spin, spin->edit_buf, sizeof(spin->edit_buf));
    else
        spin->edit_buf[0] = '\0';

    spin->edit_len = (int)strlen(spin->edit_buf);
    spin->edit_cursor = spin->edit_len;
    spin->edit_scroll = 0;
}

static void
_vk_spinbutton_edit_insert(vk_spinbutton_t *spin, char c)
{
    if(spin->edit_len >= (int)sizeof(spin->edit_buf) - 1) return;

    memmove(&spin->edit_buf[spin->edit_cursor + 1],
        &spin->edit_buf[spin->edit_cursor],
        spin->edit_len - spin->edit_cursor + 1);

    spin->edit_buf[spin->edit_cursor] = c;
    spin->edit_len++;
    spin->edit_cursor++;
}

static void
_vk_spinbutton_edit_backspace(vk_spinbutton_t *spin)
{
    if(spin->edit_cursor == 0) return;

    memmove(&spin->edit_buf[spin->edit_cursor - 1],
        &spin->edit_buf[spin->edit_cursor],
        spin->edit_len - spin->edit_cursor + 1);

    spin->edit_len--;
    spin->edit_cursor--;
}

static void
_vk_spinbutton_put_wch(WINDOW *win, int y, int x, wchar_t wc,
    attr_t attr, short pair)
{
    cchar_t cc;
    wchar_t s[2];

    s[0] = wc;
    s[1] = L'\0';

    setcchar(&cc, s, attr, pair, NULL);
    mvwadd_wch(win, y, x, &cc);
}

/* re-pair a WACS box-drawing glyph and place it, layering on `extra`
   attrs (mirrors vk_input / vk_button) */
static void
_vk_spinbutton_relief_wch(WINDOW *win, int y, int x, const cchar_t *src,
    short pair, attr_t extra)
{
    cchar_t cc;
    wchar_t wch[CCHARW_MAX];
    attr_t  attrs;
    short   dummy;

    getcchar(src, wch, &attrs, &dummy, NULL);
    setcchar(&cc, wch, attrs | extra, pair, NULL);
    mvwadd_wch(win, y, x, &cc);
}

/*
    Resolve a relief direction to its near (top/left) and far (bottom/right)
    edges, each as a (colour pair, attrs) pair.  vk_button's rule: the
    highlight edge carries the widget's bold attrs so relief_hi reads as
    bright; the shadow edge is plain so relief_lo reads dark.  RAISED puts
    the highlight at the NW, SUNKEN at the SE; flat is a single face colour.
*/
static void
_vk_spinbutton_relief_pairs(int relief, short hi, short sh, short face,
    attr_t battr, short *np, attr_t *na, short *fp, attr_t *fa)
{
    if(relief & VK_RELIEF_RAISED)
    {
        *np = hi;   *na = battr;    /* highlight (bold) at top/left   */
        *fp = sh;   *fa = 0;        /* shadow (plain) at bottom/right */
    }
    else if(relief & VK_RELIEF_SUNKEN)
    {
        *np = sh;   *na = 0;        /* shadow at top/left      */
        *fp = hi;   *fa = battr;    /* highlight at bottom/right */
    }
    else
    {
        *np = face; *na = battr;
        *fp = face; *fa = battr;
    }
}

/* draw a full-height vertical separator at `col`, teeing into the frame
   top and bottom (relief-style aware) */
static void
_vk_spinbutton_draw_vsep(vk_spinbutton_t *spin, int col, short pair,
    attr_t attr)
{
    vk_widget_t *widget = VK_WIDGET(spin);
    int         bottom_row = widget->height - 1;
    int         r;

    if(spin->relief_style == VK_BUTTON_BASIC)
    {
        wattr_set(widget->canvas, attr, pair, NULL);
        mvwaddch(widget->canvas, 0, col, '|');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        return;
    }

    if(spin->relief_style == VK_BORDER_ASCII)
    {
        wattr_set(widget->canvas, attr, pair, NULL);
        mvwaddch(widget->canvas, 0, col, '+');
        for(r = 1; r < bottom_row; r++)
            mvwaddch(widget->canvas, r, col, '|');
        mvwaddch(widget->canvas, bottom_row, col, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        return;
    }

    _vk_spinbutton_relief_wch(widget->canvas, 0, col, WACS_TTEE, pair, attr);
    for(r = 1; r < bottom_row; r++)
        _vk_spinbutton_relief_wch(widget->canvas, r, col, WACS_VLINE,
            pair, attr);
    _vk_spinbutton_relief_wch(widget->canvas, bottom_row, col, WACS_BTEE,
        pair, attr);
}

/*
    Draw the relief frame as two regions split by the separator: the value
    field on the left and the arrow controls on the right, each with its
    own 3D direction.  Transition cells take their vertical-edge colour
    (vk_frame's rule), so the separator is the value field's right edge and
    the buttons' left edge at once -- with sunken/raised they agree.  A
    second divider splits the arrow controls into inc / dec.
*/
static void
_vk_spinbutton_draw_frame(vk_spinbutton_t *spin)
{
    vk_widget_t *widget = VK_WIDGET(spin);
    short   bg = widget->bg;
    short   fg = widget->fg;
    short   hi = vdk_color_pair(widget->relief_hi, bg);
    short   sh = vdk_color_pair(widget->relief_lo, bg);
    short   face = vdk_color_pair(fg, bg);
    attr_t  battr = widget->attrs;
    int     right_col = widget->width - 1;
    int     bottom_row = widget->height - 1;
    int     sep_col = _vk_spinbutton_sep_col(spin);
    short   vn, vf, bn, bf, sep;
    attr_t  vna, vfa, bna, bfa, sepa;
    int     down_col, mid_col;
    int     i;

    _vk_spinbutton_relief_pairs(spin->field_relief, hi, sh, face, battr,
        &vn, &vna, &vf, &vfa);
    _vk_spinbutton_relief_pairs(spin->button_relief, hi, sh, face, battr,
        &bn, &bna, &bf, &bfa);
    sep = vf; sepa = vfa;   /* separator = value field's (far) right edge */

    /* second divider between the inc (up) and dec (down) arrows; it sits
       in the gap cell just left of the down arrow, coloured like the
       button region's highlight edge so it reads with the raised relief */
    _vk_spinbutton_arrow_cols(spin, NULL, &down_col);
    mid_col = down_col - 1;

    if(spin->relief_style == VK_BUTTON_BASIC)
    {
        wattr_set(widget->canvas, widget->attrs, face, NULL);
        mvwaddch(widget->canvas, 0, 0, '[');
        mvwaddch(widget->canvas, 0, right_col, ']');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        wattr_set(widget->canvas, sepa, sep, NULL);
        mvwaddch(widget->canvas, 0, sep_col, '|');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        _vk_spinbutton_draw_vsep(spin, mid_col, bn, bna);
        return;
    }

    if(spin->relief_style == VK_BORDER_ASCII)
    {
        /* top row: value-near NW corner + top edge */
        wattr_set(widget->canvas, vna, vn, NULL);
        mvwaddch(widget->canvas, 0, 0, '+');
        for(i = 1; i < sep_col; i++)
            mvwaddch(widget->canvas, 0, i, '-');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* separator tee */
        wattr_set(widget->canvas, sepa, sep, NULL);
        mvwaddch(widget->canvas, 0, sep_col, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* button-near top edge */
        wattr_set(widget->canvas, bna, bn, NULL);
        for(i = sep_col + 1; i < right_col; i++)
            mvwaddch(widget->canvas, 0, i, '-');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* button-far NE corner */
        wattr_set(widget->canvas, bfa, bf, NULL);
        mvwaddch(widget->canvas, 0, right_col, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);

        for(i = 1; i < bottom_row; i++)
        {
            wattr_set(widget->canvas, vna, vn, NULL);
            mvwaddch(widget->canvas, i, 0, '|');
            wattr_set(widget->canvas, sepa, sep, NULL);
            mvwaddch(widget->canvas, i, sep_col, '|');
            wattr_set(widget->canvas, bfa, bf, NULL);
            mvwaddch(widget->canvas, i, right_col, '|');
            wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        }

        /* bottom row: value-near SW corner */
        wattr_set(widget->canvas, vna, vn, NULL);
        mvwaddch(widget->canvas, bottom_row, 0, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* value-far bottom edge */
        wattr_set(widget->canvas, vfa, vf, NULL);
        for(i = 1; i < sep_col; i++)
            mvwaddch(widget->canvas, bottom_row, i, '-');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* separator tee */
        wattr_set(widget->canvas, sepa, sep, NULL);
        mvwaddch(widget->canvas, bottom_row, sep_col, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        /* button-far bottom edge + SE corner */
        wattr_set(widget->canvas, bfa, bf, NULL);
        for(i = sep_col + 1; i < right_col; i++)
            mvwaddch(widget->canvas, bottom_row, i, '-');
        mvwaddch(widget->canvas, bottom_row, right_col, '+');
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        _vk_spinbutton_draw_vsep(spin, mid_col, bn, bna);
        return;
    }

    /* default: WACS box drawing, two regions */
    _vk_spinbutton_relief_wch(widget->canvas, 0, 0, WACS_ULCORNER, vn, vna);
    for(i = 1; i < sep_col; i++)
        _vk_spinbutton_relief_wch(widget->canvas, 0, i, WACS_HLINE, vn, vna);
    _vk_spinbutton_relief_wch(widget->canvas, 0, sep_col, WACS_TTEE, sep, sepa);
    for(i = sep_col + 1; i < right_col; i++)
        _vk_spinbutton_relief_wch(widget->canvas, 0, i, WACS_HLINE, bn, bna);
    _vk_spinbutton_relief_wch(widget->canvas, 0, right_col,
        WACS_URCORNER, bf, bfa);

    for(i = 1; i < bottom_row; i++)
    {
        _vk_spinbutton_relief_wch(widget->canvas, i, 0, WACS_VLINE, vn, vna);
        _vk_spinbutton_relief_wch(widget->canvas, i, sep_col,
            WACS_VLINE, sep, sepa);
        _vk_spinbutton_relief_wch(widget->canvas, i, right_col,
            WACS_VLINE, bf, bfa);
    }

    _vk_spinbutton_relief_wch(widget->canvas, bottom_row, 0,
        WACS_LLCORNER, vn, vna);
    for(i = 1; i < sep_col; i++)
        _vk_spinbutton_relief_wch(widget->canvas, bottom_row, i,
            WACS_HLINE, vf, vfa);
    _vk_spinbutton_relief_wch(widget->canvas, bottom_row, sep_col,
        WACS_BTEE, sep, sepa);
    for(i = sep_col + 1; i < right_col; i++)
        _vk_spinbutton_relief_wch(widget->canvas, bottom_row, i,
            WACS_HLINE, bf, bfa);
    _vk_spinbutton_relief_wch(widget->canvas, bottom_row, right_col,
        WACS_LRCORNER, bf, bfa);

    _vk_spinbutton_draw_vsep(spin, mid_col, bn, bna);
}

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_SPINBUTTON_KLASS)
{
    .size = KLASS_SIZE(vk_spinbutton_t),
    .name = KLASS_NAME(vk_spinbutton_t),
    .ctor = _vk_spinbutton_ctor,
    .dtor = _vk_spinbutton_dtor,
};

/* ── public API ──────────────────────────────────────────────────── */

inline vk_spinbutton_t*
vk_spinbutton_create(int width)
{
    /* need room for at least a digit or two plus the arrow cells */
    if(width < 8) return NULL;

    return (vk_spinbutton_t*)vk_object_create(VK_SPINBUTTON_KLASS, width, 3);
}

inline int
vk_spinbutton_set_range(vk_spinbutton_t *spin, double min, double max)
{
    if(spin == NULL) return -1;

    spin->min = min;
    spin->max = max;

    /* pull the current value back inside the new bounds (no notify) */
    _vk_spinbutton_apply(spin, spin->value, false);

    return 0;
}

inline int
vk_spinbutton_set_step(vk_spinbutton_t *spin, double step)
{
    if(spin == NULL) return -1;

    spin->step = step;

    return 0;
}

inline int
vk_spinbutton_set_value(vk_spinbutton_t *spin, double value)
{
    if(spin == NULL) return -1;

    spin->editing = false;
    _vk_spinbutton_apply(spin, value, false);

    return 0;
}

inline double
vk_spinbutton_get_value(vk_spinbutton_t *spin)
{
    if(spin == NULL) return 0.0;

    /* fold any pending manual edit so callers always read the committed
       number */
    _vk_spinbutton_commit(spin);

    return spin->value;
}

inline int
vk_spinbutton_set_precision(vk_spinbutton_t *spin, int precision)
{
    if(spin == NULL) return -1;

    spin->precision = (precision > 0) ? precision : 0;

    return 0;
}

inline int
vk_spinbutton_set_editable(vk_spinbutton_t *spin, bool editable)
{
    if(spin == NULL) return -1;

    spin->editable = editable;

    if(!editable)
        spin->editing = false;

    return 0;
}

inline int
vk_spinbutton_set_relief_style(vk_spinbutton_t *spin, int style)
{
    vk_widget_t *widget;

    if(spin == NULL) return -1;

    if(style != VK_BORDER_SINGLE && style != VK_BORDER_ASCII
        && style != VK_BUTTON_BASIC)
        return -1;

    spin->relief_style = style;

    widget = VK_WIDGET(spin);

    if(style == VK_BUTTON_BASIC)
        vk_widget_resize(widget, widget->width, 1);
    else
        vk_widget_resize(widget, widget->width, 3);

    return 0;
}

/* 3D relief direction of the value field: VK_RELIEF_SUNKEN /
   VK_RELIEF_RAISED, or 0 for flat.  Colours come from the widget's
   relief_hi / relief_lo (vk_widget_set_relief_colors). */
inline int
vk_spinbutton_set_field_relief(vk_spinbutton_t *spin, int relief)
{
    if(spin == NULL) return -1;

    spin->field_relief = relief;

    return 0;
}

/* 3D relief direction of the increment / decrement controls */
inline int
vk_spinbutton_set_button_relief(vk_spinbutton_t *spin, int relief)
{
    if(spin == NULL) return -1;

    spin->button_relief = relief;

    return 0;
}

inline int
vk_spinbutton_set_on_change(vk_spinbutton_t *spin, VkWidgetFunc func,
    void *anything)
{
    if(spin == NULL) return -1;

    spin->on_change = func;
    spin->on_change_arg = anything;

    return 0;
}

/* apply n increments of step (n may be negative); clamps and notifies */
inline int
vk_spinbutton_step(vk_spinbutton_t *spin, int n)
{
    if(spin == NULL) return -1;

    _vk_spinbutton_commit(spin);
    _vk_spinbutton_apply(spin, spin->value + (double)n * spin->step, true);

    return 0;
}

/*
    Route a mouse click given in widget-local coordinates (the containing
    dialog knows the spinbutton's absolute origin and subtracts it).
    Returns true if the click hit an arrow (or, when editable, the value
    field) and was consumed.
*/
inline bool
vk_spinbutton_click(vk_spinbutton_t *spin, int local_x, int local_y)
{
    int up_col, down_col;

    if(spin == NULL) return false;

    if(local_y != _vk_spinbutton_text_row(spin)) return false;

    _vk_spinbutton_arrow_cols(spin, &up_col, &down_col);

    if(local_x == up_col)
    {
        vk_spinbutton_step(spin, +1);
        _vk_spinbutton_update(spin);
        return true;
    }

    if(local_x == down_col)
    {
        vk_spinbutton_step(spin, -1);
        _vk_spinbutton_update(spin);
        return true;
    }

    /* a click in the value area (left of the separator) starts editing */
    if(spin->editable)
    {
        int field_start = _vk_spinbutton_field_start(spin);
        if(local_x >= field_start && local_x < _vk_spinbutton_sep_col(spin))
        {
            _vk_spinbutton_begin_edit(spin, true);
            _vk_spinbutton_update(spin);
            return true;
        }
    }

    return false;
}

inline int
vk_spinbutton_update(vk_spinbutton_t *spin)
{
    if(spin == NULL) return -1;

    return spin->_update(spin);
}

inline void
vk_spinbutton_destroy(vk_spinbutton_t *spin)
{
    if(spin == NULL) return;

    if(!vk_object_assert(spin, vk_spinbutton_t)) return;

    spin->dtor(VK_OBJECT(spin));
}

/* ── ctor / dtor ─────────────────────────────────────────────────── */

static int
_vk_spinbutton_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_spinbutton_t *spin;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    spin = VK_SPINBUTTON(object);

    spin->value = 0.0;
    spin->min = 0.0;
    spin->max = 100.0;
    spin->step = 1.0;
    spin->precision = 0;
    spin->relief_style = VK_BORDER_SINGLE;
    spin->field_relief = VK_RELIEF_SUNKEN;      /* input-like by default */
    spin->button_relief = VK_RELIEF_RAISED;     /* button-like by default */
    spin->editable = false;

    spin->editing = false;
    spin->edit_buf[0] = '\0';
    spin->edit_len = 0;
    spin->edit_cursor = 0;
    spin->edit_scroll = 0;

    spin->on_change = NULL;
    spin->on_change_arg = NULL;

    spin->ctor = _vk_spinbutton_ctor;
    spin->dtor = _vk_spinbutton_dtor;
    spin->_update = _vk_spinbutton_update;

    /* self-contained keyboard handling (Up/Down + the inline editor) */
    vk_object_set_kmio(object, _vk_spinbutton_kmio);

    return 0;
}

static int
_vk_spinbutton_dtor(vk_object_t *object)
{
    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_spinbutton_t)) return -1;

    /* no owned heap members -- edit_buf is inline */

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

/* ── draw ────────────────────────────────────────────────────────── */

static int
_vk_spinbutton_update(vk_spinbutton_t *spin)
{
    vk_widget_t *widget;
    short       fg, bg;
    short       face_pair;
    int         field_start;
    int         field_width;
    int         value_width;
    int         text_row;
    int         up_col, down_col;
    int         i;

    const char  *shown;
    int         shown_len;
    int         cursor;
    bool        show_cursor;
    char        valbuf[VK_SPINBUTTON_EDIT_CAP];

    if(spin == NULL) return -1;

    widget = VK_WIDGET(spin);
    widget->_erase(widget);

    fg = widget->fg;
    bg = widget->bg;

    face_pair = vdk_color_pair(fg, bg);

    vk_widget_fill_pair(widget, L' ', widget->attrs, face_pair);

    /* relief frame: sunken/raised value field and arrow controls, split
       by the separator (see _vk_spinbutton_draw_frame) */
    _vk_spinbutton_draw_frame(spin);

    field_start = _vk_spinbutton_field_start(spin);
    field_width = _vk_spinbutton_field_width(spin);
    value_width = _vk_spinbutton_value_width(spin);
    text_row = _vk_spinbutton_text_row(spin);
    _vk_spinbutton_arrow_cols(spin, &up_col, &down_col);

    if(field_width < 1) return 0;

    /* choose what the value cell shows: the live edit buffer or the
       formatted number */
    if(spin->editing)
    {
        shown = spin->edit_buf;
        shown_len = spin->edit_len;
        cursor = spin->edit_cursor;
        show_cursor = true;

        if(cursor < spin->edit_scroll)
            spin->edit_scroll = cursor;
        if(value_width > 0 && cursor > spin->edit_scroll + value_width - 1)
            spin->edit_scroll = cursor - value_width + 1;
        if(spin->edit_scroll < 0)
            spin->edit_scroll = 0;
    }
    else
    {
        _vk_spinbutton_format(spin, valbuf, sizeof(valbuf));
        shown = valbuf;
        shown_len = (int)strlen(valbuf);
        cursor = 0;
        show_cursor = false;
        spin->edit_scroll = 0;
    }

    for(i = 0; i < value_width; i++)
    {
        int     idx = spin->edit_scroll + i;
        char    ch = (idx < shown_len) ? shown[idx] : ' ';
        attr_t  attrs = widget->attrs;

        if(show_cursor && idx == cursor)
            attrs |= A_REVERSE;

        wattr_set(widget->canvas, attrs, face_pair, NULL);
        mvwaddch(widget->canvas, text_row, field_start + i, ch);
        wattr_set(widget->canvas, A_NORMAL, 0, NULL);
    }

    /* the increment / decrement arrows */
    {
        attr_t  a = widget->attrs | A_BOLD;
        short   pair = vdk_color_pair(fg, bg);

        if(spin->relief_style == VK_BORDER_ASCII)
        {
            wattr_set(widget->canvas, a, pair, NULL);
            mvwaddch(widget->canvas, text_row, up_col, '+');
            mvwaddch(widget->canvas, text_row, down_col, '-');
            wattr_set(widget->canvas, A_NORMAL, 0, NULL);
        }
        else
        {
            _vk_spinbutton_put_wch(widget->canvas, text_row, up_col,
                L'▲', a, pair);
            _vk_spinbutton_put_wch(widget->canvas, text_row, down_col,
                L'▼', a, pair);
        }
    }

    return 0;
}

/* ── keyboard ────────────────────────────────────────────────────── */

static int
_vk_spinbutton_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_spinbutton_t *spin;

    if(object == NULL) return keystroke;

    spin = VK_SPINBUTTON(object);

    if(keystroke == KEY_UP)
    {
        vk_spinbutton_step(spin, +1);
        _vk_spinbutton_update(spin);
        return 0;
    }

    if(keystroke == KEY_DOWN)
    {
        vk_spinbutton_step(spin, -1);
        _vk_spinbutton_update(spin);
        return 0;
    }

    /* everything below is manual-edit only */
    if(!spin->editable) return keystroke;

    if(keystroke == KEY_CRLF || keystroke == '\n' || keystroke == KEY_ENTER)
    {
        if(spin->editing)
        {
            _vk_spinbutton_commit(spin);
            _vk_spinbutton_update(spin);
            return 0;
        }
        return keystroke;
    }

    if(keystroke == 27)         /* Esc -- abandon the edit, keep the value */
    {
        if(spin->editing)
        {
            spin->editing = false;
            spin->edit_scroll = 0;
            _vk_spinbutton_update(spin);
            return 0;
        }
        return keystroke;
    }

    if((keystroke >= '0' && keystroke <= '9')
        || keystroke == '.' || keystroke == '-')
    {
        if(!spin->editing)
            _vk_spinbutton_begin_edit(spin, false);
        _vk_spinbutton_edit_insert(spin, (char)keystroke);
        _vk_spinbutton_update(spin);
        return 0;
    }

    if(spin->editing)
    {
        switch(keystroke)
        {
            case KEY_BACKSPACE:
            case 127:
            case 8:
                _vk_spinbutton_edit_backspace(spin);
                break;

            case KEY_LEFT:
                if(spin->edit_cursor > 0) spin->edit_cursor--;
                break;

            case KEY_RIGHT:
                if(spin->edit_cursor < spin->edit_len) spin->edit_cursor++;
                break;

            case KEY_HOME:
                spin->edit_cursor = 0;
                break;

            case KEY_END:
                spin->edit_cursor = spin->edit_len;
                break;

            default:
                return keystroke;
        }

        _vk_spinbutton_update(spin);
        return 0;
    }

    return keystroke;
}
