#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_scroller.h"

static int
_vk_scroller_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_scroller_dtor(vk_object_t *object);

static int
_vk_scroller_update(vk_scroller_t *scroller);

static int
_vk_scroller_draw_scrollbar(vk_scroller_t *scroller);

static void
_vk_scroller_draw_vscroll_ascii(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    int border_colors);

static void
_vk_scroller_draw_vscroll_unicode(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    short color_pair);

static void
_vk_scroller_draw_hscroll_ascii(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    int border_colors);

static void
_vk_scroller_draw_hscroll_unicode(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    short color_pair);

require_klass(VK_WIDGET_KLASS);

declare_klass(VK_SCROLLER_KLASS)
{
    .size = KLASS_SIZE(vk_scroller_t),
    .name = KLASS_NAME(vk_scroller_t),
    .ctor = _vk_scroller_ctor,
    .dtor = _vk_scroller_dtor,
};

inline vk_scroller_t*
vk_scroller_create(int flags)
{
    vk_scroller_t   *scroller;

    scroller = (vk_scroller_t*)vk_object_create(VK_SCROLLER_KLASS,
        1, 1, flags);

    return scroller;
}

inline int
vk_scroller_set_border_style(vk_scroller_t *scroller, int style)
{
    if(scroller == NULL) return -1;

    int base = style & ~VK_BORDER_REVERSE;
    if(base < VK_BORDER_NONE || base > VK_BORDER_ASCII) return -1;

    scroller->border_style = style;

    return 0;
}

inline int
vk_scroller_set_border_colors(vk_scroller_t *scroller, short fg, short bg)
{
    if(scroller == NULL) return -1;

    scroller->border_fg = fg;
    scroller->border_bg = bg;

    return 0;
}

inline int
vk_scroller_set_scroll_info(vk_scroller_t *scroller, VkScrollInfoFunc func)
{
    if(scroller == NULL) return -1;

    scroller->scroll_info_func = func;

    return 0;
}

inline int
vk_scroller_set_scroll_source(vk_scroller_t *scroller, vk_widget_t *source)
{
    if(scroller == NULL) return -1;

    scroller->scroll_source = source;

    return 0;
}

inline int
vk_scroller_update(vk_scroller_t *scroller)
{
    if(scroller == NULL) return -1;

    return scroller->_update(scroller);
}

inline void
vk_scroller_destroy(vk_scroller_t *scroller)
{
    if(scroller == NULL) return;

    if(!vk_object_assert(scroller, vk_scroller_t)) return;

    scroller->dtor(VK_OBJECT(scroller));

    return;
}

inline int
vk_widget_attach_scroller(vk_widget_t *host, vk_scroller_t *scroller)
{
    vk_widget_t *sw;

    if(host == NULL || scroller == NULL) return -1;

    if(scroller->scrollbar_flags & VK_SCROLLBAR_VERTICAL)
    {
        if(host->vscroller != NULL) return -1;
        host->vscroller = scroller;
    }
    else if(scroller->scrollbar_flags & VK_SCROLLBAR_HORIZONTAL)
    {
        if(host->hscroller != NULL) return -1;
        host->hscroller = scroller;
    }
    else return -1;

    scroller->host = host;

    sw = VK_WIDGET(scroller);

    vk_widget_set_surface(sw, host->canvas);

    if(scroller->scrollbar_flags & VK_SCROLLBAR_VERTICAL)
    {
        vk_widget_resize(sw, 1, host->height);
        vk_widget_move(sw, host->width - 1, 0);
    }
    else
    {
        vk_widget_resize(sw, host->width, 1);
        vk_widget_move(sw, 0, host->height - 1);
    }

    return 0;
}

inline int
vk_widget_detach_scroller(vk_widget_t *host, vk_scroller_t *scroller)
{
    if(host == NULL || scroller == NULL) return -1;

    if(host->vscroller == scroller)
        host->vscroller = NULL;
    else if(host->hscroller == scroller)
        host->hscroller = NULL;
    else
        return -1;

    scroller->host = NULL;
    VK_WIDGET(scroller)->surface = NULL;

    return 0;
}

