/*
    test_destroy -- headless checks of object teardown: the calls that
    destroy an object, and the destroy cascade from a container to what
    it holds.

    What can be observed from outside is VK_EVENT_ON_DESTROY: every
    widget emits it exactly once, before anything of it is taken apart
    -- so a container is announced before the widgets it holds.  Each check counts those,
    and notes the order they came in.  A double free shows up as the
    program being killed, which fails the test.

    Exit status 0 when every check passes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vdk.h"

static int  passed = 0;
static int  failed = 0;

/* the names of the widgets that announced their destruction, in order */
static char order[256];
static int  events = 0;

/* record one check */
static void
check(const char *what, int ok)
{
    if(ok) passed++;
    else failed++;

    printf("%s  %-58s [%s]\n", ok ? "PASS" : "FAIL", what, order);
}

/* forget what was seen */
static void
reset(void)
{
    order[0] = '\0';
    events = 0;
}

/* the destroy handler every test widget gets: `anything` is its name */
static int
on_destroy(vk_object_t *object, int event, void *anything)
{
    (void)object;
    (void)event;

    events++;
    strncat(order, (const char *)anything, sizeof(order) - strlen(order) - 2);
    strncat(order, " ", sizeof(order) - strlen(order) - 1);

    return 0;
}

/*
    Was everything in `first` (names separated by spaces) announced
    before `then`?  The order among the names in `first` is not looked
    at: which of two siblings goes first is not something the library
    promises.
*/
static int
before(const char *first, const char *then)
{
    char        names[64];
    char        *name;
    const char  *at_then = strstr(order, then);
    const char  *at_name;

    if(at_then == NULL) return 0;

    snprintf(names, sizeof(names), "%s", first);

    for(name = strtok(names, " "); name != NULL; name = strtok(NULL, " "))
    {
        at_name = strstr(order, name);
        if(at_name == NULL || at_name > at_then) return 0;
    }

    return 1;
}

/* what the handlers below got back from the destroy calls they made */
static int  inner_typed = 99;
static int  inner_any = 99;
static int  inner_object = 99;
static int  inner_parent = 99;
static int  inner_dispose = 99;
static int  inner_calls = 0;

/*
    A destroy handler that does what it must not: destroys the object it
    was called for, three different ways.  Each call has to be refused,
    and the object must still be there for the next one.
*/
static int
on_destroy_destroy_self(vk_object_t *object, int event, void *anything)
{
    (void)event;
    (void)anything;

    inner_calls++;
    inner_typed = vk_label_destroy(VK_LABEL(object));
    inner_any = vk_widget_destroy(VK_WIDGET(object));
    inner_object = vk_object_destroy(object);
    inner_dispose = vk_object_dispose(object);

    return 0;
}

/*
    A destroy handler on a child that destroys the box the child is in
    (`anything`).  Allowed: the box is another object.  The box goes,
    takes the child's sibling with it, and leaves the child -- which is
    in the middle of being destroyed by someone else -- alone.
*/
static int
on_destroy_destroy_parent(vk_object_t *object, int event, void *anything)
{
    (void)object;
    (void)event;

    inner_parent = vk_box_destroy((vk_box_t *)anything);

    return 0;
}

/*
    A destroy handler on a scroll bar that destroys the widget the bar
    is attached to (`anything`).  Allowed, like the box above.
*/
static int
on_destroy_destroy_host(vk_object_t *object, int event, void *anything)
{
    (void)object;
    (void)event;

    inner_parent = vk_widget_destroy((vk_widget_t *)anything);

    return 0;
}

/* watch a widget's destruction under `name` */
static void
watch(void *widget, const char *name)
{
    vk_object_register_event(VK_OBJECT(widget), VK_EVENT_ON_DESTROY,
        on_destroy, (void *)name);
}

/* a box holding two labels, all three watched */
static vk_box_t*
make_box(vk_label_t **a, vk_label_t **b)
{
    vk_box_t    *box = vk_box_create(20, 2, VK_BOX_VERTICAL, 2);

    *a = vk_label_create(10);
    *b = vk_label_create(10);

    vk_box_set_widget(box, 0, VK_WIDGET(*a), VK_INHERIT_NONE);
    vk_box_set_widget(box, 1, VK_WIDGET(*b), VK_INHERIT_NONE);

    watch(box, "box");
    watch(*a, "first");
    watch(*b, "second");

    return box;
}

