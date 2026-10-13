#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ncursesw/ncurses.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_event.h"
#include "vdk_private.h"

static int
_vk_widget_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_widget_dtor(vk_object_t *object);

static int
_vk_widget_move(vk_widget_t *widget, int x, int y);

static int
_vk_widget_blit(vk_widget_t *widget);

static int
_vk_widget_resize(vk_widget_t *widget, int width, int height);

static int
_vk_widget_recreate(vk_widget_t *widget);

static int
_vk_widget_erase(vk_widget_t *widget);

static int
_vk_widget_redraw(vk_object_t *object);

static int
_vk_widget_redraw_self(vk_widget_t *widget);

require_klass(VK_OBJECT_KLASS);

/* process-lifetime widget ids.  0 is reserved (null getter / unset). */
static uint32_t vk_widget_id_seq = 0;

declare_klass(VK_WIDGET_KLASS)
{
    .size = KLASS_SIZE(vk_widget_t),
    .name = KLASS_NAME(vk_widget_t),
    .ctor = _vk_widget_ctor,
    .dtor = _vk_widget_dtor,
    .parent = &VK_OBJECT_KLASS,
    .redraw = _vk_widget_redraw,
};

// create a new widget from scratch
inline vk_widget_t*
vk_widget_create(int width, int height)
{
    vk_widget_t     *widget;

    if(height == 0 || width == 0) return NULL;

    widget = (vk_widget_t*)vk_object_create(VK_WIDGET_KLASS,
        width, height);

    return widget;
}

inline int
vk_widget_set_surface(vk_widget_t *widget, WINDOW *surface)
{
    if(widget == NULL) return -1;
    if(surface == NULL) return -1;

    widget->surface = surface;

    return 0;
}

inline WINDOW*
vk_widget_get_surface(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->surface;
}

inline int
vk_widget_blit(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;
    if(!(widget->state & VK_STATE_VISIBLE)) return 0;

    retval = widget->_blit(widget);

    return retval;
}

inline uint32_t
vk_widget_get_state(vk_widget_t *widget)
{
    if(widget == NULL) return 0;

    return widget->state;
}

inline void
vk_widget_set_state(vk_widget_t *widget, uint32_t state)
{
    uint32_t    old_state;

    if(widget == NULL) return;

    old_state = widget->state;

    /* the bits the library keeps for itself are not the caller's to
       change: whatever was passed for them, they stay as they were */
    state = (state & ~VDK_STATE_PRIVATE) | (old_state & VDK_STATE_PRIVATE);

    widget->state = state;

    if((state & VK_STATE_FROZEN) && !(old_state & VK_STATE_FROZEN))
    {
        int frozen_w = (widget->width < 1) ? 1 : widget->width;
        int frozen_h = (widget->height < 1) ? 1 : widget->height;

        widget->composer = newwin(frozen_h, frozen_w, 0, 0);
        if (widget->composer == NULL)
        {
            widget->composer = widget->canvas;
            return;
        }
        overwrite(widget->canvas, widget->composer);
    }

    if(!(state & VK_STATE_FROZEN) && (old_state & VK_STATE_FROZEN))
    {
        if(widget->composer != widget->canvas)
        {
            delwin(widget->composer);
            widget->composer = widget->canvas;
        }
    }
}

inline void
vk_widget_set_colors(vk_widget_t *widget, int fg, int bg)
{
    if(widget == NULL) return;

    widget->fg = fg;
    widget->bg = bg;

    /* colours given to a widget are its own: it stops following its
       container's, and VK_INHERIT_AUTO will leave them be */
    widget->state &= ~(VK_STATE_INHERIT_FG | VK_STATE_INHERIT_BG);
    widget->state |= (VDK_STATE_OWN_FG | VDK_STATE_OWN_BG);

    return;
}

