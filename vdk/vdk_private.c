#include <string.h>

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
