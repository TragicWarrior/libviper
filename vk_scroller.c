#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "viper.h"
#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_frame.h"
#include "vk_scroller.h"

static int
_vk_scroller_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_scroller_dtor(vk_object_t *object);

static int
_vk_scroller_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_scroller_update(vk_frame_t *frame);

static int
_vk_scroller_draw_scrollbar(vk_scroller_t *scroller);

static void
_vk_scroller_draw_vscroll_ascii(vk_scroller_t *scroller,
    int track_x, int track_start, int track_len, int thumb_pos,
    int border_colors);

static void
_vk_scroller_draw_vscroll_unicode(vk_scroller_t *scroller,
    int track_x, int track_start, int track_len, int thumb_pos,
    short color_pair);

static void
_vk_scroller_draw_hscroll_ascii(vk_scroller_t *scroller,
    int track_y, int track_start, int track_len, int thumb_pos,
    int border_colors);

static void
_vk_scroller_draw_hscroll_unicode(vk_scroller_t *scroller,
    int track_y, int track_start, int track_len, int thumb_pos,
    short color_pair);


require_klass(VK_FRAME_KLASS);

declare_klass(VK_SCROLLER_KLASS)
{
    .size = KLASS_SIZE(vk_scroller_t),
    .name = KLASS_NAME(vk_scroller_t),
    .ctor = _vk_scroller_ctor,
    .dtor = _vk_scroller_dtor,
    .kmio = _vk_scroller_kmio,
};


vk_scroller_t*
vk_scroller_create(int width, int height)
{
    vk_scroller_t   *scroller;

    if(height < 3 || width < 3) return NULL;

    scroller = (vk_scroller_t*)vk_object_create(VK_SCROLLER_KLASS,
        width, height);

    return scroller;
}

int
vk_scroller_set_border_style(vk_scroller_t *scroller, int style)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    return VK_FRAME(scroller)->_set_border_style(VK_FRAME(scroller), style);
}

int
vk_scroller_set_border_colors(vk_scroller_t *scroller, short fg, short bg)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    VK_FRAME(scroller)->border_fg = fg;
    VK_FRAME(scroller)->border_bg = bg;

    return 0;
}

int
vk_scroller_set_child(vk_scroller_t *scroller, vk_widget_t *child)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    return VK_FRAME(scroller)->_set_child(VK_FRAME(scroller), child);
}

vk_widget_t*
vk_scroller_get_child(vk_scroller_t *scroller)
{
    if(scroller == NULL) return NULL;

    if(!vk_object_assert(scroller, vk_scroller_t)) return NULL;

    return VK_FRAME(scroller)->child;
}

int
vk_scroller_set_scrollbar(vk_scroller_t *scroller, int flags)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    scroller->scrollbar_flags = flags;

    return 0;
}

int
vk_scroller_set_scroll_info(vk_scroller_t *scroller, VkScrollInfoFunc func)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    scroller->scroll_info_func = func;

    return 0;
}

int
vk_scroller_update(vk_scroller_t *scroller)
{
    if(scroller == NULL) return -1;

    if(!vk_object_assert(scroller, vk_scroller_t)) return -1;

    return VK_FRAME(scroller)->_update(VK_FRAME(scroller));
}

void
vk_scroller_destroy(vk_scroller_t *scroller)
{
    if(scroller == NULL) return;

    if(!vk_object_assert(scroller, vk_scroller_t)) return;

    scroller->dtor(VK_OBJECT(scroller));

    return;
}


static int
_vk_scroller_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_frame_t  *frame;

    frame = VK_FRAME(object);

    if(frame->child == NULL) return 0;

    return vk_object_push_keystroke(VK_OBJECT(frame->child), keystroke);
}

static int
_vk_scroller_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_scroller_t   *scroller;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;

        VK_FRAME_KLASS->ctor(object, &args);
        va_end(args);
    }
    else
    {
        VK_FRAME_KLASS->ctor(object, argp);
    }

    scroller = VK_SCROLLER(object);

    scroller->scroll_info_func = NULL;
    scroller->content_height = 0;
    scroller->content_width = 0;
    scroller->scroll_y = 0;
    scroller->scroll_x = 0;
    scroller->scrollbar_flags = VK_SCROLLBAR_VERTICAL;

    scroller->ctor = _vk_scroller_ctor;
    scroller->dtor = _vk_scroller_dtor;

    scroller->_draw_scrollbar = _vk_scroller_draw_scrollbar;

    VK_FRAME(scroller)->_update = _vk_scroller_update;

    return 0;
}

