/*
    test_redraw -- headless checks that a widget looks after itself.

    The rule being tested: a widget behaves correctly with no event
    handlers registered at all.  The library registers none on its own
    objects; after a resize, or after its canvas is thrown away and
    made again (which is what moving to another terminal does), a
    widget repaints itself without anyone asking.

    For each widget type the test paints it, then resizes it and
    compares its canvas with what an explicit redraw at that size
    produces, then recreates it and compares again.

    It also checks that redrawing a container repaints what it holds, to
    any depth, without the container knowing what the types are: one of
    the widgets in the tree is of a type defined here, in the test.

    Exit status 0 when every check passes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "vdk.h"
#include "vk_object.h"
#include "vk_widget.h"

static int  passed = 0;
static int  failed = 0;

/* record one check */
static void
check(const char *type, const char *what, int ok)
{
    if(ok) passed++;
    else failed++;

    printf("%s  %-12s %s\n", ok ? "PASS" : "FAIL", type, what);
}

/* does the object have any event handler registered on it? */
static int
has_handlers(void *object)
{
    return !list_empty(&VK_OBJECT(object)->event_handlers);
}

/* a widget's canvas as cells: `cells` must hold width * height */
static void
snapshot(vk_widget_t *widget, vdk_cell_t *cells)
{
    int width = 0, height = 0;
    int y;

    vk_widget_get_metrics(widget, &width, &height);

    for(y = 0; y < height; y++)
        vk_widget_read_row(widget, y, cells + (size_t)y * width, width);
}

/* does the widget's canvas now show exactly what a redraw produces? */
static int
is_current(vk_widget_t *widget)
{
    static vdk_cell_t   before[200 * 60];
    static vdk_cell_t   after[200 * 60];
    int                 width = 0, height = 0;
    size_t              bytes;

    vk_widget_get_metrics(widget, &width, &height);
    if(width > 200 || height > 60) return 0;

    bytes = (size_t)width * height * sizeof(vdk_cell_t);

    snapshot(widget, before);
    vk_widget_redraw(widget);
    snapshot(widget, after);

    return memcmp(before, after, bytes) == 0;
}

/* something was actually painted: not every cell is a blank */
static int
has_content(vk_widget_t *widget)
{
    static vdk_cell_t   cells[200 * 60];
    int                 width = 0, height = 0;
    int                 i;

    vk_widget_get_metrics(widget, &width, &height);
    snapshot(widget, cells);

    for(i = 0; i < width * height; i++)
    {
        if(cells[i].ch != ' ' && cells[i].ch != 0) return 1;
    }

    return 0;
}

/*
    A widget type the library has never heard of, written by the recipe
    in KLASSES.md.  Its redraw writes a mark on its canvas and counts
    how often it has been asked to paint.
*/
typedef struct _probe_s probe_t;

struct _probe_s
{
    vk_widget_t     parent_klass;

    int             (*ctor)     (vk_object_t *, va_list *, ...);
    int             (*dtor)     (vk_object_t *);

    int             redraws;
};

static int  _probe_ctor(vk_object_t *object, va_list *argp, ...);
static int  _probe_dtor(vk_object_t *object);
static int  _probe_redraw(vk_object_t *object);

require_klass(VK_WIDGET_KLASS);

declare_klass(PROBE_KLASS)
{
    .size = KLASS_SIZE(probe_t),
    .name = KLASS_NAME(probe_t),
    .ctor = _probe_ctor,
    .dtor = _probe_dtor,
    .parent = &VK_WIDGET_KLASS,
    .redraw = _probe_redraw,
};

/* construct: the widget first, then what is the probe's own */
static int
_probe_ctor(vk_object_t *object, va_list *argp, ...)
{
    VK_CHAIN_CTOR(object, argp, VK_WIDGET_KLASS);

    ((probe_t *)object)->ctor = _probe_ctor;
    ((probe_t *)object)->dtor = _probe_dtor;

    return 0;
}

