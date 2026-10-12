#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "vdk.h"
#include "vdk_private.h"

/*
    vdk_cells -- a canvas read back as portable cells.

    Everything a libviper program draws ends up in an ncurses WINDOW: a
    widget's canvas, or the canvas of a screen's surface where the
    widgets are composed.  What such a WINDOW holds is private to this
    process -- a character that may be a key into the terminal's
    line-drawing set, attribute bits in ncurses' own layout, and a
    colour-pair number that only this process's ncurses can resolve.

    vdk_cells_read_row() works all of that out and returns the row as
    vdk_cell_t: the character as Unicode, the two colours as RGB, and
    the attributes that still mean something once the colours are
    final.  It is the one place that knows how to do it, for anything
    that needs to show a canvas somewhere other than this terminal -- an
    image of it, or a copy of it in another program.
*/

/* colours 0 .. 255 are the ones with a standard meaning */
#define VDK_CELLS_PALETTE_SIZE      256

/*
    The standard xterm 256-colour palette, as 0x00RRGGBB: the 16 ANSI
    colours, a 6x6x6 colour cube, and 24 grays.  This is what a colour
    number means until a program redefines it.

    ncurses cannot be asked for this.  For a colour nobody has
    redefined it reports a placeholder (the 8 basic colours over and
    over), so "what is colour 196" answers "blue".
*/
static uint32_t     cells_palette[VDK_CELLS_PALETTE_SIZE];
static int          cells_palette_ready = 0;

/* what ncurses reported for each colour when nothing had been
   redefined, packed as 0x00RRGGBB; valid when cells_baseline_ready */
static uint32_t     cells_baseline[VDK_CELLS_PALETTE_SIZE];
static int          cells_baseline_ready = 0;

/* fill in cells_palette */
static void
_vdk_cells_palette_init(void)
{
    static const uint32_t ansi16[16] =
    {
        0x000000, 0xCD0000, 0x00CD00, 0xCDCD00,
        0x0000EE, 0xCD00CD, 0x00CDCD, 0xE5E5E5,
        0x7F7F7F, 0xFF0000, 0x00FF00, 0xFFFF00,
        0x5C5CFF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
    };
    static const uint32_t cube[6] = { 0, 95, 135, 175, 215, 255 };
    uint32_t    v;
    int         i, r, g, b;

    if(cells_palette_ready) return;

    for(i = 0; i < 16; i++)
        cells_palette[i] = ansi16[i];

    /* the colour cube, 16 .. 231 */
    i = 16;
    for(r = 0; r < 6; r++)
        for(g = 0; g < 6; g++)
            for(b = 0; b < 6; b++)
                cells_palette[i++] = (cube[r] << 16) | (cube[g] << 8) | cube[b];

    /* the gray ramp, 232 .. 255 */
    for(i = 0; i < 24; i++)
    {
        v = (uint32_t)(8 + i * 10);
        cells_palette[232 + i] = (v << 16) | (v << 8) | v;
    }

    cells_palette_ready = 1;
}

/*
    What ncurses says colour `idx` is, as 0x00RRGGBB.  Returns 0 and
    fills in *rgb, or -1 when ncurses has no such colour.  ncurses
    counts each part from 0 to 1000.
*/
static int
_vdk_cells_ncurses_rgb(int idx, uint32_t *rgb)
{
    int r = 0, g = 0, b = 0;

    if(idx < 0 || idx >= COLORS) return -1;
    if(extended_color_content(idx, &r, &g, &b) != OK) return -1;

    /* keep within range whatever came back */
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
    if(r > 1000) r = 1000;
    if(g > 1000) g = 1000;
    if(b > 1000) b = 1000;

    *rgb = ((uint32_t)(r * 255 / 1000) << 16)
         | ((uint32_t)(g * 255 / 1000) << 8)
         |  (uint32_t)(b * 255 / 1000);

    return 0;
}

void
_vdk_cells_colors_baseline(void)
{
    int i;

    for(i = 0; i < VDK_CELLS_PALETTE_SIZE; i++)
    {
        /* a colour this terminal does not have keeps a value no real
           reading can equal, so it is never taken for untouched */
        if(_vdk_cells_ncurses_rgb(i, &cells_baseline[i]) != 0)
            cells_baseline[i] = 0xFFFFFFFF;
    }

    cells_baseline_ready = 1;
}