static int
_vk_scroller_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_scroller_t   *scroller;
    va_list         args;
    int             flags;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    flags = va_arg(*argp, int);

    va_end(args);

    scroller = VK_SCROLLER(object);

    scroller->host = NULL;
    scroller->scroll_source = NULL;
    scroller->scroll_info_func = NULL;
    scroller->content_height = 0;
    scroller->content_width = 0;
    scroller->scroll_y = 0;
    scroller->scroll_x = 0;
    scroller->scrollbar_flags = flags;

    scroller->border_style = VK_BORDER_SINGLE;
    scroller->border_fg = -1;
    scroller->border_bg = -1;

    scroller->ctor = _vk_scroller_ctor;
    scroller->dtor = _vk_scroller_dtor;

    scroller->_update = _vk_scroller_update;
    scroller->_draw_scrollbar = _vk_scroller_draw_scrollbar;

    return 0;
}

static int
_vk_scroller_dtor(vk_object_t *object)
{
    vk_scroller_t   *scroller;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_scroller_t)) return -1;

    scroller = VK_SCROLLER(object);

    if(scroller->host != NULL)
    {
        if(scroller->host->vscroller == scroller)
            scroller->host->vscroller = NULL;
        if(scroller->host->hscroller == scroller)
            scroller->host->hscroller = NULL;
        scroller->host = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_scroller_update(vk_scroller_t *scroller)
{
    vk_widget_t *sw;
    int         viewport;

    if(scroller == NULL) return -1;
    if(scroller->host == NULL) return -1;

    if(scroller->scroll_info_func != NULL && scroller->scroll_source != NULL)
    {
        scroller->scroll_info_func(scroller->scroll_source,
            &scroller->content_height, &scroller->content_width,
            &scroller->scroll_y, &scroller->scroll_x);
    }

    sw = VK_WIDGET(scroller);

    if((scroller->border_style & ~VK_BORDER_REVERSE) == VK_BORDER_NONE)
        return 0;

    if(scroller->scrollbar_flags & VK_SCROLLBAR_VERTICAL)
    {
        viewport = sw->height - 2;
        if(scroller->content_height <= viewport || viewport < 3)
            return 0;
    }
    else if(scroller->scrollbar_flags & VK_SCROLLBAR_HORIZONTAL)
    {
        viewport = sw->width - 2;
        if(scroller->content_width <= viewport || viewport < 3)
            return 0;
    }

    scroller->_draw_scrollbar(scroller);

    return 1;
}

static int
_vk_scroller_draw_scrollbar(vk_scroller_t *scroller)
{
    vk_widget_t     *sw;
    short           fg, bg;
    short           color_pair;
    int             border_colors;
    int             viewport;
    int             scroll_range;
    int             track_len;
    int             thumb_pos;

    if(scroller == NULL) return -1;

    sw = VK_WIDGET(scroller);

    if(scroller->border_style == VK_BORDER_NONE) return 0;

    fg = (scroller->border_fg == -1) ? sw->fg : scroller->border_fg;
    bg = (scroller->border_bg == -1) ? sw->bg : scroller->border_bg;
    color_pair = vdk_color_pair(fg, bg);
    border_colors = COLOR_PAIR(vdk_color_pair(fg, bg));

    wattron(sw->canvas, border_colors);
    vk_widget_fill(sw, ' ');

    if(scroller->scrollbar_flags & VK_SCROLLBAR_VERTICAL)
    {
        viewport = sw->height - 2;

        if(scroller->content_height <= viewport || viewport < 3)
            return 0;

        track_len = viewport;
        scroll_range = scroller->content_height - viewport;

        if(scroll_range > 0)
            thumb_pos = (scroller->scroll_y * (track_len - 1))
                / scroll_range;
        else
            thumb_pos = 0;

        if(thumb_pos >= track_len) thumb_pos = track_len - 1;
        if(thumb_pos < 0) thumb_pos = 0;

        if((scroller->border_style & ~VK_BORDER_REVERSE) == VK_BORDER_ASCII)
        {
            _vk_scroller_draw_vscroll_ascii(scroller,
                1, track_len, thumb_pos, border_colors);
        }
        else
        {
            _vk_scroller_draw_vscroll_unicode(scroller,
                1, track_len, thumb_pos, color_pair);
        }
    }
    else if(scroller->scrollbar_flags & VK_SCROLLBAR_HORIZONTAL)
    {
        viewport = sw->width - 2;

        if(scroller->content_width <= viewport || viewport < 3)
            return 0;

        track_len = viewport;
        scroll_range = scroller->content_width - viewport;

        if(scroll_range > 0)
            thumb_pos = (scroller->scroll_x * (track_len - 1))
                / scroll_range;
        else
            thumb_pos = 0;

        if(thumb_pos >= track_len) thumb_pos = track_len - 1;
        if(thumb_pos < 0) thumb_pos = 0;

        if((scroller->border_style & ~VK_BORDER_REVERSE) == VK_BORDER_ASCII)
        {
            _vk_scroller_draw_hscroll_ascii(scroller,
                1, track_len, thumb_pos, border_colors);
        }
        else
        {
            _vk_scroller_draw_hscroll_unicode(scroller,
                1, track_len, thumb_pos, color_pair);
        }
    }

    return 0;
}

static void
_vk_scroller_draw_vscroll_ascii(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    int border_colors)
{
    vk_widget_t *sw = VK_WIDGET(scroller);
    int         i;

    wattron(sw->canvas, border_colors);

    mvwaddch(sw->canvas, 0, 0, '^');
    mvwaddch(sw->canvas, sw->height - 1, 0, 'v');

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            mvwaddch(sw->canvas, track_start + i, 0, '#');
        else
            mvwaddch(sw->canvas, track_start + i, 0, '|');
    }

    wattroff(sw->canvas, border_colors);
}