/* destroy: nothing of its own to free, so demote and hand on */
static int
_probe_dtor(vk_object_t *object)
{
    if(!vk_object_assert(object, probe_t)) return -1;

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

/* paint: a mark, and count the call */
static int
_probe_redraw(vk_object_t *object)
{
    probe_t *probe = (probe_t *)object;

    probe->redraws++;
    mvwaddstr(VK_WIDGET(probe)->canvas, 0, 0, "probe");

    return 0;
}

/* the probe's public destroy call, whole */
static VK_DEFINE_DESTROY(probe_destroy, probe_t)

/* a new probe, `width` columns by one row */
static probe_t*
probe_create(int width)
{
    return (probe_t *)vk_object_create(PROBE_KLASS, width, 1);
}

/* is `text` somewhere on the widget's canvas? */
static int
shows(vk_widget_t *widget, const char *text)
{
    static vdk_cell_t   cells[200 * 60];
    static char         line[201];
    int                 width = 0, height = 0;
    int                 x, y;

    vk_widget_get_metrics(widget, &width, &height);
    if(width > 200 || height > 60) return 0;

    snapshot(widget, cells);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            uint32_t ch = cells[y * width + x].ch;

            line[x] = (ch >= ' ' && ch < 127) ? (char)ch : ' ';
        }
        line[width] = '\0';

        if(strstr(line, text) != NULL) return 1;
    }

    return 0;
}

/*
    The checks every type gets.  `painted` says whether the type is
    expected to put visible characters on its canvas.
*/
static void
exercise(const char *type, void *object, int new_w, int new_h, int painted)
{
    vk_widget_t *widget = VK_WIDGET(object);

    check(type, "registers no event handlers of its own",
        !has_handlers(object));

    vk_widget_redraw(widget);

    if(painted)
        check(type, "paints something when redrawn", has_content(widget));

    check(type, "resize succeeds",
        vk_widget_resize(widget, new_w, new_h) == 0);
    check(type, "is repainted after a resize, unasked", is_current(widget));
    if(painted)
        check(type, "and still shows its contents", has_content(widget));

    check(type, "recreate succeeds", vk_widget_recreate(widget) == 0);
    check(type, "is repainted after a recreate, unasked",
        is_current(widget));
    if(painted)
        check(type, "and still shows its contents", has_content(widget));

    check(type, "still has no event handlers", !has_handlers(object));
}

/* counts the resize notices an application handler receives */
static int  notices = 0;
static int  notice_was_current = 0;

static int
on_resize(vk_object_t *object, int event, void *anything)
{
    (void)event;
    (void)anything;

    notices++;
    notice_was_current = is_current(VK_WIDGET(object));

    return 0;
}