int
main(void)
{
    vk_screen_t     *screen;
    vk_label_t      *label;
    vk_label_t      *a;
    vk_label_t      *b;
    vk_box_t        *box;
    vk_window_t     *window;

    setenv("TERM", "xterm-256color", 1);

    screen = vk_screen_create_detached(80, 24);
    if(screen == NULL)
    {
        printf("FAIL  could not create a detached screen\n");
        return 1;
    }

    vdk_color_init();

    /* the call this test was written for: before 10.1.1 it ran the
       label's destructor and then freed the label a second time */
    reset();
    label = vk_label_create(10);
    watch(label, "label");
    check("vk_object_destroy on a widget returns 0",
        vk_object_destroy(VK_OBJECT(label)) == 0);
    check("and the widget was told once", events == 1);

    reset();
    label = vk_label_create(10);
    watch(label, "label");
    vk_object_dispose(VK_OBJECT(label));
    check("vk_object_dispose on a widget tells it once", events == 1);

    reset();
    label = vk_label_create(10);
    watch(label, "label");
    vk_label_destroy(label);
    check("the typed destroy tells it once", events == 1);

    check("vk_object_destroy refuses NULL",
        vk_object_destroy(NULL) == -1);

    /* a plain container says whether it was destroyed */
    reset();
    {
        vk_container_t  *container = vk_container_create(10, 2);

        watch(container, "container");
        check("vk_container_destroy returns 0 when it destroys",
            vk_container_destroy(container) == 0 && events == 1);
    }

    reset();
    box = make_box(&a, &b);
    check("and -1 for NULL, or for something that is not exactly a container",
        vk_container_destroy(NULL) == -1
        && vk_container_destroy(VK_CONTAINER(box)) == -1 && events == 0);
    vk_box_destroy(box);

    /* a container takes what it holds along */
    reset();
    box = make_box(&a, &b);
    vk_box_destroy(box);
    check("destroying a box destroys both its labels", events == 3);
    check("the box is announced before anything it holds is taken apart",
        before("box", "first") && before("box", "second"));

    /* the same through the general calls */
    reset();
    box = make_box(&a, &b);
    vk_object_destroy(VK_OBJECT(box));
    check("vk_object_destroy on a box cascades the same way",
        events == 3 && before("box", "first") && before("box", "second"));

    reset();
    box = make_box(&a, &b);
    vk_object_dispose(VK_OBJECT(box));
    check("and so does vk_object_dispose",
        events == 3 && before("box", "first") && before("box", "second"));

    /* to any depth: a window holding a box holding labels */
    reset();
    box = make_box(&a, &b);
    window = vk_window_create(24, 6);
    vk_window_set_child(window, VK_WIDGET(box), VK_INHERIT_NONE);
    watch(window, "window");
    vk_window_destroy(window);
    check("a window takes the box and its labels with it",
        events == 4 && before("window", "box") && before("box", "first")
        && before("box", "second"));

    /* a child destroyed first takes itself off its parent's list */
    reset();
    box = make_box(&a, &b);
    vk_label_destroy(a);
    check("a child can be destroyed first", events == 1);
    vk_box_destroy(box);
    check("and its parent then destroys only what is left",
        events == 3 && before("first", "box") && before("box", "second"));

    /* a child taken out of its container outlives it */
    reset();
    box = make_box(&a, &b);
    vk_box_set_widget(box, 0, NULL, VK_INHERIT_NONE);
    vk_box_destroy(box);
    check("a child taken out first is not destroyed with the box",
        events == 2 && strcmp(order, "box second ") == 0);
    vk_label_destroy(a);
    check("and is still there to be destroyed afterwards",
        events == 3 && strcmp(order, "box second first ") == 0);

    /* what the destroy calls return: 0 for destroyed, -1 for refused */
    reset();
    check("every destroy call refuses NULL with -1",
        vk_widget_destroy(NULL) == -1 && vk_container_destroy(NULL) == -1
        && vk_label_destroy(NULL) == -1 && vk_marquee_destroy(NULL) == -1
        && vk_textbox_destroy(NULL) == -1 && vk_button_destroy(NULL) == -1
        && vk_filler_destroy(NULL) == -1 && vk_input_destroy(NULL) == -1
        && vk_activity_destroy(NULL) == -1 && vk_progress_destroy(NULL) == -1
        && vk_meter_destroy(NULL) == -1 && vk_graph_destroy(NULL) == -1
        && vk_histogram_destroy(NULL) == -1 && vk_scroller_destroy(NULL) == -1
        && vk_listbox_destroy(NULL) == -1 && vk_selectbox_destroy(NULL) == -1
        && vk_dropdown_destroy(NULL) == -1 && vk_spinbutton_destroy(NULL) == -1
        && vk_calendar_destroy(NULL) == -1 && vk_menubar_destroy(NULL) == -1
        && vk_viewport_destroy(NULL) == -1 && vk_deck_destroy(NULL) == -1
        && vk_frame_destroy(NULL) == -1 && vk_window_destroy(NULL) == -1
        && vk_popup_destroy(NULL) == -1 && vk_box_destroy(NULL) == -1
        && vk_filedialog_destroy(NULL) == -1 && vk_grid_destroy(NULL) == -1
        && vk_table_destroy(NULL) == -1 && vk_color_destroy(NULL) == -1
        && vk_screen_destroy(NULL) == -1);

    /* the exact type is destroyed */
    reset();
    label = vk_label_create(10);
    watch(label, "label");
    check("the typed call returns 0 for an object of its own type",
        vk_label_destroy(label) == 0 && events == 1);

    /* a derived object through its parent's call: refused, and alive */
    reset();
    {
        vk_marquee_t    *marquee = vk_marquee_create(10);

        watch(marquee, "marquee");
        check("a marquee is refused by vk_label_destroy, with -1",
            vk_label_destroy((vk_label_t *)marquee) == -1 && events == 0);
        check("and is still there for its own call, which returns 0",
            vk_marquee_destroy(marquee) == 0 && events == 1);
    }

    reset();
    window = vk_window_create(24, 6);
    watch(window, "window");
    check("a window is refused by vk_frame_destroy and vk_container_destroy",
        vk_frame_destroy((vk_frame_t *)window) == -1
        && vk_container_destroy((vk_container_t *)window) == -1
        && events == 0);
    check("and destroyed by vk_window_destroy",
        vk_window_destroy(window) == 0 && events == 1);

    /* an unrelated object */
    reset();
    {
        vk_button_t     *button = vk_button_create("ok");

        watch(button, "button");
        check("a button is refused by vk_label_destroy and vk_box_destroy",
            vk_label_destroy((vk_label_t *)button) == -1
            && vk_box_destroy((vk_box_t *)button) == -1 && events == 0);
        check("vk_widget_destroy takes any widget and returns 0",
            vk_widget_destroy(VK_WIDGET(button)) == 0 && events == 1);
    }

    /* a container's result is its own, whatever it held */
    reset();
    box = make_box(&a, &b);
    check("vk_box_destroy returns 0 and takes its children",
        vk_box_destroy(box) == 0 && events == 3);

    /* ---- a destroy asked for from inside the object's own destroy
            handler is refused, and the object is destroyed once ---- */

    reset();
    label = vk_label_create(10);
    vk_object_register_event(VK_OBJECT(label), VK_EVENT_ON_DESTROY,
        on_destroy_destroy_self, NULL);
    watch(label, "label");
    check("destroying a widget whose handler destroys it too returns 0",
        vk_label_destroy(label) == 0);
    check("the handler ran once", inner_calls == 1);
    check("its typed destroy of the same widget was refused",
        inner_typed == -1);
    check("so was its vk_widget_destroy", inner_any == -1);
    check("and its vk_object_destroy", inner_object == -1);
    check("and its vk_object_dispose", inner_dispose == -1);
    check("the handlers after it still ran, once", events == 1);

    /* the same through a container: the child's handler is the culprit */
    reset();
    inner_calls = 0;
    box = make_box(&a, &b);
    vk_object_register_event(VK_OBJECT(a), VK_EVENT_ON_DESTROY,
        on_destroy_destroy_self, NULL);
    check("a box whose child's handler destroys the child is destroyed",
        vk_box_destroy(box) == 0 && events == 3 && inner_calls == 1);

    /* a child's handler may destroy the box the child is in */
    reset();
    box = make_box(&a, &b);
    vk_object_register_event(VK_OBJECT(a), VK_EVENT_ON_DESTROY,
        on_destroy_destroy_parent, box);
    check("destroying a child whose handler destroys its box returns 0",
        vk_label_destroy(a) == 0);
    check("the box was destroyed by the handler", inner_parent == 0);
    check("the box and both children were each announced once",
        events == 3 && strstr(order, "box") != NULL
        && strstr(order, "first") != NULL
        && strstr(order, "second") != NULL);

    /* an attached scroll bar whose handler destroys its host */
    {
        vk_listbox_t    *list = vk_listbox_create(20, 5);
        vk_scroller_t   *bar = vk_scroller_create(VK_SCROLLBAR_VERTICAL);

        reset();
        inner_parent = 99;
        vk_widget_attach_scroller(VK_WIDGET(list), bar);
        watch(list, "list");
        watch(bar, "bar");
        vk_object_register_event(VK_OBJECT(bar), VK_EVENT_ON_DESTROY,
            on_destroy_destroy_host, list);
        check("destroying a scroll bar whose handler destroys its host returns 0",
            vk_scroller_destroy(bar) == 0);
        check("the host was destroyed by the handler, each announced once",
            inner_parent == 0 && events == 2);
    }

    reset();
    check("vk_screen_destroy returns 0", vk_screen_destroy(screen) == 0);

    printf("%d of %d passed\n", passed, passed + failed);

    return (failed == 0) ? 0 : 1;
}
