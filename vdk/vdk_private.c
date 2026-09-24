#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <langinfo.h>

#include <ncursesw/ncurses.h>

#include "vdk.h"
#include "vk_widget.h"
#include "vdk_private.h"

/* see vdk_private.h -- the single relief-cell primitive. */
void
vdk_relief_wch(WINDOW *win, int y, int x, const cchar_t *src,
    short pair, attr_t attr)
{
    wchar_t wch[CCHARW_MAX];
    attr_t  attrs;
    short   dummy;
    cchar_t cc;

    if(win == NULL || src == NULL) return;

    /* wadd_wch merges the window's current attrs into the cell; neutralise
       them so the cell carries exactly the pair + attr requested. */
    wattr_set(win, A_NORMAL, 0, NULL);

    getcchar(src, wch, &attrs, &dummy, NULL);
    setcchar(&cc, wch, attrs | attr, pair, NULL);
    mvwadd_wch(win, y, x, &cc);
}

/* see vdk_private.h for the contract. */
void
vdk_draw_relief(vk_widget_t *widget, int relief, short bg, attr_t extra)
{
    WINDOW  *canvas;
    int     right, bottom, i;
    short   hi, sh, nw_pair, se_pair;
    attr_t  hi_attrs, sh_attrs, nw_attr, se_attr;

    if(widget == NULL || widget->canvas == NULL) return;
    if(widget->width < 2 || widget->height < 2) return;

    canvas = widget->canvas;
    right  = widget->width  - 1;
    bottom = widget->height - 1;

    hi = vdk_color_pair(widget->relief_hi, bg);
    sh = vdk_color_pair(widget->relief_lo, bg);

    hi_attrs = VDK_RELIEF_HI_ATTRS(extra);
    sh_attrs = VDK_RELIEF_SH_ATTRS(extra);

    if(relief & VK_RELIEF_SUNKEN)
    {
        nw_pair = sh; nw_attr = sh_attrs;   /* shadow at top/left        */
        se_pair = hi; se_attr = hi_attrs;   /* highlight at bottom/right */
    }
    else
    {
        nw_pair = hi; nw_attr = hi_attrs;   /* highlight at top/left     */
        se_pair = sh; se_attr = sh_attrs;   /* shadow at bottom/right    */
    }

    /* top edge + UL corner (NW) */
    vdk_relief_wch(canvas, 0, 0, WACS_ULCORNER, nw_pair, nw_attr);
    for(i = 1; i < right; i++)
        vdk_relief_wch(canvas, 0, i, WACS_HLINE, nw_pair, nw_attr);

    /* top-right corner takes the right (SE) edge's colour */
    vdk_relief_wch(canvas, 0, right, WACS_URCORNER, se_pair, se_attr);

    /* left edge (NW); right edge (SE) */
    for(i = 1; i < bottom; i++)
        vdk_relief_wch(canvas, i, 0, WACS_VLINE, nw_pair, nw_attr);
    for(i = 1; i < bottom; i++)
        vdk_relief_wch(canvas, i, right, WACS_VLINE, se_pair, se_attr);

    /* bottom-left corner takes the left (NW) edge's colour */
    vdk_relief_wch(canvas, bottom, 0, WACS_LLCORNER, nw_pair, nw_attr);

    /* bottom edge + LR corner (SE) */
    for(i = 1; i < right; i++)
        vdk_relief_wch(canvas, bottom, i, WACS_HLINE, se_pair, se_attr);
    vdk_relief_wch(canvas, bottom, right, WACS_LRCORNER, se_pair, se_attr);
}

/* see vdk_private.h -- size + place the edge scrollbars (call from _on_resize). */
void
vdk_scroller_reflow(vk_widget_t *widget)
{
    if(widget == NULL) return;

    if(widget->vscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->vscroller), 1, widget->height);
        vk_widget_move(VK_WIDGET(widget->vscroller), widget->width - 1, 0);
    }

    if(widget->hscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->hscroller), widget->width, 1);
        vk_widget_move(VK_WIDGET(widget->hscroller), 0, widget->height - 1);
    }
}

/* see vdk_private.h -- re-point the bars at the rebuilt canvas (from _recreate). */
void
vdk_scroller_recreate(vk_widget_t *widget)
{
    if(widget == NULL) return;

    if(widget->vscroller != NULL)
    {
        VK_WIDGET(widget->vscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        VK_WIDGET(widget->hscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->hscroller));
    }
}

/* see vdk_private.h -- refresh + composite each visible bar (call from _update). */
void
vdk_scroller_draw(vk_widget_t *widget)
{
    if(widget == NULL) return;

    if(widget->vscroller != NULL)
    {
        if(vk_scroller_update(widget->vscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        if(vk_scroller_update(widget->hscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->hscroller));
    }
}

/* see vdk_private.h -- the canvas-reset shared by every widget's _recreate. */
int
vdk_widget_reset_canvas(vk_widget_t *widget)
{
    int width;
    int height;

    if (widget == NULL)
    {
        return -1;
    }

    width = (widget->width < 1) ? 1 : widget->width;
    height = (widget->height < 1) ? 1 : widget->height;

    if (widget->composer != widget->canvas)
    {
        delwin(widget->composer);
    }

    widget->canvas = newwin(height, width, 0, 0);
    widget->composer = widget->canvas;
    widget->state &= ~VK_STATE_FROZEN;

    return (widget->canvas == NULL) ? -1 : 0;
}

int
vdk_put_text_cols(WINDOW *win, int y, int x, const char *text, int cols)
{
    mbstate_t   ps;
    const char  *p = text;
    size_t      left;
    int         used = 0;

    if(win == NULL || cols <= 0) return 0;

    wmove(win, y, x);
    memset(&ps, 0, sizeof(ps));
    left = (text != NULL) ? strlen(text) : 0;

    while(left > 0)
    {
        wchar_t wc;
        wchar_t one[2];
        size_t  n = mbrtowc(&wc, p, left, &ps);
        int     w;

        if(n == (size_t)-1 || n == (size_t)-2)
        {
            /* invalid / truncated sequence: show one '?' and resync */
            memset(&ps, 0, sizeof(ps));
            wc = L'?';
            n = 1;
        }
        if(n == 0) break;

        w = wcwidth(wc);
        if(w < 0) w = 1;                    /* control char: one cell */
        if(used + w > cols) break;          /* would overflow: stop here */

        one[0] = wc;
        one[1] = L'\0';
        waddnwstr(win, one, 1);
        used += w;
        p += n;
        left -= n;
    }

    while(used < cols)
    {
        waddch(win, ' ');
        used++;
    }

    return used;
}

/*
    Can the terminal show UTF-8?  The locale's CODESET must be UTF-8, and
    the bare Linux console (TERM=linux) is treated as unable even then: its
    font lacks most symbols.  Cached; the locale must be set (setlocale)
    before the first call.
*/
bool
vdk_has_utf8(void)
{
    static int  cached = -1;
    const char  *term;

    if(cached >= 0) return cached != 0;

    cached = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0) ? 1 : 0;
    term = getenv("TERM");
    if(term != NULL && strcmp(term, "linux") == 0) cached = 0;

    return cached != 0;
}