/*
    The RGB value of colour `idx`.  A negative number is the terminal's
    default colour: `dflt` is returned and *is_default set.

    A colour a program has redefined (true colour, or an OSC 4 palette
    change) is whatever ncurses now says it is.  One nobody has touched
    is its standard palette entry.  The two are told apart by whether
    ncurses still reports what it did at the baseline.
*/
static uint32_t
_vdk_cells_color_rgb(int idx, uint32_t dflt, int *is_default)
{
    uint32_t    now;

    *is_default = 0;

    if(idx < 0)
    {
        *is_default = 1;
        return dflt;
    }

    _vdk_cells_palette_init();

    /* a colour with a standard meaning */
    if(idx < VDK_CELLS_PALETTE_SIZE)
    {
        if(cells_baseline_ready
            && _vdk_cells_ncurses_rgb(idx, &now) == 0
            && now != cells_baseline[idx])
        {
            return now;         /* redefined since start-up */
        }

        return cells_palette[idx];
    }

    /* beyond the standard palette there is only ncurses to ask */
    if(_vdk_cells_ncurses_rgb(idx, &now) == 0) return now;

    *is_default = 1;
    return dflt;
}

/*
    Cells drawn with A_ALTCHARSET store a VT100 line-graphics key (e.g. 'q'
    for a horizontal line, 'a' for the checkerboard) rather than the glyph
    itself.  Translate that key to the matching Unicode code point so a
    reader draws the intended box-drawing / block character instead of a
    literal Latin letter.
*/
static uint32_t
_vdk_cells_acs_to_unicode(uint32_t key)
{
    switch(key)
    {
        case 'a': return 0x2592;    /* checkerboard (CKBOARD)        */
        case 'h': return 0x2592;    /* board of squares (BOARD)      */
        case '0': return 0x2588;    /* solid block (BLOCK)           */
        case 'q': return 0x2500;    /* horizontal line (HLINE)       */
        case 'x': return 0x2502;    /* vertical line (VLINE)         */
        case 'l': return 0x250C;    /* upper-left corner (ULCORNER)  */
        case 'k': return 0x2510;    /* upper-right corner (URCORNER) */
        case 'm': return 0x2514;    /* lower-left corner (LLCORNER)  */
        case 'j': return 0x2518;    /* lower-right corner (LRCORNER) */
        case 't': return 0x251C;    /* left tee (LTEE)               */
        case 'u': return 0x2524;    /* right tee (RTEE)              */
        case 'v': return 0x2534;    /* bottom tee (BTEE)             */
        case 'w': return 0x252C;    /* top tee (TTEE)                */
        case 'n': return 0x253C;    /* cross / plus (PLUS)           */
        case '`': return 0x25C6;    /* diamond (DIAMOND)             */
        case 'f': return 0x00B0;    /* degree (DEGREE)               */
        case 'g': return 0x00B1;    /* plus/minus (PLMINUS)          */
        case ',': return 0x2190;    /* left arrow (LARROW)           */
        case '+': return 0x2192;    /* right arrow (RARROW)          */
        case '.': return 0x2193;    /* down arrow (DARROW)           */
        case '-': return 0x2191;    /* up arrow (UARROW)             */
        case '~': return 0x00B7;    /* bullet (BULLET)               */
        case 'o': return 0x23BA;    /* scan line 1 (S1)              */
        case 'p': return 0x23BB;    /* scan line 3 (S3)              */
        case 'r': return 0x23BC;    /* scan line 7 (S7)              */
        case 's': return 0x23BD;    /* scan line 9 (S9)              */
        case 'y': return 0x2264;    /* less than or equal (LEQUAL)   */
        case 'z': return 0x2265;    /* greater than or equal (GEQUAL)*/
        case '{': return 0x03C0;    /* pi (PI)                       */
        case '|': return 0x2260;    /* not equal (NEQUAL)            */
        case '}': return 0x00A3;    /* pound sterling (STERLING)     */
        default:  return key;       /* unknown: keep the key as-is   */
    }
}

/* a blank cell in the default colours */
static void
_vdk_cells_blank(vdk_cell_t *cell)
{
    cell->ch = ' ';
    cell->fg = VDK_CELL_DEFAULT_FG;
    cell->bg = VDK_CELL_DEFAULT_BG;
    cell->attrs = 0;
    cell->flags = VDK_CELL_FG_DEFAULT | VDK_CELL_BG_DEFAULT;
}