int
main(void)
{
    vk_screen_t     *screen;
    vk_label_t      *label;
    vk_marquee_t    *marquee;
    vk_button_t     *button;
    vk_input_t      *input;
    vk_textbox_t    *textbox;
    vk_listbox_t    *listbox;
    vk_selectbox_t  *selectbox;
    vk_filler_t     *filler;
    vk_progress_t   *progress;
    vk_calendar_t   *calendar;
    vk_menubar_t    *menubar;
    vk_frame_t      *frame;
    vk_window_t     *window;
    vk_box_t        *box;
    vk_grid_t       *grid;
    vk_label_t      *inner;
    vk_label_t      *a;
    vk_label_t      *b;
    int             w = 0, h = 0;

    setenv("TERM", "xterm-256color", 1);
    setlocale(LC_ALL, "");

    screen = vk_screen_create_detached(120, 40);
    if(screen == NULL)
    {
        printf("FAIL  could not create a detached screen\n");
        return 1;
    }

    vdk_color_init();

    /* ---- types that paint their own contents ---- */

    label = vk_label_create(20);
    vk_label_set_text(label, "a label");
    exercise("label", label, 30, 1, 1);

    button = vk_button_create("Okay");
    vk_widget_get_metrics(VK_WIDGET(button), &w, &h);
    exercise("button", button, w + 4, h, 1);

    input = vk_input_create(20);
    vk_input_set_text(input, "typed");
    vk_widget_get_metrics(VK_WIDGET(input), &w, &h);
    exercise("input", input, w + 8, h, 1);

    textbox = vk_textbox_create(20, 4);
    vk_textbox_set_text(textbox, "some words to wrap across the lines");
    exercise("textbox", textbox, 12, 6, 1);

    listbox = vk_listbox_create(20, 4);
    vk_listbox_add_item(listbox, "one", NULL, NULL);
    vk_listbox_add_item(listbox, "two", NULL, NULL);
    vk_listbox_add_item(listbox, "three", NULL, NULL);
    exercise("listbox", listbox, 26, 6, 1);

    selectbox = vk_selectbox_create(20, 4, VK_SELECTBOX_CHECKBOX);
    vk_selectbox_add_item(selectbox, "first", NULL, NULL);
    vk_selectbox_add_item(selectbox, "second", NULL, NULL);
    exercise("selectbox", selectbox, 26, 5, 1);

    progress = vk_progress_create(VK_PROGRESS_HORIZONTAL, 20, 1);
    vk_progress_set_value(progress, 0.6);
    exercise("progress", progress, 30, 1, 1);

    calendar = vk_calendar_create();
    vk_widget_get_metrics(VK_WIDGET(calendar), &w, &h);
    exercise("calendar", calendar, w + 2, h + 1, 1);

    menubar = vk_menubar_create(30);
    vk_menubar_add_item(menubar, "File", NULL, NULL);
    vk_menubar_add_item(menubar, "Edit", NULL, NULL);
    exercise("menubar", menubar, 40, 1, 1);

    /* a filler paints blanks in its colours: nothing visible to find */
    filler = vk_filler_create();
    vk_widget_set_colors(VK_WIDGET(filler), COLOR_WHITE, COLOR_BLUE);
    exercise("filler", filler, 6, 3, 0);

    /* a marquee moves only when it is run: a repaint for any other
       reason shows it where it is */
    marquee = vk_marquee_create(10);
    vk_marquee_set_text(marquee, "scrolling text that is too long to fit");
    check("marquee", "registers no event handlers of its own",
        !has_handlers(marquee));
    check("marquee", "resize and recreate succeed",
        vk_widget_resize(VK_WIDGET(marquee), 16, 1) == 0
        && vk_widget_recreate(VK_WIDGET(marquee)) == 0);
    check("marquee", "shows its contents after both, unasked",
        has_content(VK_WIDGET(marquee)));
    {
        static vdk_cell_t   was[16];
        static vdk_cell_t   now[16];
        vk_box_t            *holder;
        int                 i;

        vk_marquee_set_direction(marquee, VK_SCROLL_LOOP);
        vk_marquee_set_speed(marquee, 1);
        vk_marquee_run(marquee);
        snapshot(VK_WIDGET(marquee), was);

        for(i = 0; i < 5; i++) vk_widget_redraw(VK_WIDGET(marquee));
        snapshot(VK_WIDGET(marquee), now);
        check("marquee", "a redraw does not move it",
            memcmp(was, now, sizeof(was)) == 0);

        /* nor does redrawing a container it sits in */
        holder = vk_box_create(16, 1, VK_BOX_HORIZONTAL, 1);
        vk_box_set_widget(holder, 0, VK_WIDGET(marquee), VK_INHERIT_NONE);
        for(i = 0; i < 5; i++) vk_box_redraw(holder);
        snapshot(VK_WIDGET(marquee), now);
        check("marquee", "nor does redrawing the box it is in",
            memcmp(was, now, sizeof(was)) == 0);

        vk_marquee_run(marquee);
        snapshot(VK_WIDGET(marquee), now);
        check("marquee", "running it does",
            memcmp(was, now, sizeof(was)) != 0);

        vk_box_destroy(holder);
    }

    /* an activity indicator is the same: it moves only when it is run */
    {
        static vdk_cell_t   was[2];
        static vdk_cell_t   now[2];
        vk_activity_t       *activity;
        vk_box_t            *holder;
        int                 i;

        activity = vk_activity_create();
        check("activity", "registers no event handlers of its own",
            !has_handlers(activity));

        vk_activity_set_style(activity, VK_ACTIVITY_SPINNER);
        vk_activity_start(activity);
        vk_activity_run(activity);
        check("activity", "shows a frame once it is running",
            has_content(VK_WIDGET(activity)));
        snapshot(VK_WIDGET(activity), was);

        for(i = 0; i < 5; i++) vk_activity_redraw(activity);
        snapshot(VK_WIDGET(activity), now);
        check("activity", "a redraw does not move it",
            memcmp(was, now, sizeof(vdk_cell_t)) == 0);
        check("activity", "and a redraw of a current one changes nothing",
            is_current(VK_WIDGET(activity)));

        /* its canvas is thrown away and made again: it repaints the
           frame it was on, unasked */
        vk_widget_recreate(VK_WIDGET(activity));
        snapshot(VK_WIDGET(activity), now);
        check("activity", "shows the same frame after a recreate, unasked",
            memcmp(was, now, sizeof(vdk_cell_t)) == 0);

        /* a change of style resizes it, and it paints the new style's
           first frame without being run */
        vk_activity_set_style(activity, VK_ACTIVITY_BAR);
        check("activity", "paints itself after a change of style",
            has_content(VK_WIDGET(activity)));
        vk_activity_set_style(activity, VK_ACTIVITY_SPINNER);
        vk_activity_run(activity);
        snapshot(VK_WIDGET(activity), was);

        /* nor does redrawing a container it sits in */
        holder = vk_box_create(4, 1, VK_BOX_HORIZONTAL, 1);
        vk_box_set_widget(holder, 0, VK_WIDGET(activity), VK_INHERIT_NONE);
        for(i = 0; i < 5; i++) vk_box_redraw(holder);
        snapshot(VK_WIDGET(activity), now);
        check("activity", "nor does redrawing the box it is in",
            memcmp(was, now, sizeof(vdk_cell_t)) == 0);

        vk_activity_run(activity);
        snapshot(VK_WIDGET(activity), now);
        check("activity", "running it does",
            memcmp(was, now, sizeof(vdk_cell_t)) != 0);

        /* stopped, it is a blank, and running it leaves it one */
        vk_activity_stop(activity);
        vk_activity_run(activity);
        check("activity", "stopped, it shows a blank",
            !has_content(VK_WIDGET(activity)));

        vk_box_destroy(holder);
    }

    /* ---- types that hold others ---- */

    frame = vk_frame_create(20, 5);
    vk_frame_set_border_style(frame, VK_BORDER_SINGLE);
    inner = vk_label_create(10);
    vk_label_set_text(inner, "in a frame");
    vk_widget_set_expand(VK_WIDGET(inner));
    vk_frame_set_child(frame, VK_WIDGET(inner), VK_INHERIT_NONE);
    exercise("frame", frame, 30, 7, 1);
    vk_widget_get_metrics(VK_WIDGET(inner), &w, &h);
    check("frame", "its expanding child was resized with it",
        w == 28 && h == 5);

    window = vk_window_create(24, 6);
    vk_window_set_border_style(window, VK_BORDER_SINGLE);
    vk_window_set_title(window, " title ");
    exercise("window", window, 34, 8, 1);

    box = vk_box_create(20, 2, VK_BOX_VERTICAL, 2);
    a = vk_label_create(20);
    b = vk_label_create(20);
    vk_label_set_text(a, "top slot");
    vk_label_set_text(b, "bottom slot");
    vk_widget_set_expand(VK_WIDGET(a));
    vk_widget_set_expand(VK_WIDGET(b));
    vk_box_set_widget(box, 0, VK_WIDGET(a), VK_INHERIT_NONE);
    vk_box_set_widget(box, 1, VK_WIDGET(b), VK_INHERIT_NONE);

    /* a box's redraw copies its slots in; it does not repaint them.
       the labels are painted here so that there is something to copy */
    vk_label_redraw(a);
    vk_label_redraw(b);
    exercise("box", box, 30, 4, 1);
    vk_widget_get_metrics(VK_WIDGET(a), &w, &h);
    check("box", "its expanding children were laid out again",
        w == 30 && h == 2);
    check("box", "and each repainted itself at its new size",
        is_current(VK_WIDGET(a)) && is_current(VK_WIDGET(b)));

    grid = vk_grid_create(20, 4, 2, 2);
    exercise("grid", grid, 30, 6, 0);

    /* ---- an application's handler is a notice, after the work ---- */

    label = vk_label_create(10);
    vk_label_set_text(label, "notice");
    vk_object_register_event(VK_OBJECT(label), VK_EVENT_ON_RESIZE,
        on_resize, NULL);
    vk_widget_resize(VK_WIDGET(label), 16, 1);
    check("events", "an application's resize handler is called once",
        notices == 1);
    check("events", "and finds the widget already repainted",
        notice_was_current);

    /* ---- a klass reaches its parent's redraw through the klass ---- */

    {
        extern vk_object_t  *VK_WIDGET_KLASS;
        extern vk_object_t  *VK_CONTAINER_KLASS;
        extern vk_object_t  *VK_WINDOW_KLASS;
        extern vk_object_t  *VK_POPUP_KLASS;
        vk_popup_t          *popup;

        popup = vk_popup_create(30, 8, VK_BORDER_SINGLE, "Okay", NULL);
        check("klass", "a popup paints itself (its redraw calls the window's)",
            vk_widget_redraw(VK_WIDGET(popup)) == 0
            && has_content(VK_WIDGET(popup)));
        check("klass", "every klass on the way has a redraw to call",
            VK_WIDGET_KLASS->redraw != NULL
            && VK_CONTAINER_KLASS->redraw != NULL
            && VK_WINDOW_KLASS->redraw != NULL
            && VK_POPUP_KLASS->redraw != NULL);
        check("klass", "one that names none has its parent's",
            VK_POPUP_KLASS->redraw == VK_WINDOW_KLASS->redraw);
        check("klass", "a container has a redraw of its own",
            VK_CONTAINER_KLASS->redraw != VK_WIDGET_KLASS->redraw);
        vk_popup_destroy(popup);
    }

    /* ---- redrawing a container repaints what it holds ---- */
    {
        vk_window_t *top;
        vk_box_t    *outer;
        vk_box_t    *inner_box;
        vk_frame_t  *border;
        vk_grid_t   *cells;
        vk_label_t  *deep;
        vk_label_t  *in_grid;
        vk_listbox_t *list;
        probe_t     *probe;
        vk_popup_t  *popup;
        vk_label_t  *message;
        int         before;

        /* window > box > (label, box > (frame > listbox, probe)) */
        top = vk_window_create(50, 14);
        outer = vk_box_create(48, 12, VK_BOX_VERTICAL, 2);
        inner_box = vk_box_create(48, 8, VK_BOX_HORIZONTAL, 2);
        border = vk_frame_create(20, 6);
        list = vk_listbox_create(18, 4);
        deep = vk_label_create(20);
        probe = probe_create(10);

        vk_listbox_add_item(list, "one", NULL, NULL);
        vk_label_set_text(deep, "before");
        vk_frame_set_child(border, VK_WIDGET(list), VK_INHERIT_NONE);
        vk_box_set_widget(inner_box, 0, VK_WIDGET(border), VK_INHERIT_NONE);
        vk_box_set_widget(inner_box, 1, VK_WIDGET(probe), VK_INHERIT_NONE);
        vk_box_set_widget(outer, 0, VK_WIDGET(deep), VK_INHERIT_NONE);
        vk_box_set_widget(outer, 1, VK_WIDGET(inner_box), VK_INHERIT_NONE);
        vk_window_set_child(top, VK_WIDGET(outer), VK_INHERIT_NONE);

        /* nothing below the window has been painted by hand */
        vk_window_redraw(top);
        check("tree", "one redraw of the window paints the whole tree",
            shows(VK_WIDGET(top), "before") && shows(VK_WIDGET(top), "one")
            && shows(VK_WIDGET(top), "probe"));

        /* change two widgets at different depths; repaint only the top */
        vk_label_set_text(deep, "after");
        vk_listbox_add_item(list, "two", NULL, NULL);
        vk_window_redraw(top);
        check("tree", "a changed child shows after redrawing only the window",
            shows(VK_WIDGET(top), "after") && !shows(VK_WIDGET(top), "before"));
        check("tree", "so does one three containers down",
            shows(VK_WIDGET(top), "two"));

        /* the container never learned what a probe is */
        before = probe->redraws;
        vk_window_redraw(top);
        check("tree", "a type the library does not know is repainted too",
            probe->redraws == before + 1);

        /* a resize reaches the probe through three containers, each of
           which is resized and repainted in turn: the probe is painted
           for its own resize and not once more for every level */
        before = probe->redraws;
        vk_widget_resize(VK_WIDGET(top), 60, 16);
        check("tree", "a resize paints a deep widget once, not once a level",
            probe->redraws - before <= 1);
        check("tree", "and the tree is current afterwards",
            shows(VK_WIDGET(top), "after") && shows(VK_WIDGET(top), "two")
            && is_current(VK_WIDGET(top)));

        /* a redraw part of the way down reaches only what is below it */
        before = probe->redraws;
        vk_frame_redraw(border);
        check("tree", "redrawing a sibling branch leaves the others alone",
            probe->redraws == before);

        /* a redraw of something already current changes nothing */
        check("tree", "redrawing a current tree changes nothing",
            is_current(VK_WIDGET(top)));

        vk_window_destroy(top);

        /* a grid, and a table through it */
        cells = vk_grid_create(40, 6, 2, 2);
        in_grid = vk_label_create(12);
        vk_label_set_text(in_grid, "old cell");
        vk_grid_set_widget(cells, 0, 0, VK_WIDGET(in_grid), VK_INHERIT_NONE);
        vk_grid_redraw(cells);
        vk_label_set_text(in_grid, "new cell");
        vk_grid_redraw(cells);
        check("tree", "a grid repaints what is in its cells",
            shows(VK_WIDGET(cells), "new cell"));
        vk_grid_destroy(cells);

        /* a popup's parts are ordinary children now */
        popup = vk_popup_create(40, 9, VK_BORDER_SINGLE, "Okay", NULL);
        message = vk_label_create(30);
        vk_label_set_text(message, "first message");
        vk_popup_set_client(popup, VK_WIDGET(message));
        vk_popup_redraw(popup);
        vk_label_set_text(message, "second message");
        vk_popup_redraw(popup);
        check("tree", "a popup repaints its client and its buttons",
            shows(VK_WIDGET(popup), "second message")
            && shows(VK_WIDGET(popup), "Okay"));
        vk_popup_destroy(popup);
    }

    /* ---- a resize keeps what was on the canvas, all of it ---- */
    {
        /* a plain widget has no redraw: whatever the program painted
           on it is all there is, so a resize must carry every cell
           over, the last column and the last row included */
        static vdk_cell_t   cells[40 * 10];
        vk_widget_t         *plain;
        WINDOW              *canvas;
        int                 x, y;
        int                 kept = 1;

        plain = vk_widget_create(10, 4);
        canvas = vk_widget_get_canvas(plain);
        for(y = 0; y < 4; y++)
        {
            for(x = 0; x < 10; x++)
                mvwaddch(canvas, y, x, (chtype)('a' + x + y));
        }

        vk_widget_resize(plain, 20, 6);
        snapshot(plain, cells);
        for(y = 0; y < 4; y++)
        {
            for(x = 0; x < 10; x++)
            {
                if(cells[y * 20 + x].ch != (uint32_t)('a' + x + y)) kept = 0;
            }
        }
        check("resize", "growing keeps the last column and the last row",
            kept);
        check("resize", "the last column is still there",
            cells[0 * 20 + 9].ch == (uint32_t)('a' + 9));
        check("resize", "the last row is still there",
            cells[3 * 20 + 0].ch == (uint32_t)('a' + 3));

        /* shrinking keeps what still fits */
        vk_widget_resize(plain, 5, 2);
        snapshot(plain, cells);
        kept = 1;
        for(y = 0; y < 2; y++)
        {
            for(x = 0; x < 5; x++)
            {
                if(cells[y * 5 + x].ch != (uint32_t)('a' + x + y)) kept = 0;
            }
        }
        check("resize", "shrinking keeps every cell that still fits", kept);

        /* a one-cell widget is the smallest there is */
        vk_widget_resize(plain, 1, 1);
        check("resize", "down to one cell and back up",
            vk_widget_resize(plain, 8, 3) == 0);
        snapshot(plain, cells);
        check("resize", "the one cell survived",
            cells[0].ch == (uint32_t)'a');

        vk_widget_destroy(plain);
    }

    /* ---- the two klass macros, on a type the library does not know ---- */
    {
        probe_t     *made;
        vk_label_t  *other;

        made = probe_create(8);
        check("macros", "VK_CHAIN_CTOR built a working widget",
            made != NULL && vk_widget_redraw(VK_WIDGET(made)) == 0
            && made->redraws == 1);
        check("macros", "a constructor refuses a NULL argument list",
            VK_OBJECT(made)->ctor(VK_OBJECT(made), NULL) == -1);

        other = vk_label_create(8);
        check("macros", "VK_DEFINE_DESTROY refuses NULL",
            probe_destroy(NULL) == -1);
        check("macros", "and an object of another type",
            probe_destroy((probe_t *)other) == -1);
        check("macros", "and destroys its own",
            probe_destroy(made) == 0);
        vk_label_destroy(other);
    }

    /* ---- arranging without painting ---- */
    {
        static vdk_cell_t   blank[40 * 10];
        static vdk_cell_t   after[40 * 10];
        vk_box_t            *col;
        vk_box_t            *row;
        vk_grid_t           *cells;
        vk_frame_t          *border;
        vk_label_t          *top;
        vk_label_t          *middle;
        vk_label_t          *bottom;
        vk_label_t          *left;
        vk_label_t          *right;
        vk_label_t          *cell;
        vk_label_t          *framed;
        int                 x = -1, y = -1, w = 0, h = 0;
        int                 x2 = -1, y2 = -1;

        /* a column packed to natural sizes: one row, the rest, one row */
        col = vk_box_create(30, 8, VK_BOX_VERTICAL, 3);
        vk_box_set_homogeneous(col, false);
        top = vk_label_create(30);
        middle = vk_label_create(30);
        bottom = vk_label_create(30);
        vk_label_set_text(top, "top");
        vk_label_set_text(middle, "middle");
        vk_label_set_text(bottom, "bottom");
        vk_widget_set_expand(VK_WIDGET(middle));
        vk_box_set_widget(col, 0, VK_WIDGET(top), VK_INHERIT_AUTO);
        vk_box_set_widget(col, 1, VK_WIDGET(middle), VK_INHERIT_AUTO);
        vk_box_set_widget(col, 2, VK_WIDGET(bottom), VK_INHERIT_AUTO);

        snapshot(VK_WIDGET(col), blank);
        check("layout", "vk_box_layout returns 0", vk_box_layout(col) == 0);

        vk_widget_get_position(VK_WIDGET(middle), &x, &y);
        vk_widget_get_metrics(VK_WIDGET(middle), &w, &h);
        check("layout", "the child that expands has its place and its size",
            x == 0 && y == 1 && w == 30 && h == 6);
        vk_widget_get_position(VK_WIDGET(bottom), &x, &y);
        check("layout", "the child after it is placed below it",
            x == 0 && y == 7);

        snapshot(VK_WIDGET(col), after);
        check("layout", "and nothing was painted on the box",
            memcmp(blank, after, 30 * 8 * sizeof(vdk_cell_t)) == 0);

        /* a redraw afterwards puts nothing anywhere else */
        vk_box_redraw(col);
        vk_widget_get_position(VK_WIDGET(bottom), &x2, &y2);
        check("layout", "a redraw agrees with where the layout put things",
            x2 == x && y2 == y && shows(VK_WIDGET(col), "bottom"));
        vk_box_destroy(col);

        /* a row of equal shares: each child centred in its half */
        row = vk_box_create(40, 3, VK_BOX_HORIZONTAL, 2);
        left = vk_label_create(10);
        right = vk_label_create(10);
        vk_box_set_widget(row, 0, VK_WIDGET(left), VK_INHERIT_AUTO);
        vk_box_set_widget(row, 1, VK_WIDGET(right), VK_INHERIT_AUTO);
        vk_box_layout(row);
        vk_widget_get_position(VK_WIDGET(left), &x, &y);
        vk_widget_get_position(VK_WIDGET(right), &x2, &y2);
        check("layout", "a box of equal shares centres each child in its slot",
            x == 5 && y == 1 && x2 == 25 && y2 == 1);
        vk_box_destroy(row);

        /* a grid: the child of the second column, second row */
        cells = vk_grid_create(40, 6, 2, 2);
        cell = vk_label_create(10);
        vk_grid_set_widget(cells, 1, 1, VK_WIDGET(cell), VK_INHERIT_AUTO);
        vk_widget_get_position(VK_WIDGET(cell), &x, &y);
        check("layout", "before a grid is arranged its child is at the origin",
            x == 0 && y == 0);
        check("layout", "vk_grid_layout returns 0", vk_grid_layout(cells) == 0);
        vk_widget_get_position(VK_WIDGET(cell), &x, &y);
        check("layout", "afterwards it is in its cell",
            x >= 20 && y >= 3);
        vk_grid_redraw(cells);
        vk_widget_get_position(VK_WIDGET(cell), &x2, &y2);
        check("layout", "and a redraw agrees", x2 == x && y2 == y);
        vk_grid_destroy(cells);

        /* a frame fits a child that expands inside its border */
        border = vk_frame_create(20, 6);
        framed = vk_label_create(4);
        vk_frame_set_child(border, VK_WIDGET(framed), VK_INHERIT_AUTO);
        vk_widget_set_expand(VK_WIDGET(framed));
        vk_frame_layout(border);
        vk_widget_get_metrics(VK_WIDGET(framed), &w, &h);
        check("layout", "a frame fits its child inside the border",
            w == 18 && h == 4);
        vk_frame_destroy(border);

        /* nothing to arrange, and nothing there at all */
        top = vk_label_create(10);
        check("layout", "a widget that holds nothing has nothing to do",
            vk_widget_layout(VK_WIDGET(top)) == 0);
        check("layout", "a NULL widget is refused",
            vk_widget_layout(NULL) == -1);
        vk_label_destroy(top);
    }

    vk_screen_destroy(screen);

    printf("%d of %d passed\n", passed, passed + failed);

    return (failed == 0) ? 0 : 1;
}