static void
_vk_scroller_draw_vscroll_unicode(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    short color_pair)
{
    vk_widget_t *sw = VK_WIDGET(scroller);
    cchar_t     ch;
    wchar_t     wch[2] = {0, 0};
    int         i;

    wch[0] = 0x25B2;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(sw->canvas, 0, 0, &ch);

    wch[0] = 0x25BC;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(sw->canvas, sw->height - 1, 0, &ch);

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            wch[0] = 0x2588;
        else
            wch[0] = 0x2592;

        setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
        mvwadd_wch(sw->canvas, track_start + i, 0, &ch);
    }
}

static void
_vk_scroller_draw_hscroll_ascii(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    int border_colors)
{
    vk_widget_t *sw = VK_WIDGET(scroller);
    int         i;

    wattron(sw->canvas, border_colors);

    mvwaddch(sw->canvas, 0, 0, '<');
    mvwaddch(sw->canvas, 0, sw->width - 1, '>');

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            mvwaddch(sw->canvas, 0, track_start + i, '=');
        else
            mvwaddch(sw->canvas, 0, track_start + i, '-');
    }

    wattroff(sw->canvas, border_colors);
}

static void
_vk_scroller_draw_hscroll_unicode(vk_scroller_t *scroller,
    int track_start, int track_len, int thumb_pos,
    short color_pair)
{
    vk_widget_t *sw = VK_WIDGET(scroller);
    cchar_t     ch;
    wchar_t     wch[2] = {0, 0};
    int         i;

    wch[0] = 0x25C4;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(sw->canvas, 0, 0, &ch);

    wch[0] = 0x25BA;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(sw->canvas, 0, sw->width - 1, &ch);

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            wch[0] = 0x2588;
        else
            wch[0] = 0x2592;

        setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
        mvwadd_wch(sw->canvas, 0, track_start + i, &ch);
    }
}