static int
_vk_scroller_dtor(vk_object_t *object)
{
    vk_frame_t      *frame;
    vk_container_t  *container;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_scroller_t)) return -1;

    frame = VK_FRAME(object);
    container = VK_CONTAINER(object);

    if(frame->child != NULL)
    {
        container->remove_widget(container, frame->child);
        frame->child = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_scroller_update(vk_frame_t *frame)
{
    vk_scroller_t   *scroller;
    vk_widget_t     *widget;

    if(frame == NULL) return -1;

    scroller = VK_SCROLLER(frame);
    widget = VK_WIDGET(frame);

    if(scroller->scroll_info_func != NULL && frame->child != NULL)
    {
        scroller->scroll_info_func(frame->child,
            &scroller->content_height, &scroller->content_width,
            &scroller->scroll_y, &scroller->scroll_x);
    }

    widget->_erase(widget);
    frame->_draw_border(frame);
    scroller->_draw_scrollbar(scroller);

    if(frame->child != NULL)
    {
        vk_widget_draw(frame->child);
    }

    return 0;
}

static int
_vk_scroller_draw_scrollbar(vk_scroller_t *scroller)
{
    vk_frame_t      *frame;
    vk_widget_t     *widget;
    short           fg, bg;
    short           color_pair;
    int             border_colors;
    int             viewport_h;
    int             viewport_w;
    int             scroll_range;
    int             track_len;
    int             thumb_pos;

    if(scroller == NULL) return -1;

    frame = VK_FRAME(scroller);
    widget = VK_WIDGET(scroller);

    if(frame->border_style == VK_FRAME_NONE) return 0;

    fg = (frame->border_fg == -1) ? widget->fg : frame->border_fg;
    bg = (frame->border_bg == -1) ? widget->bg : frame->border_bg;
    color_pair = viper_color_pair(fg, bg);
    border_colors = VIPER_COLORS(fg, bg);

    viewport_h = widget->height - 2;
    viewport_w = widget->width - 2;

    // vertical scrollbar on right border
    if((scroller->scrollbar_flags & VK_SCROLLBAR_VERTICAL)
        && scroller->content_height > viewport_h
        && viewport_h >= 3)
    {
        track_len = viewport_h;
        scroll_range = scroller->content_height - viewport_h;

        if(scroll_range > 0)
            thumb_pos = (scroller->scroll_y * (track_len - 1))
                / scroll_range;
        else
            thumb_pos = 0;

        if(thumb_pos >= track_len) thumb_pos = track_len - 1;
        if(thumb_pos < 0) thumb_pos = 0;

        if(frame->border_style == VK_FRAME_ASCII)
        {
            _vk_scroller_draw_vscroll_ascii(scroller,
                widget->width - 1, 1, track_len, thumb_pos,
                border_colors);
        }
        else
        {
            _vk_scroller_draw_vscroll_unicode(scroller,
                widget->width - 1, 1, track_len, thumb_pos,
                color_pair);
        }
    }

    // horizontal scrollbar on bottom border
    if((scroller->scrollbar_flags & VK_SCROLLBAR_HORIZONTAL)
        && scroller->content_width > viewport_w
        && viewport_w >= 3)
    {
        track_len = viewport_w;
        scroll_range = scroller->content_width - viewport_w;

        if(scroll_range > 0)
            thumb_pos = (scroller->scroll_x * (track_len - 1))
                / scroll_range;
        else
            thumb_pos = 0;

        if(thumb_pos >= track_len) thumb_pos = track_len - 1;
        if(thumb_pos < 0) thumb_pos = 0;

        if(frame->border_style == VK_FRAME_ASCII)
        {
            _vk_scroller_draw_hscroll_ascii(scroller,
                widget->height - 1, 1, track_len, thumb_pos,
                border_colors);
        }
        else
        {
            _vk_scroller_draw_hscroll_unicode(scroller,
                widget->height - 1, 1, track_len, thumb_pos,
                color_pair);
        }
    }

    return 0;
}

static void
_vk_scroller_draw_vscroll_ascii(vk_scroller_t *scroller,
    int track_x, int track_start, int track_len, int thumb_pos,
    int border_colors)
{
    vk_widget_t *widget = VK_WIDGET(scroller);
    int         i;

    wattron(widget->canvas, border_colors);

    mvwaddch(widget->canvas, 0, track_x, '^');
    mvwaddch(widget->canvas, widget->height - 1, track_x, 'v');

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            mvwaddch(widget->canvas, track_start + i, track_x, '#');
        else
            mvwaddch(widget->canvas, track_start + i, track_x, '|');
    }

    wattroff(widget->canvas, border_colors);
}

static void
_vk_scroller_draw_vscroll_unicode(vk_scroller_t *scroller,
    int track_x, int track_start, int track_len, int thumb_pos,
    short color_pair)
{
    vk_widget_t *widget = VK_WIDGET(scroller);
    cchar_t     ch;
    wchar_t     wch[2] = {0, 0};
    int         i;

    wch[0] = 0x25B2;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(widget->canvas, 0, track_x, &ch);

    wch[0] = 0x25BC;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(widget->canvas, widget->height - 1, track_x, &ch);

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            wch[0] = 0x2588;
        else
            wch[0] = 0x2592;

        setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
        mvwadd_wch(widget->canvas, track_start + i, track_x, &ch);
    }
}

static void
_vk_scroller_draw_hscroll_ascii(vk_scroller_t *scroller,
    int track_y, int track_start, int track_len, int thumb_pos,
    int border_colors)
{
    vk_widget_t *widget = VK_WIDGET(scroller);
    int         i;

    wattron(widget->canvas, border_colors);

    mvwaddch(widget->canvas, track_y, 0, '<');
    mvwaddch(widget->canvas, track_y, widget->width - 1, '>');

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            mvwaddch(widget->canvas, track_y, track_start + i, '=');
        else
            mvwaddch(widget->canvas, track_y, track_start + i, '-');
    }

    wattroff(widget->canvas, border_colors);
}

static void
_vk_scroller_draw_hscroll_unicode(vk_scroller_t *scroller,
    int track_y, int track_start, int track_len, int thumb_pos,
    short color_pair)
{
    vk_widget_t *widget = VK_WIDGET(scroller);
    cchar_t     ch;
    wchar_t     wch[2] = {0, 0};
    int         i;

    wch[0] = 0x25C4;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(widget->canvas, track_y, 0, &ch);

    wch[0] = 0x25BA;
    setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
    mvwadd_wch(widget->canvas, track_y, widget->width - 1, &ch);

    for(i = 0; i < track_len; i++)
    {
        if(i == thumb_pos)
            wch[0] = 0x2588;
        else
            wch[0] = 0x2592;

        setcchar(&ch, wch, A_NORMAL, color_pair, NULL);
        mvwadd_wch(widget->canvas, track_y, track_start + i, &ch);
    }
}