inline void
vk_widget_set_relief_colors(vk_widget_t *widget, short hi, short lo)
{
    if(widget == NULL) return;

    widget->relief_hi = hi;
    widget->relief_lo = lo;
}

inline void
vk_widget_set_attrs(vk_widget_t *widget, attr_t attrs)
{
    if(widget == NULL) return;

    widget->attrs = attrs;

    /* attributes given to a widget are its own */
    widget->state &= ~VK_STATE_INHERIT_ATTRS;
}

/*
    Is `prop` exactly one of the properties a program may set?  The raw
    state word has room for more than there are, and holds bits that
    belong to the library.
*/
static bool
_vk_widget_is_prop(uint32_t prop)
{
    switch(prop)
    {
        case VK_PROP_VISIBLE:
        case VK_PROP_FROZEN:
        case VK_PROP_NORESIZE:
        case VK_PROP_EXPAND:
        case VK_PROP_INHERIT_FG:
        case VK_PROP_INHERIT_BG:
        case VK_PROP_INHERIT_ATTRS:
            return true;

        default:
            return false;
    }
}

/*
    Turn one property on or off, leaving the others as they are.
*/
int
vk_widget_set_prop(vk_widget_t *widget, uint32_t prop, bool on)
{
    uint32_t    state;

    if(widget == NULL) return -1;
    if(!_vk_widget_is_prop(prop)) return -1;

    state = widget->state;

    if(on)
        state |= prop;
    else
        state &= ~prop;

    /* through vk_widget_set_state, which does the work that freezing
       and thawing take */
    vk_widget_set_state(widget, state);

    /* a colour that is to follow the container is no longer the
       widget's own; one that stops following is kept as it stands */
    if(prop == VK_PROP_INHERIT_FG)
    {
        if(on)
            widget->state &= ~VDK_STATE_OWN_FG;
        else
            widget->state |= VDK_STATE_OWN_FG;
    }

    if(prop == VK_PROP_INHERIT_BG)
    {
        if(on)
            widget->state &= ~VDK_STATE_OWN_BG;
        else
            widget->state |= VDK_STATE_OWN_BG;
    }

    return 0;
}

/*
    Is one property on?
*/
bool
vk_widget_get_prop(vk_widget_t *widget, uint32_t prop)
{
    if(widget == NULL) return false;
    if(!_vk_widget_is_prop(prop)) return false;

    return (widget->state & prop) != 0;
}

inline int
vk_widget_get_colors(vk_widget_t *widget, short *fg, short *bg)
{
    if(widget == NULL) return -1;

    if(fg != NULL) *fg = widget->fg;
    if(bg != NULL) *bg = widget->bg;

    return 0;
}

inline attr_t
vk_widget_get_attrs(vk_widget_t *widget)
{
    if(widget == NULL) return 0;

    return widget->attrs;
}

/*
    Repaint a widget of any type as what it is: whatever its klass (or
    the nearest ancestor that has one) supplies as its redraw.
*/
int
vk_widget_redraw(vk_widget_t *widget)
{
    vk_object_t *object = VK_OBJECT(widget);

    if(widget == NULL) return -1;

    /* nothing to paint with: not a failure */
    if(object->redraw == NULL) return 0;

    return object->redraw(object);
}

/*
    Repaint a widget whose children are known to be current.  A resize
    and a recreate both end with this: each child has just been through
    the same and repainted itself, so a container among them copies its
    children in without repainting them all again (which, down a deep
    tree, would paint the innermost widget once per level).
*/
static int
_vk_widget_redraw_self(vk_widget_t *widget)
{
    int retval;

    widget->state |= VDK_STATE_CHILDREN_CURRENT;
    retval = vk_widget_redraw(widget);
    widget->state &= ~VDK_STATE_CHILDREN_CURRENT;

    return retval;
}

/*
    The redraw of a plain widget: it has no contents of its own to
    paint, so there is nothing to do.  It is here so that every widget
    klass has a redraw, and a klass can always call its parent's.
*/
static int
_vk_widget_redraw(vk_object_t *object)
{
    (void)object;

    return 0;
}

