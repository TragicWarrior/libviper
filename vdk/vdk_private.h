#ifndef _VDK_PRIVATE_H_
#define _VDK_PRIVATE_H_

/*
    vdk_private -- helpers shared across widget implementations but not part of
    any widget's public API.  This header is intentionally NOT installed and is
    NOT pulled in by vdk.h; only the widget .c files that use a helper include
    it, so it adds nothing to the public surface or to widgets that don't need
    it.
*/

#include <ncursesw/ncurses.h>

#include "vk_widget.h"

/*
    The relief attr rule, in one place: the highlight edge is always drawn
    A_BOLD so relief_hi (white) reads bright, and the shadow edge is never
    drawn A_BOLD so relief_lo (black) reads dark -- bold-on-black is bright
    black (grey), which washes the shadow out.  Any other attrs the caller
    passes (e.g. A_REVERSE) layer onto both edges.  Widgets that compose
    relief by hand (vk_dropdown, vk_spinbutton, the ASCII fallbacks) apply
    these to their edge cells so every widget shades identically.
*/
#define VDK_RELIEF_HI_ATTRS(extra)  ((extra) | A_BOLD)
#define VDK_RELIEF_SH_ATTRS(extra)  ((extra) & ~(attr_t)A_BOLD)

/*
    The single 3D-relief-cell primitive: recolour a WACS_ box glyph (keep its
    wide char + ALTCHARSET) with `pair` + `attr` and place it at (y, x).  Drawn
    per-cell with mvwadd_wch because mvwhline/mvwvline over an ACS chtype ignore
    the window pair/attr.  The window's own attributes are neutralised first --
    wadd_wch merges them into the cell, so a stray window-level A_BOLD (e.g. a
    frame's border_attrs) would wash the shadow edge out.  The cell you get is
    exactly the pair + attr you pass.  vdk_draw_relief() and the composite
    widgets that can't use it (vk_dropdown, vk_spinbutton) all draw every
    relief cell through this one function.
*/
void    vdk_relief_wch(WINDOW *win, int y, int x, const cchar_t *src,
            short pair, attr_t attr);

/*
    Draw a full single-line 3D relief box on the widget's canvas, using its
    relief_hi (highlight) and relief_lo (shadow) colours over `bg` -- pass the
    background the border actually sits on (a frame's border-resolved bg, or
    plain widget->bg).  The relief helper of choice for any widget whose
    relief IS a plain box.

    The highlight/shadow shading is fixed by VDK_RELIEF_HI/SH_ATTRS -- callers
    cannot get one edge bright and the other washed out.  `extra` layers extra
    attrs (e.g. A_REVERSE, a frame's border_attrs) onto both edges; A_BOLD in
    it is redundant on the highlight and stripped from the shadow.  RAISED puts
    the highlight at the NW (top + left), the shadow at the SE (bottom +
    right); SUNKEN swaps them.  The transition corners (UR, LL) take their
    vertical edge's colour.
*/
void    vdk_draw_relief(vk_widget_t *widget, int relief, short bg,
            attr_t extra);

/*
    Edge-scrollbar wiring, shared by every widget that hosts a vscroller /
    hscroller (vk_frame, vk_listbox, vk_textbox, vk_selectbox).  The vertical
    bar runs down the right column, the horizontal bar along the bottom row; a
    NULL scroller is skipped.  Each helper mirrors one lifecycle step so the
    host's _on_resize / _recreate / _update just calls the matching one instead
    of hand-wiring both bars identically in every widget:

      vdk_scroller_reflow    -- size + place the bars at the edges (_on_resize)
      vdk_scroller_recreate  -- re-point the bars at the rebuilt canvas and
                                recreate theirs (_recreate)
      vdk_scroller_draw      -- refresh each bar and composite it if it wants to
                                be shown (_update)
*/
void    vdk_scroller_reflow(vk_widget_t *widget);
void    vdk_scroller_recreate(vk_widget_t *widget);
void    vdk_scroller_draw(vk_widget_t *widget);

#endif  /* _VDK_PRIVATE_H_ */
