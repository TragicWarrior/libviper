/*
    test_destroy -- headless checks of object teardown: the calls that
    destroy an object, and the destroy cascade from a container to what
    it holds.

    What can be observed from outside is VK_EVENT_ON_DESTROY: every
    widget emits it exactly once as it goes.  Each check counts those,
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
    check("the children go before the box's own handler runs",
        before("first second", "box"));

    /* the same through the general calls */
    reset();
    box = make_box(&a, &b);
    vk_object_destroy(VK_OBJECT(box));
    check("vk_object_destroy on a box cascades the same way",
        events == 3 && before("first second", "box"));

    reset();
    box = make_box(&a, &b);
    vk_object_dispose(VK_OBJECT(box));
    check("and so does vk_object_dispose",
        events == 3 && before("first second", "box"));

    /* to any depth: a window holding a box holding labels */
    reset();
    box = make_box(&a, &b);
    window = vk_window_create(24, 6);
    vk_window_set_child(window, VK_WIDGET(box), VK_INHERIT_NONE);
    watch(window, "window");
    vk_window_destroy(window);
    check("a window takes the box and its labels with it",
        events == 4 && before("first second", "box") && before("box", "window"));

    /* a child destroyed first takes itself off its parent's list */
    reset();
    box = make_box(&a, &b);
    vk_label_destroy(a);
    check("a child can be destroyed first", events == 1);
    vk_box_destroy(box);
    check("and its parent then destroys only what is left",
        events == 3 && before("first second", "box"));

    /* a child taken out of its container outlives it */
    reset();
    box = make_box(&a, &b);
    vk_box_set_widget(box, 0, NULL, VK_INHERIT_NONE);
    vk_box_destroy(box);
    check("a child taken out first is not destroyed with the box",
        events == 2 && strcmp(order, "second box ") == 0);
    vk_label_destroy(a);
    check("and is still there to be destroyed afterwards",
        events == 3 && strcmp(order, "second box first ") == 0);

    reset();
    vk_screen_destroy(screen);

    printf("%d of %d passed\n", passed, passed + failed);

    return (failed == 0) ? 0 : 1;
}