inline WINDOW*
vk_widget_get_canvas(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->canvas;
}

/*
    Row `y` of the widget as portable cells.  The composer is read, not
    the canvas: it is what is copied to the surface, so it is what is
    on show.  The two are the same window unless the widget is frozen,
    when the composer is the snapshot taken at that moment.
*/
int
vk_widget_read_row(vk_widget_t *widget, int y, vdk_cell_t *cells, int cols)
{
    if(widget == NULL) return -1;

    return vdk_cells_read_row(widget->composer, y, cells, cols);
}

inline int
vk_widget_get_metrics(vk_widget_t *widget, int *width, int *height)
{
    if(widget == NULL) return -1;
    if(width == NULL && height == NULL) return -1;

    if(width !=NULL) *width = widget->width;
    if(height != NULL) *height = widget->height;

    return 0;
}

inline int
vk_widget_get_position(vk_widget_t *widget, int *x, int *y)
{
    if(widget == NULL) return -1;

    if(x != NULL) *x = widget->x;
    if(y != NULL) *y = widget->y;

    return 0;
}

inline int
vk_widget_resize(vk_widget_t *widget, int width, int height)
{
    int retval;

    if(widget == NULL) return -1;
    if(widget->state & VK_STATE_NORESIZE) return -1;
    if(width == WSIZE_UNCHANGED) width = widget->width;
    if(height == WSIZE_UNCHANGED) height = widget->height;

    /* ncurses newwin/wresize reject 0; a 0-high expand filler segfaults. */
    if (width < 1 || height < 1)
    {
        return -1;
    }
    if(width == widget->width && height == widget->height) return 0;

    retval = widget->_resize(widget, width, height);
    if(retval != 0) return retval;

    /*
        the canvas has its new size.  what follows is the library's own
        work, done here so that no widget type has to arrange for it:
        scroll bars attached to the widget are refitted to it, the type
        arranges what it holds, and the widget repaints itself.
    */
    vdk_scroller_reflow(widget);

    if(widget->_layout != NULL) widget->_layout(widget);

    /* a widget with no canvas (a deck) has nothing to repaint.  what
       the widget holds was resized, and so repainted, by _layout */
    if(widget->canvas != NULL) _vk_widget_redraw_self(widget);

    /* only now is anyone else told: the widget is resized and painted */
    vk_object_emit(VK_OBJECT(widget), VK_EVENT_ON_RESIZE);

    return 0;
}

inline int
vk_widget_erase(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_erase(widget);

    return retval;
}

inline void
vk_widget_fill(vk_widget_t *widget, chtype ch)
{
    long            i;

    if(widget == NULL) return;
    if(widget->canvas == NULL) return;

    wmove(widget->canvas, 0, 0);

    i = widget->width * widget->height;

    while(i)
    {
        waddch(widget->canvas, ch);
        i--;
    }

    return;
}

inline void
vk_widget_fill_pair(vk_widget_t *widget, wchar_t wch, attr_t attrs, short pair)
{
    cchar_t     cc;
    wchar_t     buf[2];
    long        i;

    if(widget == NULL) return;
    if(widget->canvas == NULL) return;

    /* a cchar_t carries the pair as a full short, unlike COLOR_PAIR()
       which truncates it into the 8-bit A_COLOR field. */
    buf[0] = wch;
    buf[1] = L'\0';
    setcchar(&cc, buf, attrs, pair, NULL);

    wmove(widget->canvas, 0, 0);

    i = widget->width * widget->height;

    while(i)
    {
        wadd_wch(widget->canvas, &cc);
        i--;
    }

    return;
}


inline int
vk_widget_move(vk_widget_t *widget, int x, int y)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_move(widget, x, y);

    return retval;
}