/* the colour halfway between two, part by part */
static uint32_t
_vdk_cells_blend(uint32_t a, uint32_t b)
{
    return (((VDK_CELL_R(a) + VDK_CELL_R(b)) / 2) << 16)
         | (((VDK_CELL_G(a) + VDK_CELL_G(b)) / 2) << 8)
         |  ((VDK_CELL_B(a) + VDK_CELL_B(b)) / 2);
}

int
vdk_cells_read_row(WINDOW *win, int y, vdk_cell_t *cells, int cols)
{
    int     rows, width;
    int     x;

    if(win == NULL || cells == NULL || cols <= 0) return -1;

    getmaxyx(win, rows, width);
    if(y < 0 || y >= rows) return -1;

    for(x = 0; x < cols; x++)
    {
        vdk_cell_t  *cell = &cells[x];
        cchar_t     cc;
        wchar_t     wbuf[CCHARW_MAX + 1];
        attr_t      attrs = 0;
        short       pair_short = 0;
        int         pair = 0;
        int         pf = -1, pb = -1;
        int         fg_default = 0, bg_default = 0;
        uint32_t    ch;
        uint32_t    swap;
        int         swap_flag;

        _vdk_cells_blank(cell);

        /* past the window's edge, or a cell that cannot be read */
        if(x >= width) continue;
        if(mvwin_wch(win, y, x, &cc) != OK) continue;

        wbuf[0] = 0;

        /* the pair comes back twice: as a short, which cannot hold the
           larger pair numbers, and through the last argument as an
           int, which can.  the int is the one to believe */
        if(getcchar(&cc, wbuf, &attrs, &pair_short, &pair) != OK)
            continue;

        /* only the first character of the cell is kept: a combining
           mark that follows it is left out */
        ch = (wbuf[0] == 0) ? ' ' : (uint32_t)wbuf[0];

        if(attrs & A_ALTCHARSET)
            ch = _vdk_cells_acs_to_unicode(ch);

        cell->ch = ch;

        /* the pair's two colour numbers, then what those colours are */
        if(extended_pair_content(pair, &pf, &pb) != OK)
        {
            pf = -1;
            pb = -1;
        }

        cell->fg = _vdk_cells_color_rgb(pf, VDK_CELL_DEFAULT_FG, &fg_default);
        cell->bg = _vdk_cells_color_rgb(pb, VDK_CELL_DEFAULT_BG, &bg_default);

        /* reverse video (standout is drawn the same way) swaps the
           two, and which of them was a default goes along */
        if(attrs & (A_REVERSE | A_STANDOUT))
        {
            swap = cell->fg;
            cell->fg = cell->bg;
            cell->bg = swap;

            swap_flag = fg_default;
            fg_default = bg_default;
            bg_default = swap_flag;
        }

        /* dim is rarely honoured by a terminal: draw it instead, by
           moving the foreground halfway to the background.  the result
           is a colour of its own, no longer the terminal's default */
        if(attrs & A_DIM)
        {
            cell->fg = _vdk_cells_blend(cell->fg, cell->bg);
            fg_default = 0;
        }

        cell->flags = 0;
        if(fg_default) cell->flags |= VDK_CELL_FG_DEFAULT;
        if(bg_default) cell->flags |= VDK_CELL_BG_DEFAULT;

        cell->attrs = 0;
        if(attrs & A_BOLD)      cell->attrs |= VDK_CELL_BOLD;
        if(attrs & A_UNDERLINE) cell->attrs |= VDK_CELL_UNDERLINE;
        if(attrs & A_BLINK)     cell->attrs |= VDK_CELL_BLINK;
#ifdef A_ITALIC
        if(attrs & A_ITALIC)    cell->attrs |= VDK_CELL_ITALIC;
#endif

        /* a double-width character: this cell holds it, the next one
           is its second half and is not read as a cell of its own */
        if(ch > 0x7F && wcwidth((wchar_t)ch) == 2 && x + 1 < cols)
        {
            cell->flags |= VDK_CELL_WIDE;

            cells[x + 1] = *cell;
            cells[x + 1].ch = 0;
            cells[x + 1].flags &= (uint16_t)~VDK_CELL_WIDE;
            cells[x + 1].flags |= VDK_CELL_WIDE_TAIL;

            x++;
        }
    }

    return cols;
}