inline int
vk_widget_recreate(vk_widget_t *widget)
{
    int retval;

    if(widget == NULL) return -1;

    retval = widget->_recreate(widget);
    if(retval != 0) return retval;

    /*
        the widget has a fresh, blank canvas.  the library's own work
        follows, so that no widget type has to arrange for it: scroll
        bars attached to the widget are given fresh canvases too, and
        the widget repaints itself.
    */
    vdk_scroller_recreate(widget);

    /* a widget with no canvas (a deck) has nothing to repaint.  what
       the widget holds was recreated, and so repainted, by _recreate */
    if(widget->canvas != NULL) _vk_widget_redraw_self(widget);

    /* only now is anyone else told */
    vk_object_emit(VK_OBJECT(widget), VK_EVENT_ON_RECREATE);

    return 0;
}

inline void
vk_widget_set_userptr(vk_widget_t *widget, void *ptr)
{
    if(widget == NULL) return;

    widget->anything = ptr;
}

inline void*
vk_widget_get_userptr(vk_widget_t *widget)
{
    if(widget == NULL) return NULL;

    return widget->anything;
}

inline uint32_t
vk_widget_get_id(vk_widget_t *widget)
{
    if(widget == NULL) return 0;

    return widget->id;
}

inline int
vk_widget_destroy(vk_widget_t *widget)
{
    if(widget == NULL) return -1;

    /* a widget of a derived type -- a window, a button -- handed to
       the general call: destroy it as what it is.  (This used to
       return without doing anything, which silently leaked every
       widget destroyed through a plain widget pointer.)  The derived
       destructors come back here once they have stepped the object
       down to a plain widget.  Unlike the typed calls, this one takes
       any widget, so the only refusal is NULL. */
    if(!vk_object_assert(widget, vk_widget_t))
    {
        vk_object_dispose(VK_OBJECT(widget));
        return 0;
    }

    /* first of all, and once: tell anyone listening that it is going */
    _vk_object_announce_destroy(VK_OBJECT(widget));

    widget->dtor(VK_OBJECT(widget));

    return 0;
}

static int
_vk_widget_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_widget_t *widget;
    va_list     args;
    int         width;
    int         height;

    if(object == NULL) return -1;

    /*
        if argp is set then we're being called by a superclass.
        otherwise, we're being called directly.
    */
    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    // install our derived klass methods
    widget = VK_WIDGET(object);

    widget->fg = COLOR_BLACK;
    widget->bg = COLOR_WHITE;
    widget->relief_hi = COLOR_WHITE;
    widget->relief_lo = COLOR_BLACK;
    widget->state = VK_STATE_VISIBLE;

    widget->id = ++vk_widget_id_seq;
    if(widget->id == 0)
        widget->id = ++vk_widget_id_seq;

    widget->ctor = _vk_widget_ctor;
    widget->dtor = _vk_widget_dtor;

    widget->_move = _vk_widget_move;
    widget->_blit = _vk_widget_blit;
    widget->_resize = _vk_widget_resize;
    widget->_recreate = _vk_widget_recreate;
    widget->_erase = _vk_widget_erase;

    // interate through var args after constructing base klass
    width = va_arg(*argp, int);
    height = va_arg(*argp, int);
    va_end(args);

    if (width < 1)
    {
        width = 1;
    }
    if (height < 1)
    {
        height = 1;
    }

    widget->canvas = newwin(height, width, 0, 0);
    widget->composer = widget->canvas;
    widget->width = width;
    widget->height = height;
    widget->x = 0;
    widget->y = 0;

    /* self-initialize the list node so list_del() is safe even for a
       widget that is never added to a deck/container */
    INIT_LIST_HEAD(&widget->list);

    return 0;
}

static int
_vk_widget_move(vk_widget_t *widget, int x, int y)
{
    int     retval = 0;

    if(widget == NULL) return -1;
    if(x == WPOS_UNCHANGED && y == WPOS_UNCHANGED) return 0;

    if(x == WPOS_UNCHANGED) x = widget->x;
    if(y == WPOS_UNCHANGED) y = widget->y;

    retval = mvwin(widget->canvas, y, x);

    if(retval == ERR) return -1;

    widget->x = x;
    widget->y = y;

    return 0;
}

static int
_vk_widget_resize(vk_widget_t *widget, int width, int height)
{
    WINDOW  *copy_pad;
    int     pad_width;
    int     pad_height;
    int     pad_x;
    int     pad_y;
    int     copy_h;
    int     copy_w;

    getmaxyx(widget->canvas, pad_height, pad_width);
    getbegyx(widget->canvas, pad_y, pad_x);

    /* the copy that carries the old contents across the resize is the
       size of the canvas, so every cell goes with it.  (It used to be
       a row and a column smaller, which lost the last of each.)
       newwin(0, ...) means "the rest of the screen", so never 0. */
    copy_h = (pad_height > 0) ? pad_height : 1;
    copy_w = (pad_width > 0) ? pad_width : 1;
    copy_pad = newwin(copy_h, copy_w, pad_y, pad_x);
    if (copy_pad == NULL)
    {
        return -1;
    }
    overwrite(widget->canvas, copy_pad);

    werase(widget->canvas);
    if (wresize(widget->canvas, height, width) == ERR)
    {
        delwin(copy_pad);
        return -1;
    }

    overwrite(copy_pad, widget->canvas);
    delwin(copy_pad);

    widget->width = width;
    widget->height = height;

    return 0;
}

static int
_vk_widget_blit(vk_widget_t *widget)
{
    int     retval;
    int     dmincol;
    int     dminrow;
    int     dmaxcol;
    int     dmaxrow;
    int     max_x;
    int     max_y;

    if(widget == NULL) return -1;

    getmaxyx(widget->surface, max_y, max_x);

    dmincol = widget->x;
    dminrow = widget->y;

    // set copy horizontal boundary
    if(widget->x + widget->width > max_x)
        dmaxcol = max_x;
    else
        dmaxcol = widget->x + widget->width;

    // set copy vertical boundary
    if(widget->y + widget->height > max_y)
        dmaxrow = max_y;
    else
        dmaxrow = widget->y + widget->height;

    retval = copywin(widget->composer, widget->surface,
        0, 0,
        dminrow, dmincol,
        dmaxrow - 1, dmaxcol -1,
        FALSE);

    if(retval == ERR) return -1;

    return 0;
}

static int
_vk_widget_recreate(vk_widget_t *widget)
{
    return vdk_widget_reset_canvas(widget);
}

static int
_vk_widget_erase(vk_widget_t *widget)
{
    if(widget == NULL) return -1;
    if(widget->canvas == NULL) return -1;

    werase(widget->canvas);

    return 0;
}

static int
_vk_widget_dtor(vk_object_t *object)
{
    vk_widget_t *widget;

    if(object == NULL) return -1;

    widget = VK_WIDGET(object);

    /* a widget that is destroyed while a container or a deck still
       lists it takes itself off that list, so the holder is not left
       with a pointer into freed memory to trip over later.  (A node
       that was never listed points at itself; one that was removed
       has been cleared.) */
    if(widget->list.next != NULL && widget->list.next != &widget->list)
        list_del(&widget->list);

    /* scrollers attached to the widget go with it.  One that should
       outlive it has to be detached first (vk_widget_detach_scroller).
       The scroller's own destructor clears the link back to us. */
    if(widget->vscroller != NULL)
        vk_object_dispose(VK_OBJECT(widget->vscroller));
    if(widget->hscroller != NULL)
        vk_object_dispose(VK_OBJECT(widget->hscroller));

    if(widget->composer != widget->canvas)
        delwin(widget->composer);

    delwin(widget->canvas);

    vk_object_demote(object, vk_object_t);
    vk_object_destroy(object);

    return 0;
}
