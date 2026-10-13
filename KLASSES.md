# VK Klass Reference

## Hierarchy

```
vk_object_t
│
├─ vk_screen_t
│
└─ vk_widget_t
   │
   ├─ vk_scroller_t
   │
   ├─ vk_container_t
   │  │
   │  ├─ vk_box_t
   │  │  │
   │  │  └─ vk_filedialog_t
   │  │
   │  ├─ vk_grid_t
   │  │  │
   │  │  └─ vk_table_t
   │  │     │
   │  │     └─ vk_color_t
   │  │
   │  └─ vk_frame_t
   │     │
   │     └─ vk_window_t
   │        │
   │        └─ vk_popup_t
   │
   ├─ vk_label_t
   │  │
   │  └─ vk_marquee_t
   │
   ├─ vk_deck_t
   │
   ├─ vk_textbox_t
   │
   ├─ vk_button_t
   │
   ├─ vk_input_t
   │
   ├─ vk_spinbutton_t
   │
   ├─ vk_filler_t
   │
   ├─ vk_activity_t
   │
   ├─ vk_progress_t
   │  │
   │  └─ vk_meter_t
   │
   ├─ vk_graph_t
   │  │
   │  └─ vk_histogram_t
   │
   ├─ vk_menubar_t
   │
   ├─ vk_calendar_t
   │
   ├─ vk_viewport_t
   │
   └─ vk_listbox_t
      │
      ├─ vk_selectbox_t
      │
      └─ vk_dropdown_t
```

## Klass Framework Overview

The VK klass system is a hand-rolled object-oriented type system in C. It
provides single inheritance, virtual dispatch via function pointers, and a
rudimentary RTTI mechanism.

## Struct Embedding (Inheritance)

Each derived klass embeds its parent as the first struct member (`parent_klass`).
A pointer to any derived type can be safely cast to any of its ancestors.
Cast macros are defined in `vdk.h`:

| Macro | Target Type |
|-------|-------------|
| `VK_OBJECT(x)` | `vk_object_t *` |
| `VK_SCREEN(x)` | `vk_screen_t *` |
| `VK_WIDGET(x)` | `vk_widget_t *` |
| `VK_CONTAINER(x)` | `vk_container_t *` |
| `VK_FRAME(x)` | `vk_frame_t *` |
| `VK_SCROLLER(x)` | `vk_scroller_t *` |
| `VK_BOX(x)` | `vk_box_t *` |
| `VK_GRID(x)` | `vk_grid_t *` |
| `VK_TABLE(x)` | `vk_table_t *` |
| `VK_COLOR(x)` | `vk_color_t *` |
| `VK_VIEWPORT(x)` | `vk_viewport_t *` |
| `VK_LABEL(x)` | `vk_label_t *` |
| `VK_MARQUEE(x)` | `vk_marquee_t *` |
| `VK_LISTBOX(x)` | `vk_listbox_t *` |
| `VK_WINDOW(x)` | `vk_window_t *` |
| `VK_SELECTBOX(x)` | `vk_selectbox_t *` |
| `VK_DROPDOWN(x)` | `vk_dropdown_t *` |
| `VK_TEXTBOX(x)` | `vk_textbox_t *` |
| `VK_DECK(x)` | `vk_deck_t *` |
| `VK_BUTTON(x)` | `vk_button_t *` |
| `VK_INPUT(x)` | `vk_input_t *` |
| `VK_SPINBUTTON(x)` | `vk_spinbutton_t *` |
| `VK_FILLER(x)` | `vk_filler_t *` |
| `VK_MENUBAR(x)` | `vk_menubar_t *` |
| `VK_FILEDIALOG(x)` | `vk_filedialog_t *` |
| `VK_CALENDAR(x)` | `vk_calendar_t *` |
| `VK_POPUP(x)` | `vk_popup_t *` |
| `VK_PROGRESS(x)` | `vk_progress_t *` |
| `VK_METER(x)` | `vk_meter_t *` |
| `VK_GRAPH(x)` | `vk_graph_t *` |
| `VK_HISTOGRAM(x)` | `vk_histogram_t *` |

## Klass Templates

Each type declares a global singleton "klass descriptor" using:

```c
declare_klass(KLASS_NAME)
{
    .size   = KLASS_SIZE(type),
    .name   = KLASS_NAME(type),
    .ctor   = ...,
    .dtor   = ...,
    .parent = &PARENT_KLASS,      // the type this one is built on
    .redraw = ...,                // how it paints itself; optional
};
require_klass(KLASS_NAME);    // extern reference from other files
```

These are: `VK_OBJECT_KLASS`, `VK_SCREEN_KLASS`, `VK_WIDGET_KLASS`, `VK_CONTAINER_KLASS`,
`VK_FRAME_KLASS`, `VK_SCROLLER_KLASS`, `VK_WINDOW_KLASS`, `VK_BOX_KLASS`,
`VK_GRID_KLASS`, `VK_TABLE_KLASS`, `VK_COLOR_KLASS`,
`VK_LABEL_KLASS`, `VK_MARQUEE_KLASS`, `VK_LISTBOX_KLASS`,
`VK_SELECTBOX_KLASS`, `VK_DROPDOWN_KLASS`, `VK_TEXTBOX_KLASS`, `VK_DECK_KLASS`,
`VK_BUTTON_KLASS`, `VK_INPUT_KLASS`, `VK_FILLER_KLASS`,
`VK_MENUBAR_KLASS`, `VK_FILEDIALOG_KLASS`, `VK_CALENDAR_KLASS`,
`VK_POPUP_KLASS`, `VK_VIEWPORT_KLASS`, `VK_PROGRESS_KLASS`,
`VK_METER_KLASS`, `VK_GRAPH_KLASS`, `VK_HISTOGRAM_KLASS`.
The template carries the type's size, name, constructor and destructor,
the klass it is built on (`.parent`, since 11.0.0) and how it paints
itself (`.redraw`, since 11.0.0). It serves as both the type descriptor
and the vtable seed.

A type that names no `.redraw` takes its parent's: the first time an
object of the type is constructed, the library follows `.parent` up the
chain and fills the empty slot in. A type that paints exactly like the
one it is built on (`vk_meter_t` on `vk_progress_t`) therefore names
nothing.

## Allocation and Construction

`vk_object_construct(klass, ...)` does three things:

1. `calloc(1, klass->size)` -- allocates the full derived struct
2. `memcpy` the `vk_object_t` header from the klass template
3. Calls `klass->ctor(object, &argp)` if a ctor is installed

Each derived type provides a convenience wrapper:

```c
vk_screen_create(void)
vk_widget_create(width, height)
vk_container_create(width, height)
vk_frame_create(width, height)
vk_scroller_create(flags)
vk_box_create(width, height, orientation, slots)
vk_grid_create(width, height, cols, rows)
vk_table_create(width, height, cols, rows, divider_style)
vk_color_create(width, height, cols, rows, divider_style)
vk_viewport_create(width, height)
vk_label_create(width)
vk_marquee_create(width)
vk_listbox_create(width, height)
vk_window_create(width, height)
vk_selectbox_create(width, height, mode)
vk_dropdown_create(width, max_visible)
vk_textbox_create(width, height)
vk_deck_create(void)
vk_button_create(text)
vk_input_create(width)
vk_spinbutton_create(width)
vk_filler_create(void)
vk_menubar_create(width)
vk_calendar_create(void)
vk_filedialog_create(width, height, style, multiselect)
vk_popup_create(width, height, style, ...)
```

## Constructor Chaining

Each derived ctor explicitly calls its parent's ctor through the parent's
klass singleton before installing its own methods:

```
_vk_screen_ctor     -> vk_object_construct (no parent ctor call)
_vk_container_ctor  -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_frame_ctor      -> VK_CONTAINER_KLASS->ctor(object, argp)
_vk_scroller_ctor   -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_box_ctor        -> VK_CONTAINER_KLASS->ctor(object, argp)
_vk_grid_ctor       -> VK_CONTAINER_KLASS->ctor(object, argp)
_vk_table_ctor      -> VK_GRID_KLASS->ctor(object, argp)
_vk_color_ctor      -> VK_TABLE_KLASS->ctor(object, argp)
_vk_viewport_ctor   -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_label_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_marquee_ctor    -> VK_LABEL_KLASS->ctor(object, argp)
_vk_window_ctor     -> VK_FRAME_KLASS->ctor(object, argp)
_vk_listbox_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_selectbox_ctor  -> VK_LISTBOX_KLASS->ctor(object, argp)
_vk_dropdown_ctor   -> VK_LISTBOX_KLASS->ctor(object, argp)
_vk_textbox_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_deck_ctor       -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_button_ctor     -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_input_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_spinbutton_ctor -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_filler_ctor     -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_menubar_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_calendar_ctor   -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_filedialog_ctor -> VK_BOX_KLASS->ctor(object, argp)
_vk_popup_ctor      -> VK_WINDOW_KLASS->ctor(object, argp)
_vk_progress_ctor   -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_meter_ctor      -> VK_PROGRESS_KLASS->ctor(object, argp)
_vk_graph_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_histogram_ctor  -> VK_GRAPH_KLASS->ctor(object, argp)
```

Each does it with one line, `VK_CHAIN_CTOR(object, argp, PARENT_KLASS)`
(since 12.3.0): refuse a NULL object or argument list, then run the
parent's constructor. A constructor is always handed its argument list,
by `vk_object_construct()` or by the constructor of a type built on it.
The parent takes its own arguments off the front; what is left belongs
to this type and is read in order with `va_arg(*argp, type)`.

(Before 12.3.0 every constructor also allowed for being called directly
with the arguments themselves, and carried a `va_start` / `va_end` block
for it. Nothing called one that way, and the block ended an argument
list it had not started. A constructor called with a NULL list now
returns -1.)

After the parent constructs, the derived ctor overwrites the function pointer
slots in its own struct region with its implementations.

## Destruction

Destroying a widget has two sides, and they run in different directions.

**Up the type chain** -- one object is taken apart layer by layer, from
its own type down to `vk_object_t`. This is the demote pattern, below.

**Down the widget tree** -- a widget that hosts others destroys them when
it is destroyed, and each of those does the same for what it hosts. This
is the destroy cascade, further below. It is an ordinary step of the
container's destructor.

Before either, the object is **announced**: `VK_EVENT_ON_DESTROY` is
emitted on it, once, while it is still whole. The library registers no
handler for that event and nothing in the teardown depends on one; it is
there for the application.

### What a Destroy Call Returns (since 10.2.0)

Every `vk_<type>_destroy()` returns **0** when it destroyed the object
and **-1** when it did not.

The typed calls are strict. Each destroys only an object that is
*exactly* its type and refuses any other, a derived one included:
`vk_frame_destroy()` will not destroy a window, `vk_label_destroy()`
will not destroy a marquee. A refusal changes nothing -- the object is
still alive and still the caller's to destroy -- and -1 is the only sign
of it. NULL is refused the same way.

Before 10.2.0 these calls returned nothing, so a refusal was silent and
the object simply leaked.

When all you hold is a widget or object pointer, `vk_widget_destroy()`
and `vk_object_dispose()` destroy an object as whatever it really is.
`vk_widget_destroy()` returns -1 only for NULL.

### Destructor Chaining (Demote Pattern)

A derived dtor:

1. Cleans up its own resources
2. Calls `vk_object_demote(object, parent_type)` to rewrite `.name` and `.size`
3. Delegates to the parent's destroy function

This continues until `vk_object_destroy()` calls `free()`.

#### Example: vk_window_t

```
vk_window_destroy
  -> emit VK_EVENT_ON_DESTROY          (once; the window is still whole)
  -> _vk_window_dtor
    -> free title
    -> vk_object_demote(object, vk_frame_t)
    -> vk_frame_destroy
      -> _vk_frame_dtor
        -> vk_object_demote(object, vk_container_t)
        -> vk_container_destroy
          -> _vk_container_dtor
            -> destroy each child        (the cascade, below)
            -> vk_object_demote(object, vk_widget_t)
            -> vk_widget_destroy
              -> _vk_widget_dtor
                -> take itself off its parent's list
                -> destroy attached scrollers
                -> delete the canvas
                -> vk_object_demote(object, vk_object_t)
                -> vk_object_destroy
                  -> free the handler list
                  -> free(object)
```

Every destroy call on the way down (`vk_frame_destroy`,
`vk_container_destroy`, ...) would announce the object too; the object
remembers that it has been announced, so the event is emitted by the
first of them and by no other.

#### Example: vk_scroller_t

```
_vk_scroller_dtor
  -> detach from host (clear host->vscroller or host->hscroller)
  -> vk_object_demote(object, vk_widget_t)
  -> vk_widget_destroy
    -> _vk_widget_dtor
      -> vk_object_demote(object, vk_object_t)
      -> vk_object_destroy
        -> free(object)
```

Every class built on `vk_container_t` (`vk_box_t`, `vk_frame_t`,
`vk_window_t`, `vk_popup_t`, `vk_grid_t`, `vk_table_t`, `vk_color_t`,
`vk_filedialog_t`) passes through the container's destructor on the way
down. Leaf classes (`vk_label_t`, `vk_button_t`, ...) demote directly to
`vk_widget_t`.

### The Destroy Cascade (since 10.0.0)

**A container owns what it holds.** Destroying a window destroys its
child; destroying a box or a grid destroys what is in its slots; and so
on down the tree, to any depth.

The mechanism is the container's destructor (since 11.0.0):

1. `_vk_container_dtor` walks the container's child list, takes each
   child off it, and destroys the child through the destructor of the
   child's own type (`vk_object_dispose`).
2. A child that is itself a container does the same for its own as it
   goes.

Every class built on `vk_container_t` passes through that destructor, so
every one of them takes part without doing anything.

(10.x carried the cascade on `VK_EVENT_ON_DESTROY`: the container
registered a handler for its own event. The teardown no longer depends
on an event.)

### The Destroy Event

`VK_EVENT_ON_DESTROY` is a notice for the application, for example to
free what a widget's user pointer refers to. Since 11.0.0:

- It is emitted **once** per object, by whichever destroy call is made
  on it, **before** anything of the object is taken down. A handler sees
  the widget whole and may use anything of it, type-specific calls
  included.
- A container is announced **before** its children: its handler runs
  while they are all still alive. Each child is announced in turn as the
  container's destructor reaches it.
- The library registers no handler of its own for it.
- A handler that destroys the object it was called for is refused
  (since 12.2.1): every destroy call returns -1 for an object whose
  destroy handlers are running, and the destroy that is running them
  finishes the job when the handler returns. Before 12.2.1 this freed
  the object twice. A handler may destroy *other* objects, the
  container its object sits in included.

(In 10.x the event arrived from the widget layer of the teardown, with
the object's own type already gone and a container's children already
destroyed.)

What follows from the cascade:

- **Destroy the outermost widget and nothing else.** A dialog is torn
  down with one `vk_window_destroy()`.
- **Do not destroy a child after its container.** It is already gone;
  that is a double free. (Before 10.0.0 containers only unlinked their
  children, and callers had to destroy each one by hand.)
- **Destroying a child first is still safe.** A widget takes itself off
  its parent's list as it is destroyed.
- **To keep a child, take it out first:** `vk_box_set_widget(box, slot,
  NULL, ...)`, `vk_frame_set_child(frame, NULL, ...)`,
  `vk_popup_set_client(popup, NULL)`, `vk_container_remove_widget()`, or
  `vk_container_vacate()` for all of them.
- **Attached scrollers go with their widget.** Detach one first
  (`vk_widget_detach_scroller`) to keep it.
- **A deck and a screen do not own.** `vk_deck_destroy()` lets go of the
  windows on it, and a widget attached to a screen surface is only shown
  there; both are destroyed by whoever created them.

`vk_widget_destroy()` accepts a widget of any derived type and destroys
it as what it is. `vk_object_dispose()` does the same for any object.
The typed calls (`vk_button_destroy()` and the rest) still refuse an
object that is not exactly their type.

## Writing a New Klass

Everything a new widget type has to do, using a hypothetical `vk_gauge_t`
built on `vk_widget_t`. Follow the steps and resize, teleport and
destruction work without anything further.

**1. The struct** embeds the parent first, then its own slots and data:

```c
struct _vk_gauge_s
{
    vk_widget_t     parent_klass;

    int             (*ctor)     (vk_object_t *, va_list *, ...);
    int             (*dtor)     (vk_object_t *);

    int             value;
};
```

**2. The descriptor** names the parent and the redraw:

```c
require_klass(VK_WIDGET_KLASS);

declare_klass(VK_GAUGE_KLASS)
{
    .size   = KLASS_SIZE(vk_gauge_t),
    .name   = KLASS_NAME(vk_gauge_t),
    .ctor   = _vk_gauge_ctor,
    .dtor   = _vk_gauge_dtor,
    .parent = &VK_WIDGET_KLASS,
    .redraw = _vk_gauge_redraw,
};
```

**3. The constructor** calls the parent's first, then sets up its own:

```c
static int
_vk_gauge_ctor(vk_object_t *object, va_list *argp, ...)
{
    VK_CHAIN_CTOR(object, argp, VK_WIDGET_KLASS);

    VK_GAUGE(object)->ctor = _vk_gauge_ctor;
    VK_GAUGE(object)->dtor = _vk_gauge_dtor;

    return 0;
}
```

`VK_CHAIN_CTOR` goes first. A type that takes arguments of its own reads
them after it, in the order `vk_gauge_create()` passes them:
`VK_GAUGE(object)->max = va_arg(*argp, int);`

**4. The destructor** frees its own, demotes, and hands on to the parent:

```c
static int
_vk_gauge_dtor(vk_object_t *object)
{
    if(!vk_object_assert(object, vk_gauge_t)) return -1;

    // free what vk_gauge_t allocated, if anything

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}
```

**5. The redraw** paints the canvas from the widget's state. If the type
is built on one that paints, call that first (see Redraw and Blit):

```c
static int
_vk_gauge_redraw(vk_object_t *object)
{
    vk_gauge_t  *gauge = VK_GAUGE(object);

    // paint VK_WIDGET(gauge)->canvas from gauge->value

    return 0;
}
```

**6. The public calls**: create, destroy, redraw, and the type's own.

```c
vk_gauge_t*
vk_gauge_create(int width)
{
    return (vk_gauge_t *)vk_object_create(VK_GAUGE_KLASS, width, 1);
}

VK_DEFINE_DESTROY(vk_gauge_destroy, vk_gauge_t)

int
vk_gauge_redraw(vk_gauge_t *gauge)
{
    return vk_widget_redraw(VK_WIDGET(gauge));
}
```

`VK_DEFINE_DESTROY` is the whole of the destroy call: it refuses NULL
and any object that is not exactly a `vk_gauge_t`, announces the
destruction, and runs the destructor, returning 0 or -1.

That is all. What the type does **not** do:

- register an event handler for anything
- repaint after a resize or a teleport -- the base calls its redraw
- look after attached scroll bars
- destroy its children, if it is built on a container -- the container's
  destructor does
- repaint its children, if it is built on a container -- calling the
  parent's redraw first does
- hand its colors to its children, if it is built on a container -- the
  same call does

Two optional extras, for a type that holds other widgets or wraps text:
set `VK_WIDGET(object)->_layout` in the constructor to arrange the
contents after a resize, and override `_recreate` to pass a teleport on
to widgets the type holds outside a container's child list.

## RTTI (vk_object_assert)

`vk_object_assert(object, type)` compares the object's runtime `.name` string
and `.size` against a compile-time type. Returns 1 if the object IS that exact
type, 0 otherwise.

Assert is used **only in destructors** (`_dtor` / `_destroy`) as a safety
guard before freeing resources. Public API functions do not assert — callers
who cast via `VK_FRAME()`, `VK_LISTBOX()`, etc. accept responsibility for
type correctness. This means derived types can be used through their parent's
public API (e.g. a `vk_window_t*` can be passed to
`vk_frame_set_border_colors()` via `VK_FRAME(window)`). Convenience macros
in `vdk.h` make this transparent — `vk_window_set_border_colors()` expands
to the frame call with the cast built in.

## Virtual Methods

Each klass level defines function pointer slots in its struct for polymorphic
dispatch. Public APIs call through these pointers.

| Klass | Slots |
|-------|-------|
| `vk_object_t` | `ctor`, `dtor`, `kmio`, `redraw` |
| `vk_screen_t` | `ctor`, `dtor` |
| `vk_widget_t` | `ctor`, `dtor`, `_blit`, `_move`, `_resize`, `_recreate`, `_erase`, `_layout` |
| `vk_container_t` | `ctor`, `dtor`, `add_widget`, `remove_widget`, `vacate`, `rotate` |
| `vk_box_t` | `ctor`, `dtor` |
| `vk_frame_t` | `ctor`, `dtor`, `_set_border_style`, `_set_child`, `_draw_border` |
| `vk_scroller_t` | `ctor`, `dtor`, `_draw_scrollbar` |
| `vk_window_t` | `ctor`, `dtor`, `_draw_title` |
| `vk_listbox_t` | `ctor`, `dtor`, `_add_item`, `_set_item`, `_remove_item`, `_get_item`, `_get_item_count`, `_get_selected`, `_exec_item`, `_add_separator`, `_reset` |
| `vk_selectbox_t` | `ctor`, `dtor` |
| `vk_dropdown_t` | `ctor`, `dtor` |
| `vk_label_t` | `ctor`, `dtor` |
| `vk_marquee_t` | `ctor`, `dtor` |
| `vk_textbox_t` | `ctor`, `dtor` |
| `vk_deck_t` | `ctor`, `dtor` (overrides widget's `_blit`, `_erase`, `_resize`, `_recreate`) |
| `vk_button_t` | `ctor`, `dtor` |
| `vk_input_t` | `ctor`, `dtor` |
| `vk_spinbutton_t` | `ctor`, `dtor` |
| `vk_filler_t` | `ctor`, `dtor` |
| `vk_menubar_t` | `ctor`, `dtor`, `_add_item`, `_get_item_count`, `_exec_item`, `_reset` |
| `vk_calendar_t` | `ctor`, `dtor` |
| `vk_filedialog_t` | `ctor`, `dtor` |
| `vk_popup_t` | `ctor`, `dtor` |
| `vk_progress_t` | `ctor`, `dtor` |
| `vk_meter_t` | `ctor`, `dtor`, `_fill_color` (overrides progress's `_fill_color`) |
| `vk_graph_t` | `ctor`, `dtor`, `_bar_count`, `_bar_value` |
| `vk_histogram_t` | `ctor`, `dtor`, `_bar_count`, `_bar_value` (overrides graph's `_bar_*` virtuals) |

### Redraw and Blit (since 11.0.0)

Showing a widget is two separate operations.

**Redraw** paints the widget's canvas from the widget's state: the text
of a label, the items of a list, the border and title of a window. It is
one slot, `redraw`, in `vk_object_t`, filled from the klass descriptor,
so every widget has one and the base can call it without knowing the
type. `vk_widget_redraw(widget)` calls it; so does every
`vk_<type>_redraw()`.

**Blit** copies the finished canvas to where the widget sits on the
surface it is attached to. It paints nothing. It is the widget slot
`_blit`, reached through `vk_widget_blit(widget)`.

The slots were `_update` (one per type, in each type's own struct) and
`_draw` before 11.0.0, and the public calls `vk_<type>_update()` and
`vk_widget_draw()`. 11.x kept those names as macros for the new ones;
12.0.0 removed them.

**Which types paint what.** A type either supplies a `redraw` or takes
the one of the type it is built on. A type that supplies one decides for
itself whether the parent's painting is part of it:

| Type | `redraw` |
|------|----------|
| `vk_widget_t` | does nothing |
| `vk_container_t` | repaints each widget it holds (since 11.1.0); paints nothing of its own |
| `vk_frame_t`, `vk_box_t`, `vk_grid_t` | the container's, then its own canvas with the children copied in |
| `vk_meter_t` | the progress bar's |
| `vk_marquee_t`, `vk_selectbox_t`, `vk_dropdown_t` | its own, complete; the parent's is not called |
| `vk_window_t` | the container's, then its own (border, title, child); the frame's is not called |
| `vk_table_t` | the grid's, then its dividers |
| `vk_color_t` | the table's, then its swatches |
| `vk_histogram_t` | bins its samples, then the graph's |
| `vk_popup_t` | the window's |
| `vk_filedialog_t` | the box's |
| every other type | its own |

**Calling the parent's.** A type that wants what its parent paints calls
it first, then paints its own on top, so that its own is not painted
over:

```c
static int
_my_window_redraw(vk_object_t *object)
{
    // everything a window paints: border, title, scroll bars, child
    VK_WINDOW_KLASS->redraw(object);

    // then what is mine, on top
    ...

    return 0;
}
```

It is called through the parent's klass, not through
`vk_window_redraw()`: the public call goes through the object's own
slot, which is this function. Calling the parent's is the author's
choice; only constructors and destructors must chain.

**Containers repaint what they hold (since 11.1.0).** Redrawing a
container redraws everything in it, to any depth, and then the container
itself. Change any number of widgets anywhere in a window and call
`vk_window_redraw()` once.

It is done in one place. `vk_container_t`'s redraw walks the container's
child list and calls `vk_widget_redraw()` on each child, which repaints
it as whatever it is; a container knows nothing about the types it
holds, and a new type needs no mention anywhere. Frame, window, box and
grid call that first (`VK_CONTAINER_KLASS->redraw(object)`), then paint
their own canvas and copy the children in. A type built on one of them
gets it by calling its parent's redraw, or by naming none.

What this means for a caller:

- Redrawing a widget repaints that widget and what is inside it. It does
  not reach upward: after redrawing a label alone, its box and window
  still show the old one until they are redrawn.
- Redrawing the outermost widget is always enough. Redrawing a child
  first and then its container, as was needed before 11.1.0, still
  works; the child is simply painted twice.
- A redraw of a window costs a repaint of the whole tree in it. To
  repaint one corner of a large tree often, redraw that part and blit
  it.

The repaint that follows a resize or a recreate does not walk down
again: each child has just been through the same and repainted itself,
so the container only copies them in. A deep widget is painted once per
resize, not once per level.

(Before 11.1.0 a container only copied its children's canvases in, and a
changed child had to be redrawn by hand before its container.)

## Public API Convention

All public API functions in the `.c` files are declared `inline` as a compiler
hint to reduce call overhead. These are thin wrappers that validate arguments
and dispatch through the virtual method pointers.

Where a derived klass inherits API from its parent (e.g. `vk_window_t`
inheriting border and child APIs from `vk_frame_t`), convenience macros in
`vdk.h` provide the derived-type name. These sit inline in the header
alongside real function declarations so the API reads as a flat list.

## Headers

`vdk.h` is the single public header for VDK. It declares all VDK types,
constants, cast macros, callback typedefs, function prototypes, and the
color system API.

It also says which release it belongs to (since 12.0.0):
`VDK_VERSION_MAJOR`, `VDK_VERSION_MINOR`, `VDK_VERSION_PATCH`, the text
`VDK_VERSION`, and `VDK_VERSION_AT_LEAST(major, minor, patch)` for a
program that needs a release or newer:

```c
#ifndef VDK_VERSION_MAJOR
#error "this program needs libviper 12.0.0 or newer"
#elif !VDK_VERSION_AT_LEAST(12, 2, 0)
#error "this program needs libviper 12.2.0 or newer"
#endif
```

The `#ifndef` goes first and on its own line. A header from before
12.0.0 defines none of these names, and the preprocessor reads the whole
of an `#if`, so `!defined(...) || !VDK_VERSION_AT_LEAST(...)` is an error
in itself there.

The numbers are checked against the build's version when libviper is
configured, so the header cannot drift from the library. VDK source files include `vdk.h` through their private
headers and have no dependency on `viper.h`. Non-public VDK sources live
in the `vdk/` subdirectory.

`viper.h` is the legacy libviper header. It includes `vdk.h` for backwards
compatibility, so existing code that includes `viper.h` continues to work.

## Color System

VDK uses ncurses color pairs. The color API is declared in `vdk.h`:

- `vdk_color_init()` — optional convenience that calls `start_color()` and
  registers color pairs via `init_pair()`. Must be called after
  `vk_screen_create()`. Because ncurses pair 0 cannot be modified with
  `init_pair()`, `vdk_color_init()` swaps white-on-black into index 0 so
  it maps to the ncurses default pair. Pairs are registered in three tiers:
  - **Basic (0–63):** 8×8 matrix for the standard 8 colors (0–7)
  - **ANSI 16×16 (64–319):** exact pairs for colors 0–15 (skipping the
    basic pairs already covered above). Provides accurate mapping for
    AIX bright colors (8–15) without the halved-fg approximation.
  - **Extended (320+):** 256×256 with halved foreground (`fg >> 1`) to
    fit within `SHRT_MAX`. Only registered when `COLORS >= 256`.
- `vdk_color_pair(fg, bg)` — maps an (fg, bg) pair to an ncurses pair
  index using fast arithmetic. No globals. Uses a tiered lookup: basic
  (fg ≤ 7, bg ≤ 7), ANSI 16×16 (fg ≤ 15, bg ≤ 15), then extended.
- `VDK_COLORS(fg, bg)` — convenience macro: `COLOR_PAIR(vdk_color_pair(fg, bg))`.

Widget internals use `COLOR_PAIR(vdk_color_pair(fg, bg))` directly (the
ncurses way) rather than the `VDK_COLORS` macro.

If the caller does not call `vdk_color_init()`, they are responsible for
calling `start_color()` and `init_pair()` themselves.

## Reading a Canvas Back (since 10.1.0)

Everything drawn ends up in an ncurses `WINDOW`: a widget's canvas, or
the canvas of a surface where widgets are composed. A cell of one is
private to the process -- its character may be a line-drawing key, its
attributes are ncurses bits, its colours are a pair number. To show a
canvas anywhere else (an image, a copy in another program) those have to
be worked out, and one function does it:

- `vdk_cells_read_row(win, y, cells, cols)` -- decode row `y` of any
  `WINDOW` into `vdk_cell_t`. Returns `cols`, or -1 for no such row.
- `vk_widget_read_row(widget, y, cells, cols)` -- the same for a widget.
  It reads the **composer**, which is what is on show: the canvas, or a
  frozen widget's snapshot.
- `vk_screen_read_row(screen, y, cells, cols)` -- the same for the
  active surface's canvas, i.e. the screen as composed.

A `vdk_cell_t` is 16 bytes with a fixed layout:

| Field | Meaning |
|-------|---------|
| `ch` | Unicode code point (line-drawing keys already translated); 0 in the second half of a double-width character |
| `fg`, `bg` | `0x00RRGGBB` (`VDK_CELL_R/G/B` unpack them) |
| `attrs` | `VDK_CELL_BOLD`, `VDK_CELL_UNDERLINE`, `VDK_CELL_ITALIC`, `VDK_CELL_BLINK` |
| `flags` | `VDK_CELL_FG_DEFAULT`, `VDK_CELL_BG_DEFAULT`, `VDK_CELL_WIDE`, `VDK_CELL_WIDE_TAIL` |

Things a reader should know:

- **Reverse and dim are not attributes.** A terminal cannot be counted on
  for either, so they are folded into the colours: reverse swaps `fg` and
  `bg`, dim moves `fg` halfway to `bg`.
- **A default colour still has an RGB value** (light gray on black), plus
  a flag, so a reader that cannot do better has something to draw.
- **Only the first character of a cell is kept.** A combining mark after
  it is dropped.
- **Colours rely on `vdk_color_init()`.** ncurses does not know what the
  standard 256-colour palette looks like; asked about a colour nobody
  has redefined, it returns a placeholder. So the palette is built in,
  `vdk_color_init()` records ncurses' placeholders as its last step, and
  ncurses is believed only for a colour whose value has since changed
  (a program redefined it).

## Screens and Surfaces

`vk_screen_t` manages the ncurses terminal (SCREEN) and provides virtual
surfaces. It derives directly from `vk_object_t`, not `vk_widget_t`.

`vk_screen_create()` calls `newterm()` on stdin/stdout, initializes
keypad, raw mode, and creates one default surface. Color initialization
is the caller's responsibility (see Color System below). Each surface holds its own
full-screen canvas (WINDOW) and an array of attached widgets.

| Function | Purpose |
|----------|---------|
| `vk_screen_add_surface` | Add a new virtual surface, returns its id |
| `vk_screen_del_surface` | Remove a surface by id (minimum one must remain) |
| `vk_screen_set_surface` | Set the active surface by id; emits `VK_EVENT_ON_SURFACE_CHANGE` |
| `vk_screen_get_active_surface` | Return the active surface index |
| `vk_screen_get_surface_count` | Return the total number of surfaces |
| `vk_screen_get_window` | Return the active surface's canvas |
| `vk_screen_attach_widget` | Attach a widget to a surface |
| `vk_screen_detach_widget` | Detach a widget from a surface |
| `vk_screen_resize` | Handle terminal resize (updates all surface canvases) |
| `vk_screen_poll_resize` | Poll actual terminal size via ioctl; calls resize if changed |
| `vk_screen_teleport` | Migrate the entire UI to a different PTY |
| `vk_screen_adopt` | The same, also naming the terminal type; NULL path rebuilds in place |
| `vk_screen_detach` | Leave the terminal and keep running on none |
| `vk_screen_create_detached` | Create a screen that starts on no terminal |
| `vk_screen_set_wallpaper` | Register a wallpaper callback (`VkSurfaceBkgdFunc`) |
| `vk_screen_set_overlay` | Register an overlay callback (same signature as wallpaper) |
| `vk_screen_set_surface_bkgd` | Persist a `wbkgdset` value on a surface canvas; reapplied automatically after teleport |
| `vk_screen_apply_stdscr_bkgd` | Push the active surface's bkgd onto `stdscr`; invoked automatically on active-surface change, live color change, and teleport |
| `vk_screen_paint_wallpaper` | Manually invoke the wallpaper callback on the active surface |
| `vk_screen_refresh` | Composite the active surface: erase, wallpaper, widget blit, overlay, refresh |
| `vk_screen_destroy` | Tear down screen, restore evicted terminal, free resources |

### Wallpaper

A wallpaper callback can be registered via `vk_screen_set_wallpaper()`.
The callback signature is:

```c
void callback(vk_screen_t *screen, int surface_id, WINDOW *canvas);
```

The callback receives the surface id so it can paint different backgrounds
per surface. `vk_screen_refresh()` calls the wallpaper automatically as
part of its composite sequence:

1. Erase the active surface canvas
2. Fire the wallpaper callback (if set) — paints on the surface canvas
3. Blit all attached widgets on top (opaque, fully covering wallpaper)
4. Overwrite to stdscr
5. Fire the overlay callback (if set) — paints on stdscr after compositing
6. Refresh stdscr

This means the caller only needs to update widget state and call
`vk_screen_refresh()` — no manual erase, widget draw, or wallpaper
calls required. The wallpaper shows through wherever widgets don't cover.

`vk_screen_paint_wallpaper()` is also available for callers who need to
invoke the wallpaper outside the normal refresh cycle.

**Caveat:** wallpaper callbacks should paint the surface canvas manually
with `wattron`/`mvwaddch` (or `setcchar`/`mvwadd_wch`). Avoid `wbkgd()`
on the surface canvas — it sets the window's background color pair, which
causes ncurses pair 0 to inherit that color instead of the terminal
default (white-on-black). This breaks any rendering that relies on pair 0,
such as deck drop shadows.

### Surface bkgd

Each surface stores a `chtype bkgd` value via
`vk_screen_set_surface_bkgd(screen, surface_id, bkgd)`. The value is
**applied to `stdscr`**, not to the surface canvas:

- `vk_screen_apply_stdscr_bkgd(screen)` pushes the active surface's
  stored bkgd onto `stdscr`. This is invoked automatically from
  `vk_screen_set_surface` (active surface change), from
  `vk_screen_set_surface_bkgd` when the changed surface is the active
  one (live color change), and from `vk_screen_teleport` after the
  new SCREEN is established (the new `stdscr` would otherwise come up
  with the ncurses default bkgd).

The `stdscr` propagation matters because `stdscr` is the only `WINDOW`
ncurses converts into the byte stream the outer terminal renders.
When `wrefresh(stdscr)` decides to emit hardware-scroll /
insert-delete-line optimizations for a heavy redraw, any cell the outer
terminal exposes during those operations takes `stdscr`'s bkgd value.
Without this, those exposed cells flash as the terminal's default
background (typically black) before the next composite paints over
them. With it, the exposed cells come up in the desktop's color.

Why not the surface canvas?  The surface canvas is fully overpainted
every `vk_screen_refresh` (`werase` -> wallpaper_func paints every
cell -> widgets composite -> `overwrite` to `stdscr`), so its bkgd is
never visible.  Worse, applying a non-default bkgd there interacts
badly with the deck shadow code (`_vk_deck_draw_shadow`), which writes
pair-0 cells -- `vdk_color_init` maps white-on-black to pair 0 for
ncurses default-pair compatibility, and ncurses fills pair-0 cells
with the destination window's bkgd's pair on write.  A non-zero bkgd
on the surface canvas would tint shadows with the surface color.
`stdscr` doesn't have this problem: cells reach `stdscr` only via
`overwrite` from the (fully-painted) canvas, which preserves cell pair
values verbatim and doesn't go through the pair-0 substitution.

Consumers should set bkgd attribute bits to zero (e.g. just
`' ' | COLOR_PAIR(N)`), since ncurses ORs the bkgd's attribute bits
into every cell written to the WINDOW. A non-zero attribute (`A_BOLD`,
`A_REVERSE`, etc.) in bkgd will bleed into all written content.

### Overlay

An overlay callback can be registered via `vk_screen_set_overlay()`. It
uses the same `VkSurfaceBkgdFunc` signature as the wallpaper callback,
but fires after widget compositing and the `overwrite()` to stdscr. The
canvas parameter is stdscr itself, so the overlay paints on top of the
final composited image. This is useful for elements that must appear above
all widgets, such as a software cursor on terminals without hardware cursor
support (e.g. GPM on the Linux console).

### Teleport

`vk_screen_teleport(screen, pty_path)` migrates the UI to a different
terminal.

Nothing running on the target terminal is moved out of the way. The
caller sees to it that whatever was reading that terminal (a shell) is
waiting on something else, typically a small foreground program that
asked for the move and stays until the screen leaves again. (Before
9.0.0 the library found the terminal's session leader and suspended
it.) The sequence is:

1. **Open the target** -- for writing and for reading, close-on-exec.
2. **Save termios** -- snapshot the target PTY's terminal attributes
   before ncurses changes them.
3. **Create new SCREEN** -- call `newterm()`, initialize
   colors/keypad/raw mode.
4. **Recreate widgets** -- rebuild all surface canvases and widget trees
   on the new screen via `vk_widget_recreate()`.
5. **Tear down old terminal** -- `endwin()` the old SCREEN, restore the
   termios saved when the screen arrived there, close old file handles.
6. **Drain input** -- flush stale terminal response bytes from the
   `newterm`/`keypad` initialization sequences.
7. **Emit `VK_EVENT_ON_TELEPORT`** -- fires all handlers registered on
   the screen object (e.g. to reinitialize colors via `vdk_color_init()`).

Old ncurses WINDOWs are intentionally leaked during teleport because
`delwin` on windows bound to a different SCREEN corrupts ncurses internal
state. The old SCREEN is shut down with `endwin()` but not `delscreen()`
for the same reason. This is bounded: teleport is a rare operation and
each invocation leaks only the previous set of canvases.

On destroy, the screen performs the same handoff: the terminal's
termios are restored.

`vk_screen_detach(screen)` is the same move with no target: the screen
goes on drawing to the null device at the size it had.
`vk_screen_create_detached(width, height)` starts a screen that way, for
a program that runs in the background and is attached later.

### Post-Teleport Resize

After teleport, our process is not in the target terminal's session, so
SIGWINCH from that terminal goes to the stopped shell, not us.
`vk_screen_poll_resize()` works around this by calling
`ioctl(TIOCGWINSZ)` on the screen's `fd_out` to read the actual terminal
dimensions, comparing against stored values, and calling
`vk_screen_resize()` if they differ. Callers should invoke it each tick
of their event loop (the demo does this alongside `KEY_RESIZE` checks).

## KMIO (Keyboard/Mouse I/O)

VDK widgets are IO-agnostic by default. The `kmio` function pointer on
`vk_object_t` is a slot the application can fill via
`vk_object_set_kmio(object, func)`. Keystrokes are dispatched through it
via `vk_object_push_keystroke()`, which calls `object->kmio(object, keystroke)`
if the slot is non-NULL.

### Default Container Forwarding

Container klasses install default `kmio` handlers in their constructors
that forward keystrokes to children:

- **`vk_frame_t`** — forwards to `frame->child` (inherited by `vk_window_t`)
- **`vk_box_t`** — forwards to `slot_widgets[focused_slot]`
- **`vk_deck_t`** — forwards to the topmost widget

These defaults enable keystroke propagation through the widget tree without
any application code. The application can override any default handler with
`vk_object_set_kmio()` — the function pointer is simply overwritten.

### Box Focus Cycling

The box default kmio forwards all keystrokes to the child in
`slot_widgets[focused_slot]`. It does not intercept any keys for focus
cycling — that policy is left to the application. The scaffolding:

- `box->focused_slot` — public int, initialized to 0. Controls which
  slot receives forwarded keystrokes.
- The application installs a custom kmio via `vk_object_set_kmio()` that
  intercepts a key of its choosing (e.g. TAB, arrow keys) to advance or
  retreat `focused_slot`, and forwards everything else to the child via
  `vk_object_push_keystroke()`.

### Composite Widget KMIO

`vk_filedialog_t` installs its own `kmio` handler at creation time because
its internal navigation (switching between path input and file list,
directory traversal) requires coordinated keyboard handling across multiple
child widgets. See File Dialogs.

### Application Handlers

The application installs custom handlers for leaf widgets and any container
where the default forwarding is insufficient. Typical patterns include:

- **Navigation** — a listbox handler that moves `curr_item` on arrow keys,
  skips separators, and calls `_exec_item` on Enter.
- **Scrolling** — a textbox handler that adjusts `scroll_top` on arrow/page
  keys and redraws.
- **Toggle** — a selectbox handler that calls `vk_selectbox_toggle_item()`
  on Enter/Space and delegates navigation to the listbox handler.
- **Focus rotation** — a box handler that uses `KEY_TAB` to cycle
  `focused_slot`, overriding the default forwarding.
- **Stack cycling** — a deck handler that calls `vk_deck_cycle()` on
  `KEY_TAB` and forwards other keys to the topmost widget.

## Event System

`vk_object_t` carries a linked list of event handlers. Any object (widget
or screen) can register handlers for named events and emit events to notify
all registered listeners.

| API | Description |
|-----|-------------|
| `vk_object_register_event(object, event, func, data)` | Add a handler for an event type |
| `vk_object_unregister_event(object, event, func)` | Remove a handler by event + function pointer |
| `vk_object_emit(object, event)` | Fire all handlers registered for the event |

The handler signature is `int (*VkEventFunc)(vk_object_t *object, int event, void *data)`.
The `data` pointer is the value passed at registration time, enabling per-handler
context (e.g. a struct with pointers to related widgets).

### Event Types

Events are grouped by purpose with spaced numeric ranges:

| Event | Value | Emitted by |
|-------|-------|------------|
| `VK_EVENT_ON_RESIZE` | 1 | `vk_widget_resize()`, last: the widget is resized, laid out and repainted |
| `VK_EVENT_ON_RECREATE` | 2 | `vk_widget_recreate()`, last: the widget has its new canvas and is repainted |
| `VK_EVENT_ON_TELEPORT` | 3 | `vk_screen_teleport()` after terminal migration completes |
| `VK_EVENT_ON_DESTROY` | 4 | the destroy call made on an object, first and once, before anything is taken down |
| `VK_EVENT_ON_CLICK` | 10 | `vk_button_press()` |
| `VK_EVENT_ON_SELECT` | 11 | listbox/selectbox/menubar navigation and check/radio selection |
| `VK_EVENT_ON_UNSELECT` | 12 | selectbox uncheck |
| `VK_EVENT_ON_ACTIVATE` | 13 | listbox/selectbox/menubar `exec_curr` |
| `VK_EVENT_ON_SUBMIT` | 14 | reserved for input submission |
| `VK_EVENT_ON_CHANGE` | 16 | `vk_spinbutton` value change (arrow step or committed edit) |
| `VK_EVENT_ON_FOCUS` | 20 | `vk_box_set_subfocus()` on the newly focused slot widget |
| `VK_EVENT_ON_UNFOCUS` | 21 | `vk_box_set_subfocus()` on the previously focused slot widget |
| `VK_EVENT_ON_SCROLL` | 22 | textbox scroll functions |
| `VK_EVENT_ON_SURFACE_CHANGE` | 30 | `vk_screen_set_surface()` when the active surface changes |

### Events Are for the Application (since 11.0.0)

The library registers no handler on its own objects. Nothing it does
depends on one being present, on the order handlers were registered in,
or on a handler doing something. An event is a notice, emitted after the
library's own work is finished (or, for destroy, before it starts), and
an object with no handlers at all behaves completely.

So an application handler for `VK_EVENT_ON_RESIZE` finds the widget
already at its new size, laid out and repainted, and has only its own
business to do. A new widget type registers nothing to take part in
resize, recreate or destroy.

(Before 11.0.0 several types registered handlers for their own resize,
recreate and destroy events in their constructors, and the library's
behaviour was carried by them.)

### Cleanup

Event handlers are freed automatically during `vk_object_destroy()`.

## Frames and Scrollers

`vk_frame_t` is a bordered single-child container. It supports three border
styles:

| Constant | Style |
|----------|-------|
| `VK_FRAME_NONE` | No border |
| `VK_FRAME_ASCII` | Drawn with `+`, `-`, `\|` |
| `VK_FRAME_SINGLE` | ncurses ACS single-line box |
| `VK_FRAME_DOUBLE` | Unicode double-line box (U+2550 et al.) |

Any style except `VK_FRAME_NONE` can be combined with `VK_FRAME_REVERSE`
to draw the border in reverse video:

```c
vk_frame_set_border_style(frame, VK_FRAME_SINGLE | VK_FRAME_REVERSE);
```

The flag is a modifier (bit 4), not a standalone style. Frames, windows,
and scrollers all honour it.

The border occupies one cell on each side, so the child is sized to
`(width-2, height-2)`. Border colors default to -1 (inherit from widget
fg/bg) but can be overridden independently. Minimum size is 3x3.

`vk_scroller_t` derives directly from `vk_widget_t` (not from `vk_frame_t`).
It is not a container and does not own children. Instead, it is an attachment
widget that draws a scrollbar strip on a host widget's canvas.

Each scroller handles one axis. Create with `vk_scroller_create(flags)` where
`flags` is `VK_SCROLLBAR_VERTICAL` or `VK_SCROLLBAR_HORIZONTAL`. For both
axes, create two scrollers and attach both.

Attachment is via `vk_widget_attach_scroller(host, scroller)`. This sets:
- `host->vscroller` or `host->hscroller` (stored on `vk_widget_t`)
- `scroller->surface = host->canvas` (blit target)
- Scroller canvas sized to a 1-column or 1-row strip matching the host

Bordered hosts (frame, window) draw attached scrollers in their redraw after
the border and before children. Borderless hosts (listbox) draw scrollers
after their own content. On resize the scrollers are refitted to the host,
and on recreate (teleport) they are given fresh canvases; both are done by
`vk_widget_resize()` and `vk_widget_recreate()` for every widget, so no host
type arranges for it.

When there is nothing to scroll (content fits the viewport), the scroller
skips drawing entirely so the host's border is preserved underneath.

On a bordered host (frame/window), the scrollbar replaces the right border
column (vertical) or bottom border row (horizontal). On a borderless host
(e.g. listbox), the scrollbar hugs the edge and the host renders content
1 column/row narrower.

Scroll state is obtained through a `VkScrollInfoFunc` callback. The caller
sets both the callback and a scroll source widget via
`vk_scroller_set_scroll_source()`. The scroller queries the source during
its redraw for content dimensions and scroll offsets. The scroller does not
drive scrolling -- the source widget manages its own scroll position.

Both `vk_window_t` and `vk_frame_t` include scroller drawing in their
redraw.

## Windows

`vk_window_t` inherits from `vk_frame_t` and adds a title drawn on the top
border and a user decoration callback. It supplies its own redraw and
overrides `_recreate`. All frame APIs (border style, border colors,
child management, redraw) are available under `vk_window_*` names via
convenience macros.

The title is rendered on border row 0 using the border colors and
`border_attrs` (set via `vk_window_set_border_attrs()`).
Justification is controlled by:

| Constant | Alignment |
|----------|-----------|
| `VK_JUSTIFY_LEFT` | Title starts at column 2 |
| `VK_JUSTIFY_CENTER` | Title centered on border |
| `VK_JUSTIFY_RIGHT` | Title ends at column width-2 |

If the border style is `VK_FRAME_NONE`, no title is drawn.

A `VkWindowDecorateFunc` callback can be registered via
`vk_window_set_decorate()`. It fires during the window's redraw after the border and
title are drawn but before the child widget is drawn. The callback receives
the window's canvas, so the caller can paint on the border (which persists)
or inside the frame (which the child will overwrite).

## Boxes

`vk_box_t` is a layout container that arranges child widgets in a row
(horizontal) or column (vertical). It derives from `vk_container_t`.

| Constant | Orientation |
|----------|-------------|
| `VK_BOX_HORIZONTAL` | Children arranged left-to-right in columns |
| `VK_BOX_VERTICAL` | Children arranged top-to-bottom in rows |

The number of slots is fixed at creation (default 1). Each slot holds one
widget, placed via `vk_box_set_widget(box, slot, widget)`.

The box ctor extracts `orientation` and `slots` from the va_arg chain after
the parent (container -> widget) consumes `width` and `height`. This is the
first klass to consume extra construction parameters beyond the base pair.

### Homogeneous Mode (Default)

By default, boxes are **homogeneous**: slots are evenly sized based on the
box dimensions (the last slot absorbs any remainder). Each child is centered
within its slot on both axes. Children with `VK_STATE_EXPAND` are resized
to fill their slot; non-expand children keep their natural dimensions and
are centered.

### Non-Homogeneous Mode

`vk_box_set_homogeneous(box, false)` switches to non-homogeneous layout.
Each slot is sized to its child's natural dimension (width for horizontal,
height for vertical). Children with `VK_STATE_EXPAND` absorb leftover
space evenly, with the last expand child taking any remainder. Hidden
children (without `VK_STATE_VISIBLE`) are skipped entirely during both
sizing and drawing passes, so their space is reclaimed by expand children.

The box's redraw erases, then for each slot: positions, resizes (if
expand), and draws the child widget. The box does not propagate resize to
grandchildren (e.g. a frame's child won't be resized if the box resizes
the frame).

## Decks

`vk_deck_t` is a z-order manager derived from `vk_widget_t`. It maintains a
linked list of child widgets in stacking order and composites them bottom-to-top
during the screen refresh cycle.

Unlike other widget types, the deck has **no canvas of its own**. The ctor
frees the canvas allocated by the widget base class and sets it to NULL.
During `_blit`, the deck iterates its children in reverse (bottom-to-top)
and draws each one directly onto the deck's surface (the screen surface
canvas). Children use the standard destructive `copywin` so overlapping
windows correctly occlude what's beneath them.

Because the deck has no canvas, `_erase` and `_resize` are no-ops, and
`vk_deck_create()` takes no size parameters.

| Constant | Position |
|----------|----------|
| `VK_DECK_TOP` | Add widget above all others (default) |
| `VK_DECK_BOTTOM` | Add widget below all others |

| Function | Purpose |
|----------|---------|
| `vk_deck_create()` | Create an empty deck |
| `vk_deck_add_widget(deck, widget, position)` | Insert widget at top or bottom of stack |
| `vk_deck_remove_widget(deck, widget)` | Remove widget from stack |
| `vk_deck_set_top(deck, widget)` | Move a widget already in the deck to the top |
| `vk_deck_get_top(deck)` | Return the topmost widget (or NULL) |
| `vk_deck_cycle(deck, vector)` | Rotate the stack (`VK_VECTOR_LEFT` / `VK_VECTOR_RIGHT`) |
| `vk_deck_count(deck)` | Number of widgets in the stack (0 if NULL or empty) |
| `vk_deck_get_widget(deck, index)` | Widget at the given index (0 == top); NULL on bounds violation |
| `vk_deck_set_shadow(deck, enabled)` | Enable or disable drop shadows on all children |
| `vk_deck_set_shadow_colors(deck, fg, bg)` | Set shadow color pair (default white-on-black) |
| `vk_deck_redraw(deck)` | Manually composite children (delegates to `_blit`) |
| `vk_deck_destroy(deck)` | Let go of all windows (they are not destroyed) and destroy the deck |

The deck's `_blit` sets each child's `surface` pointer to `deck->surface`
before drawing. This means children do not need a valid surface at add time —
the binding happens lazily during the screen refresh cycle. The deck can be
attached and detached from surfaces at runtime, acting as a floating overlay.

### Shadows

When shadows are enabled via `vk_deck_set_shadow(deck, TRUE)`, each child
widget casts a drop shadow — an L-shaped strip (right edge + bottom edge,
offset by 1 cell). The shadow reads the existing characters from the surface
and recolors them, preserving the glyph beneath. Shadow colors default to
white-on-black and can be changed via `vk_deck_set_shadow_colors(deck, fg, bg)`.
Shadows are drawn before each widget during bottom-to-top compositing, so
higher widgets correctly occlude the shadows of lower ones.

`_recreate` (teleport) propagates `vk_widget_recreate()` to all children so
they rebuild their canvases on the new SCREEN. The deck itself has nothing
to recreate.

## Resize Propagation

`vk_widget_resize()` does the whole job, in this order (since 11.0.0):

1. `_resize` -- the canvas gets its new size.
2. Scroll bars attached to the widget are refitted to it.
3. `_layout`, if the type has one -- the type arranges what it holds for
   the new size.
4. `redraw` -- the widget repaints itself.
5. `VK_EVENT_ON_RESIZE` is emitted, for the application.

A request for the size the widget already has does nothing and emits
nothing.

`_layout` is a slot in `vk_widget_t` that most types leave empty. The
types that fill it:

- **vk_frame_t** (and window, popup) -- resizes the child to
  `(width - 2, height - 2)`
- **vk_box_t** (and file dialog) -- recalculates slot dimensions and
  resizes each child
- **vk_grid_t** (and table, colour picker) -- recalculates the cells
- **vk_textbox_t** -- reflows its text to the new width

Propagation is recursive: a box's `_layout` resizes each slot's widget
with `vk_widget_resize()`, which runs the same five steps for it (a
frame inside the box resizes its own child, and so on). A container is
repainted after its children, so what it composites is current.

### Arranging Without Painting (since 12.2.0)

`_layout` puts each child in its place as well as sizing it, and is
public as `vk_widget_layout(widget)` (with `vk_box_layout()`,
`vk_grid_layout()`, `vk_frame_layout()` and `vk_window_layout()` as
names for it). It paints nothing.

Most programs never call it: the library arranges after every resize,
and a redraw arranges as it paints. It is for the program that has to
measure before the first redraw, for instance to size a label from the
slot a box has given it:

```c
vk_box_layout(client);                      /* places and sizes, no painting */
vk_widget_get_metrics(VK_WIDGET(label), &w, &h);
```

Before 12.2.0 a box or grid placed its children only inside its redraw,
so the way to measure was to redraw the box and read the result.

## Recreate Propagation

Canvas recreation is triggered during `vk_screen_teleport()` when all
widget canvases must be rebuilt on a new ncurses SCREEN.

`vk_widget_recreate()` does the whole job, in this order (since 11.0.0):

1. `_recreate` -- the widget gets a fresh, blank canvas.
2. Scroll bars attached to the widget get fresh canvases too.
3. `redraw` -- the widget repaints itself.
4. `VK_EVENT_ON_RECREATE` is emitted, for the application.

The base `_recreate` creates a fresh `newwin` and resets `composer` to
the new canvas (clearing `VK_STATE_FROZEN`). Container klasses override
`_recreate` to propagate through the widget tree:

- **vk_frame_t** -- creates its canvas, updates scroller surfaces, recreates
  scrollers, then recreates the child
- **vk_window_t** -- same as frame (scrollers first, then child)
- **vk_box_t** -- creates its canvas, then recreates each slot's widget
- **vk_grid_t** -- creates its canvas, then recreates each cell's widget
- **vk_deck_t** -- propagates recreate to children (deck has no canvas to recreate)

All update child and scroller surface pointers before propagating so
widgets blit to the correct parent canvas. All custom `_recreate` methods
reset `composer = canvas` and clear `VK_STATE_FROZEN`, matching the base
behavior.

A type that only paints (a label, a list, a graph) overrides nothing:
step 3 repaints it. An application registers for `VK_EVENT_ON_RECREATE`
only for what the library cannot know about, such as content it drew on
a widget's canvas by hand.

## Widget State Flags

`vk_widget_t` carries a process-lifetime `uint32_t id`, assigned in
`_vk_widget_ctor` (every visual klass chains there). `0` is reserved
(null widget / unset). Ids are not reused for the life of the process.
There is no registry and no setter; read with `vk_widget_get_id()`.
`_recreate` (teleport) rebuilds the ncurses `WINDOW`, not the widget,
so the id is unchanged.

### Properties (since 11.2.0)

A widget has a handful of on / off properties that say how it behaves.
One is read or changed at a time, and the others are left alone:

```c
vk_widget_set_prop(widget, VK_PROP_EXPAND, false);

if(vk_widget_get_prop(widget, VK_PROP_VISIBLE)) ...
```

| Property | Effect | By name |
|----------|--------|---------|
| `VK_PROP_VISIBLE` | Widget is drawn during refresh. Off = hidden. On by default. | `vk_widget_show()`, `vk_widget_hide()`, `vk_widget_is_visible()` |
| `VK_PROP_FROZEN` | Widget shows a snapshot and ignores repaints. | `vk_widget_freeze()`, `vk_widget_thaw()`, `vk_widget_is_frozen()` |
| `VK_PROP_NORESIZE` | `vk_widget_resize()` returns -1 immediately. | `vk_widget_set_noresize()`, `vk_widget_clear_noresize()`, `vk_widget_is_noresize()` |
| `VK_PROP_EXPAND` | Container resizes widget to fill its slot. | `vk_widget_set_expand()`, `vk_widget_clear_expand()`, `vk_widget_is_expand()` |
| `VK_PROP_INHERIT_FG` | Foreground follows the container's. | the `VK_INHERIT_*` attach flags |
| `VK_PROP_INHERIT_BG` | Background follows the container's. | the `VK_INHERIT_*` attach flags |
| `VK_PROP_INHERIT_ATTRS` | Text attributes follow the container's. | the `VK_INHERIT_*` attach flags |

The names in the last column are macros over the two calls; the ones
that existed before 11.2.0 do what they always did.

`vk_widget_set_prop()` returns 0, or -1 for a NULL widget or for
anything that is not exactly one property.

### The State Word

The properties are bits of a `uint32_t state` on `vk_widget_t`, which
`vk_widget_get_state()` and `vk_widget_set_state()` read and write whole.
Prefer the property calls: writing the whole word means reading it,
changing a bit and writing it back, and a call that passes one flag by
itself turns every other property off.

| Flag | Bit |
|------|-----|
| `VK_STATE_VISIBLE` | `1 << 1` |
| `VK_STATE_FROZEN` | `1 << 3` |
| `VK_STATE_NORESIZE` | `1 << 7` |
| `VK_STATE_EXPAND` | `1 << 8` |
| `VK_STATE_INHERIT_FG` | `1 << 16` |
| `VK_STATE_INHERIT_BG` | `1 << 17` |
| `VK_STATE_INHERIT_ATTRS` | `1 << 18` |

Bits 29 to 31 belong to the library. `vk_widget_set_state()` leaves them
as they are whatever it is passed.

### Visibility

`vk_widget_show()` and `vk_widget_hide()` set/clear `STATE_VISIBLE`.
`vk_widget_is_visible()` queries it. When not visible, `vk_widget_blit()`
returns 0 without blitting. Since `vk_screen_refresh()` erases the surface
canvas before compositing, hidden widgets leave their region transparent
to the wallpaper.

### Frozen

A frozen widget remains visible but its display stops updating. This is
implemented via a `composer` pointer on `vk_widget_t`. Normally `composer`
points to `canvas` — the blit path reads from `composer`, so there is zero
overhead. When `STATE_FROZEN` is set via `vk_widget_set_state()`, a
snapshot of `canvas` is copied into a new window and `composer` is pointed
at the snapshot. Updates continue mutating `canvas`, but the blit keeps
sending the frozen snapshot to the surface. On unfreeze, the snapshot is
deleted and `composer` is reset to `canvas`, resuming live content.

Teleport clears `STATE_FROZEN` and frees the snapshot (the old SCREEN's
windows are invalid on the new terminal).

### No-Resize

When `VK_STATE_NORESIZE` is set, `vk_widget_resize()` refuses to change the
widget's dimensions and returns -1. Useful for fixed-size widgets that
should not be affected by parent layout changes.

### Expand

When a widget is placed inside a container (`vk_box_t` slot or `vk_frame_t`
child), the container only resizes it to fill the available space if
`VK_PROP_EXPAND` is on. It is **off** by default so widgets keep their
natural dimensions. Use `vk_widget_set_expand()` before placing the
widget in a container to opt in, and `vk_widget_clear_expand()` to opt
out again.

## Widget Attributes

`vk_widget_t` carries `fg`, `bg` (color), and `attrs` (ncurses attributes
like `A_BOLD`, `A_UNDERLINE`). Set via `vk_widget_set_colors()` and
`vk_widget_set_attrs()`. Widgets that render text (labels, marquees)
combine colors and attrs into their `wattron`/`wbkgd` calls
automatically. A new widget is black on white.

### Colors From the Container (since 11.2.0)

Every widget paints its own canvas in its own colors; a container does
not paint through what it holds. So that a dialog does not have to color
each widget in it, a widget can **follow** its container: the container
hands its colors on each time it is redrawn, before it redraws what it
holds.

```c
vk_widget_set_colors(VK_WIDGET(window), COLOR_WHITE, COLOR_BLUE);

vk_box_set_widget(box, 0, VK_WIDGET(label),   VK_INHERIT_AUTO);
vk_box_set_widget(box, 1, VK_WIDGET(warning), VK_INHERIT_AUTO);
vk_widget_set_colors(VK_WIDGET(warning), COLOR_RED, COLOR_WHITE);
vk_window_set_child(window, VK_WIDGET(box), VK_INHERIT_AUTO);

vk_window_redraw(window);
```

The box and the label come out white on blue. The warning keeps its red
on white: colors given to a widget are its own.

The last argument of the attach calls says what the child follows:

| Flag | The child |
|------|-----------|
| `VK_INHERIT_AUTO` | follows whichever of the two colors it has not been given itself. **The default**: it is 0 |
| `VK_INHERIT_NONE` | is left as it is; it stands alone |
| `VK_INHERIT_FG`, `VK_INHERIT_BG`, `VK_INHERIT_COLOR` | follows that color, or both, and nothing else |
| `VK_INHERIT_ATTRS`, `VK_INHERIT_ALL` | follows the text attributes, or everything |

Since 12.0.0 `VK_INHERIT_AUTO` is the default, the value 0. (In 11.2.0
`VK_INHERIT_NONE` was 0 and `AUTO` had to be asked for.) A program that
names its flags is unaffected by the change of values once it is
rebuilt.

Rules:

- **Use `VK_INHERIT_AUTO` unless there is a reason not to.** Color the
  outermost widget and what is inside follows. `VK_INHERIT_NONE` is for
  a widget that must keep the look it has whatever it is put in.
- **`vk_widget_set_colors()` wins.** It makes both colors the widget's
  own, whether it is called before or after the attach, and
  `vk_widget_set_attrs()` does the same for attributes.
- **Order does not matter.** A tree may be built bottom-up and the
  window colored last; the first redraw sorts it out. (Before 11.2.0
  the flags copied once, at attach, and the container had to be colored
  first.)
- **A later change follows.** Recolor the container, redraw it, and
  everything that follows it is recolored.
- **Attributes do not follow under `VK_INHERIT_AUTO`.** Bold on a window
  would otherwise make every label in it bold. Ask for them with
  `VK_INHERIT_ATTRS`.
- **To go back to following,** turn the property on:
  `vk_widget_set_prop(widget, VK_PROP_INHERIT_BG, true)`.

What is handed on is `fg`, `bg` and `attrs` only. A type's other colors
(a list's highlight, a progress bar's fill, relief, a scroll bar's
border) are set on that type. A frame's border follows the frame's own
colors unless it is given border colors. A widget attached to a screen
or a deck has no container to follow.

The parts of a popup and of a file dialog follow it: coloring a popup
colors its layout, its button bar, its buttons and the client it is
given, until one of them is given colors of its own
(`vk_popup_set_colors()`, `vk_popup_set_button_colors()`, or
`vk_widget_set_colors()` on the part).

How it works: `vk_widget_set_colors()` notes that the colors are the
widget's own; the attach calls turn the `VK_PROP_INHERIT_*` properties
on; and `vk_container_t`'s redraw, which already visits every child,
copies the container's values into each child that follows before
redrawing it. A child that is a container does the same for its own, so
the colors reach the bottom of the tree.

### 3D Relief Colors

`vk_widget_t` also carries `relief_hi` and `relief_lo` fields that control
the highlight and shadow colors used in 3D beveled rendering. The defaults
are `COLOR_WHITE` (highlight) and `COLOR_BLACK` (shadow). Set via
`vk_widget_set_relief_colors(widget, hi, lo)`. Widgets that render 3D
relief (buttons, inputs, dropdowns) use these fields instead of hardcoded
colors, so the bevel appearance can be configured per widget.

## Labels

`vk_label_t` is a single-line text widget derived from `vk_widget_t`. It
has a fixed height of 1.

| Constant | Justification |
|----------|---------------|
| `VK_JUSTIFY_LEFT` | Left-aligned (default) |
| `VK_JUSTIFY_RIGHT` | Right-aligned |
| `VK_JUSTIFY_CENTER` | Centered |

Text longer than the widget width is clipped at the canvas boundary by
ncursesw. Multi-byte UTF-8 text is safe — rendering relies on ncursesw
window clipping rather than byte-count truncation. The label does not
scroll; see `vk_marquee_t` for overflow scrolling.

## Buttons

`vk_button_t` is a push button derived from `vk_widget_t`. It supports
two families of relief style: **3D** (the default) and **basic**.

Display width is measured via `wcswidth`, so UTF-8 text sizes correctly.

### 3D relief (`VK_FRAME_SINGLE`, `VK_FRAME_ASCII`)

The widget is 3 rows tall (border, centered text, border) and
`1 + display_width + 1` columns wide. The bevel is a two-tone color
split that simulates a raised look. Starting clockwise from the
bottom-left corner (inclusive), the highlight color (`relief_hi` on face)
covers the left edge, top-left corner, and top edge. From the top-right
corner (inclusive), the shadow color (`relief_lo` on face) covers the right
edge, bottom edge, and corners. On press, the highlight and shadow swap
to give a sunken appearance. The relief colors default to white/black
and can be changed via `vk_widget_set_relief_colors()`. `VK_FRAME_SINGLE` uses WACS_* box-drawing
characters; `VK_FRAME_ASCII` uses `+`, `-`, `|` with the same split.

### Basic relief (`VK_BUTTON_BASIC`)

A single-row button in the style of the Links text browser:
`[ text ]`. The widget is 1 row tall and `2 + display_width + 2`
columns wide. On press, the brackets render in reverse video.
`vk_button_set_relief_style` automatically resizes the widget when
switching between 3D and basic styles.

### API

| API | Description |
|-----|-------------|
| `vk_button_create(text)` | Create a button (default: `VK_FRAME_SINGLE`) |
| `vk_button_set_text` | Replace button text (no resize) |
| `vk_button_set_relief_style` | `VK_FRAME_SINGLE`, `VK_FRAME_ASCII`, or `VK_BUTTON_BASIC` |
| `vk_button_set_pressed_colors` | Set fg/bg for the pressed face (3D only) |
| `vk_button_set_on_press` | Register a `VkWidgetFunc` callback + user data |
| `vk_button_press` | Set pressed state and fire callback |
| `vk_button_release` | Clear pressed state |
| `vk_button_redraw` | Redraw the button |

Normal colors are set via `vk_widget_set_colors()`. The button is
IO-agnostic; the application calls `vk_button_press()` /
`vk_button_release()` to toggle state.

## Inputs

`vk_input_t` is a single-line text input derived from `vk_widget_t`. It
supports two relief style families matching the button widget:

### 3D Relief (`VK_FRAME_SINGLE`, `VK_FRAME_ASCII`)

The widget is 3 rows tall with a sunken border (inverted bevel from
buttons): `relief_lo` shadow on top/left, `relief_hi` highlight on
bottom/right. The relief colors default to black/white and can be changed
via `vk_widget_set_relief_colors()`.
`VK_FRAME_SINGLE` uses WACS_* box-drawing characters; `VK_FRAME_ASCII`
uses `+`, `-`, `|`. The text field occupies the middle row between the
borders.

### Basic Relief (`VK_BUTTON_BASIC`)

A single-row input in the style of the Links text browser: `[ text ]`.
The text field sits between the brackets.

### Text Editing

The input manages a dynamically-growing text buffer (initial capacity 256,
doubles via realloc). The cursor position and horizontal scroll offset are
maintained so text wider than the field width scrolls as the cursor moves.
The cursor is rendered with `A_REVERSE` on the character at the cursor
position.

`vk_input_set_max_length()` enforces an optional character limit. When set,
inserts beyond the limit are rejected and existing text is truncated.

### API

| API | Description |
|-----|-------------|
| `vk_input_create(width)` | Create an input (minimum width 5, default 3D style) |
| `vk_input_set_text(input, text)` | Replace text (NULL clears), cursor moves to end |
| `vk_input_get_text(input)` | Return current text (const pointer) |
| `vk_input_set_relief_style` | `VK_FRAME_SINGLE`, `VK_FRAME_ASCII`, or `VK_BUTTON_BASIC` |
| `vk_input_set_max_length` | Set maximum character count (0 = unlimited) |
| `vk_input_insert_char(input, ch)` | Insert printable ASCII (32-126) at cursor |
| `vk_input_backspace(input)` | Delete character before cursor |
| `vk_input_delete(input)` | Delete character at cursor |
| `vk_input_move_cursor(input, offset)` | Move cursor by offset (clamped to bounds) |
| `vk_input_home(input)` | Move cursor to start |
| `vk_input_end(input)` | Move cursor to end |
| `vk_input_clear(input)` | Clear text and reset cursor/scroll |
| `vk_input_redraw(input)` | Redraw the input |
| `vk_input_destroy(input)` | Destroy the input |

The input is IO-agnostic; the application installs a `kmio` handler that
maps keystrokes to the editing API functions.

## Spin Buttons

`vk_spinbutton_t` is a numeric value field with increment / decrement
arrows, derived from `vk_widget_t`. The value is a `double` (so the step
can be any magnitude) clamped to `[min, max]`, with a configurable step
and display precision. Manual text editing is opt-in.

### Layout

The 3-row relief field is split by a vertical separator into a value
field on the left and the arrow controls on the right; a second separator
divides the inc (`▲`) and dec (`▼`) arrows. Under the ASCII relief style
the arrows render as `+` / `-`; under `VK_BUTTON_BASIC` the field collapses
to one row.

### Relief

The value field and the arrow controls carry independent 3D relief
directions, set with `vk_spinbutton_set_field_relief()` and
`vk_spinbutton_set_button_relief()` (each takes `VK_RELIEF_SUNKEN`,
`VK_RELIEF_RAISED`, or `0` for flat; defaults are sunken field / raised
buttons). The colours follow the same convention as `vk_button` and
`vk_frame`: the highlight edge carries the widget's bold attrs so
`relief_hi` reads bright, the shadow edge is plain so `relief_lo` reads
dark. The shared separator is the value field's right edge and the
buttons' left edge at once, so the two regions meet cleanly.

### Interaction

Keyboard is self-contained through the widget's own `kmio`: Up / Down step
the value, and (when editable) digits / `.` / `-` / Backspace / Left /
Right / Home / End edit an inline buffer that is parsed and clamped on
Enter (Esc abandons it). Mouse is parent-assisted -- a leaf widget does
not know its absolute screen position, so the container translates a click
into widget-local coordinates and calls `vk_spinbutton_click()`, which
hit-tests the arrows (and, when editable, the value field).

A value change (an arrow step or a committed edit) emits
`VK_EVENT_ON_CHANGE` and invokes an optional `on_change` `VkWidgetFunc`,
mirroring `vk_button`'s dual notify.

### API

| API | Description |
|-----|-------------|
| `vk_spinbutton_create(width)` | Create a spin button (minimum width 8) |
| `vk_spinbutton_set_range(spin, min, max)` | Set the clamp bounds |
| `vk_spinbutton_set_step(spin, step)` | Set the increment per arrow step |
| `vk_spinbutton_set_value(spin, value)` | Set the value (clamped, no notify) |
| `vk_spinbutton_get_value(spin)` | Return the value (commits a pending edit) |
| `vk_spinbutton_set_precision(spin, n)` | Fractional digits shown |
| `vk_spinbutton_set_editable(spin, bool)` | Allow manual text entry (default off) |
| `vk_spinbutton_set_relief_style` | `VK_BORDER_SINGLE`, `VK_BORDER_ASCII`, or `VK_BUTTON_BASIC` |
| `vk_spinbutton_set_field_relief(spin, relief)` | 3D direction of the value field |
| `vk_spinbutton_set_button_relief(spin, relief)` | 3D direction of the arrow controls |
| `vk_spinbutton_set_on_change(spin, func, data)` | Register a value-change callback |
| `vk_spinbutton_step(spin, n)` | Apply n steps (n may be negative); clamps + notifies |
| `vk_spinbutton_click(spin, lx, ly)` | Route a widget-local click; returns whether consumed |
| `vk_spinbutton_redraw(spin)` | Redraw the spin button |
| `vk_spinbutton_destroy(spin)` | Destroy the spin button |

## Textboxes

`vk_textbox_t` is a multi-line read-only text display derived from
`vk_widget_t`. It supports word wrapping, vertical scrolling via keyboard,
and attached scrollers.

Text is set via `vk_textbox_set_text()`, which stores a copy and reflows
it into a cached line array. The reflow engine honors hard newlines and
breaks long lines at word boundaries (spaces/tabs) when word wrap is
enabled. If no word boundary exists within the paint width, the line is
hard-broken at the column limit.

Scrolling is controlled by the application via a user-installed `kmio`
handler that manipulates `textbox->scroll_top` and calls
`vk_textbox_redraw(textbox)`. The textbox itself is IO-agnostic.

The textbox supports attached scrollers the same way `vk_listbox_t` does
(borderless host pattern). When a vertical scroller is attached, the
textbox renders content 1 column narrower and the scrollbar draws on the
vacated column. Resize and recreate hooks propagate to attached scrollers.

Colors and attributes are set via the standard `vk_widget_set_colors()`
and `vk_widget_set_attrs()` on the textbox widget.

## Marquees

`vk_marquee_t` derives from `vk_label_t`. When text fits the widget width,
it renders identically to a label. When text overflows, it scrolls.

The marquee is a state machine driven by the caller via `vk_marquee_run()`.
Each call is one tick: the marquee moves on, then repaints. The caller
controls the tick rate externally (e.g. via `wtimeout` on the input loop).

Only `vk_marquee_run()` moves it (since 11.1.0). A redraw for any other
reason -- a resize, or a container above it being redrawn -- paints the
marquee where it is. Before 11.1.0 every redraw was a tick.

| Constant | Behavior |
|----------|----------|
| `VK_SCROLL_LEFT` | Scroll left, revealing trailing text |
| `VK_SCROLL_RIGHT` | Scroll right, revealing leading text |
| `VK_SCROLL_LOOP` | Continuous circular crawl (ticker style) |

For `VK_SCROLL_LEFT` and `VK_SCROLL_RIGHT`:

- Starts paused for `pause_duration` ticks
- Scrolls one position every `scroll_interval` ticks
- On completion: if `repeat` is true, resets to initial position and pauses
  again; otherwise stops

For `VK_SCROLL_LOOP`:

- Scrolls continuously with no pause
- Text wraps around with a small gap between repetitions
- Uses wide character rendering (`mvwadd_wch`) for proper unicode support

`vk_marquee_set_text()` is a marquee-specific function (not a pass-through)
because it resets scroll state when text changes. `vk_marquee_get_text()`
reads the text via the inherited label API.

## Selectboxes

`vk_selectbox_t` derives from `vk_listbox_t` and adds check/radio selection
semantics. Items are rendered with indicator glyphs prepended to their names.

| Constant | Mode |
|----------|------|
| `VK_SELECTBOX_CHECKBOX` | Multi-select: each item toggles independently |
| `VK_SELECTBOX_RADIO` | Single-select: toggling unchecks all others first |

### Glyph Styles

The indicator style is set via `vk_selectbox_set_style()`:

| Style | Checkbox unchecked / checked | Radio unchecked / checked | Width |
|-------|------------------------------|---------------------------|-------|
| `VK_FRAME_ASCII` | `[ ]` / `[x]` | `( )` / `(*)` | 4 cols |
| `VK_FRAME_SINGLE` (default) | `[ ]` / `[✓]` | `○` / `●` | 4 / 2 cols |

Unicode checkbox uses U+2713 (check mark) inside ASCII brackets. Unicode
radio uses U+25CB (white circle) and U+25CF (black circle).

### Item State

Each `vk_item_t` carries a `flags` field. `VK_ITEM_CHECKED` (bit 0) tracks
the checked state. The selectbox provides `check_item`, `uncheck_item`,
`toggle_item`, `uncheck_all`, and `item_is_checked` APIs. Separators
cannot be checked.

### Input Handling

The selectbox is IO-agnostic. The application installs a `kmio` handler that
calls `vk_selectbox_toggle_item()` on Enter/Space and delegates navigation
to inherited listbox functions (e.g. `vk_selectbox_set_next()`,
`vk_selectbox_redraw()`). All listbox item management and navigation APIs
are available under `vk_selectbox_*` names via convenience macros.

### Its Own Redraw

The selectbox supplies its own `redraw` in its klass descriptor. Listbox
navigation repaints through `vk_widget_redraw()`, which dispatches on the
object, so it reaches the selectbox's renderer, which prepends indicator
glyphs.

## Dropdowns

`vk_dropdown_t` derives from `vk_listbox_t`. It displays a single-row
collapsed view showing the currently selected item, and expands into a
popup `vk_window_t` containing a copy of the item list for selection.

### Relief Styles

The collapsed view supports two relief families:

| Style | Height | Appearance |
|-------|--------|------------|
| `VK_BUTTON_BASIC` (default) | 1 row | `text ▼` — plain text with arrow |
| `VK_FRAME_SINGLE` | 3 rows | 3D bordered box with text and arrow in a separator column |

The 3D bordered style uses `relief_hi`/`relief_lo` for the bevel (set via
`vk_widget_set_relief_colors()`).

### Popup

When expanded via `vk_dropdown_set_expanded(dropdown, true)`, a
`vk_window_t` popup is created containing a `vk_listbox_t` copy of the
items. The popup inherits the dropdown's widget colors and highlight
colors. The application is responsible for positioning the popup on screen
(via `vk_widget_move` on the popup widget returned by
`vk_dropdown_get_popup()`), attaching it to the screen surface, and
detaching it when collapsed.

`vk_dropdown_popup_navigate(dropdown, direction)` moves the popup's
selection. `vk_dropdown_popup_select(dropdown)` commits the popup's
selection to the dropdown and closes the popup.

### API

All listbox item management and navigation APIs are available under
`vk_dropdown_*` names via convenience macros.

| API | Description |
|-----|-------------|
| `vk_dropdown_create(width, max_visible)` | Create a dropdown (max_visible limits popup height) |
| `vk_dropdown_set_relief_style(dd, style)` | Set collapsed relief style |
| `vk_dropdown_set_expanded(dd, expanded)` | Expand or collapse the popup |
| `vk_dropdown_get_expanded(dd)` | Return expansion state |
| `vk_dropdown_get_popup(dd)` | Return the popup widget (or NULL) |
| `vk_dropdown_popup_navigate(dd, dir)` | Navigate popup selection (-1 = prev, 1 = next) |
| `vk_dropdown_popup_select(dd)` | Commit popup selection and collapse |
| `vk_dropdown_redraw(dd)` | Redraw the collapsed view |
| `vk_dropdown_destroy(dd)` | Destroy dropdown and popup |

## Fillers

`vk_filler_t` is a minimal blank widget derived from `vk_widget_t`. It is
created at 1x1 with `VK_STATE_EXPAND` set, so it absorbs all available
space when placed in a container. Use it as a spacer in non-homogeneous
boxes to push other widgets apart.

The filler's redraw fills the canvas with the widget's color pair. Like
every widget it is repainted after a resize or a recreate, so it paints
correctly when a container resizes or recreates its children.

| API | Description |
|-----|-------------|
| `vk_filler_create()` | Create a 1x1 filler with expand enabled |
| `vk_filler_destroy(filler)` | Destroy the filler |

## Menubars

`vk_menubar_t` is a single-row horizontal menu bar derived from `vk_widget_t`.
It displays labeled items separated by `WACS_VLINE` separators and supports
keyboard navigation with wrap-around.

Each item is a `vk_item_t` (the same node type used by `vk_listbox_t`).
Items render as ` label ` (1 space padding each side). Between items, a
vertical line separator occupies 1 column. The natural width is the sum of
all item widths plus separators.

When `focused` is true, the current item is highlighted using `mvwchgat`
with the highlight color pair. Navigation via `set_next`/`set_prev` always
wraps and emits `VK_EVENT_ON_SELECT`. `exec_curr` emits
`VK_EVENT_ON_ACTIVATE` and calls the item's `VkWidgetFunc` callback.

`hit_test` maps a column offset (relative to the widget) to an item index,
enabling mouse interaction. `get_item_position` returns the column offset
of a given item index, useful for positioning dropdowns.

Like every widget the menubar is repainted after a resize or a recreate,
so it works correctly inside containers that resize their children (e.g.
`vk_box_t`).

| API | Description |
|-----|-------------|
| `vk_menubar_create(width)` | Create a menubar (height always 1) |
| `vk_menubar_add_item(m, name, func, data)` | Add a labeled item |
| `vk_menubar_get_item_count(m)` | Return number of items |
| `vk_menubar_get_curr(m)` | Return current item index |
| `vk_menubar_set_curr(m, idx)` | Set current item (emits ON_SELECT) |
| `vk_menubar_set_next(m)` | Advance with wrap (emits ON_SELECT) |
| `vk_menubar_set_prev(m)` | Retreat with wrap (emits ON_SELECT) |
| `vk_menubar_exec_curr(m)` | Execute current item callback (emits ON_ACTIVATE) |
| `vk_menubar_set_highlight(m, fg, bg)` | Set highlight colors for focused item |
| `vk_menubar_set_focused(m, focused)` | Set focus state (emits ON_FOCUS/ON_UNFOCUS) |
| `vk_menubar_get_focused(m)` | Return focus state |
| `vk_menubar_hit_test(m, x)` | Return item index at column x, or -1 |
| `vk_menubar_get_item_position(m, idx, &x)` | Get column offset of item |
| `vk_menubar_redraw(m)` | Repaint the menubar |
| `vk_menubar_reset(m)` | Remove all items |
| `vk_menubar_destroy(m)` | Destroy the menubar |

## Activity Indicators

`vk_activity_t` is a 1x1 widget derived from `vk_widget_t` that animates
a spinning/cycling indicator. It supports five animation styles:

| Style | Constant | Frames |
|-------|----------|--------|
| Spinner | `VK_ACTIVITY_SPINNER` | ASCII `\|/-\` (4 frames) |
| Dots | `VK_ACTIVITY_DOTS` | Braille characters U+280B series (10 frames) |
| Circles | `VK_ACTIVITY_CIRCLES` | Quarter-circle U+25D0 series (4 frames) |
| Bar | `VK_ACTIVITY_BAR` | Growing block U+2581 series (8 frames) |
| Moon | `VK_ACTIVITY_MOON` | Moon phases U+1F311–U+1F318 (8 frames, widget widens to 2) |

The widget uses a caller-driven tick model like `vk_marquee_t`: the
application calls `vk_activity_run()` each iteration of the event loop.
A `speed` parameter controls how many ticks elapse before advancing to
the next frame (default 1 = advance every tick).

Each call to `vk_activity_run()` is one tick: the indicator moves on,
then repaints. Only that moves it (since 12.1.0). Like every other
widget it has a redraw, which paints the frame it is on: it repaints
itself after a resize, a change of style or a move to another terminal,
and when a container it sits in is redrawn, without moving. Before
12.1.0 it painted only inside `vk_activity_run()`, so it was blank
after any of those until the next tick.

| API | Description |
|-----|-------------|
| `vk_activity_create()` | Create a 1x1 activity indicator (default: spinner, stopped) |
| `vk_activity_set_style(a, style)` | Set animation style; resets frame counter |
| `vk_activity_get_style(a)` | Return current style constant |
| `vk_activity_set_speed(a, interval)` | Set tick interval between frame advances |
| `vk_activity_start(a)` | Begin animating |
| `vk_activity_stop(a)` | Stop animating (clears to blank) |
| `vk_activity_is_running(a)` | Return whether the indicator is active |
| `vk_activity_run(a)` | Tick function — call once per event loop iteration |
| `vk_activity_redraw(a)` | Repaint the frame it is on, without moving |
| `vk_activity_destroy(a)` | Destroy the indicator |

## File Dialogs

`vk_filedialog_t` is a composite widget derived from `vk_box_t`. It provides
a file browser with a path input, scrollable file list, and OK/Cancel buttons
arranged in a 3-slot vertical box with non-homogeneous layout.

### Construction

```c
vk_filedialog_t *dialog = vk_filedialog_create(width, height, style, multiselect);
```

- `style`: `VK_FRAME_SINGLE`, `VK_FRAME_ASCII`, or `VK_BUTTON_BASIC` —
  controls the relief style of buttons and the path input
- `multiselect`: when true, the file list is a `vk_selectbox_t` (checkbox
  mode) instead of a plain `vk_listbox_t`
- Minimum size: 10 columns, 5 rows

The create function builds the internal widget tree:

| Slot | Widget | Flags |
|------|--------|-------|
| 0 | `vk_input_t` (path) | natural height |
| 1 | `vk_listbox_t` or `vk_selectbox_t` (file list) | `VK_STATE_EXPAND` |
| 2 | `vk_box_t` (button bar with OK + Cancel) | natural height |

A `vk_scroller_t` (vertical) is attached to the file list. The box uses
non-homogeneous layout so the path input and button bar keep their natural
heights while the file list absorbs remaining space.

Default path is `$HOME` (falls back to `.` if unset).

### Internal Construction

The filedialog builds its widget tree using internal helpers and direct
struct access rather than the public box API:

- Direct struct access: `VK_BOX(dialog)->homogeneous = false`
- Internal helper: `_vk_filedialog_set_child()` writes `slot_widgets[]`
  and calls `container->add_widget()` directly
- Its redraw repaints its parts, then calls `VK_BOX_KLASS->redraw()` for box layout

### KMIO

Unlike other VDK widgets, the filedialog installs a built-in `kmio` handler
at creation time. This is a deliberate exception to the IO-agnostic pattern
because the filedialog's internal navigation (switching between path input
and file list, directory traversal) requires coordinated keyboard handling
across multiple child widgets.

| Key | File list mode | Path input mode |
|-----|---------------|-----------------|
| `/` | Switch to path input | — |
| Escape | — | Cancel edit, return to file list |
| Enter | Activate (open directory or select file) | Navigate to typed path |
| Up/Down | Move selection | — |
| Space | Toggle checkbox (multiselect only) | Insert space |
| Backspace | Go to parent directory | Delete character |
| Left/Right | — | Move cursor |

### Wrap-Around

`vk_filedialog_set_wrap(dialog, true)` enables wrap-around navigation on
the file list. When enabled, pressing Up on the first item wraps to the
last, and Down on the last wraps to the first. Disabled by default.

### Destructor

The filedialog is a box with all of its parts in the box's slots, so its
dtor frees its own strings, demotes to `vk_box_t` and destroys the box;
the destroy cascade takes the input, the list frame and list, the button
bar and buttons, and the list's scroller.

### API

| API | Description |
|-----|-------------|
| `vk_filedialog_create(w, h, style, multi)` | Create a file dialog |
| `vk_filedialog_set_path(dialog, path)` | Set directory (resolves via `realpath`) |
| `vk_filedialog_get_path(dialog)` | Return current directory path |
| `vk_filedialog_get_selected(dialog)` | Return name of highlighted item |
| `vk_filedialog_set_wrap(dialog, bool)` | Enable/disable wrap-around navigation |
| `vk_filedialog_set_colors(dialog, fg, bg)` | Set colors on all child widgets |
| `vk_filedialog_set_highlight(dialog, fg, bg)` | Set highlight colors on file list |
| `vk_filedialog_set_button_colors(dialog, fg, bg)` | Set button face colors |
| `vk_filedialog_set_button_attrs(dialog, attrs)` | Set button text attributes |
| `vk_filedialog_get_file_list(dialog)` | Return the internal file list widget |
| `vk_filedialog_redraw(dialog)` | Repaint the dialog and everything in it |
| `vk_filedialog_destroy(dialog)` | Destroy dialog and all children |

## Popups

`vk_popup_t` is a composite widget derived from `vk_window_t`. It provides
a bordered dialog with a configurable client area and an optional button
bar, suitable for modal prompts, confirmations, and edit forms.

### Construction

```c
vk_popup_t *popup = vk_popup_create(width, height, style, "OK", "Cancel", NULL);
```

- `style`: `VK_FRAME_SINGLE`, `VK_FRAME_ASCII`, or `VK_BUTTON_BASIC` —
  controls the relief style of buttons created at construction time
- Variadic arguments are NULL-terminated button labels; pass only `NULL`
  for a popup with no buttons
- Minimum size: 5 columns, 3 rows

The create function builds the internal widget tree:

| Slot | Widget | Flags |
|------|--------|-------|
| 0 | client area (`vk_filler_t` default, or user widget) | `VK_STATE_EXPAND` |
| 1 | `vk_box_t` (button bar, horizontal) | natural height |

The layout is a 2-slot non-homogeneous vertical box. The client area
absorbs all available space. The button bar height is 1 for basic style
or 3 for framed styles.

### Client Area

`vk_popup_set_client(popup, widget)` replaces the default filler with a
user-provided widget. The widget is given `VK_STATE_EXPAND`. Passing NULL
restores the default filler. Since 10.0.0 a popup destroys its client
along with everything else in it; call `vk_popup_set_client(popup, NULL)`
before `vk_popup_destroy()` to keep the client.

### Buttons

Buttons can be added at create time via variadic labels or later via
`vk_popup_add_button()`. Up to `VK_POPUP_MAX_BUTTONS` (8) are supported.
When a button is pressed, the popup stores the button's index as the
result and emits `VK_EVENT_ON_CLOSE`. `vk_popup_close(popup, result)`
does the same programmatically.

`vk_popup_add_button` handles three cases: no existing bar (creates one),
bar with available slots (fills next slot), bar needing more slots
(reconstructs the bar).

### Destructor

The popup dtor saves pointers to all owned children, removes them from
containers, demotes to `vk_window_t`, destroys the window, then destroys
each child individually (buttons, button bar, default filler, layout).

### API

All window APIs (title, border style, border colors, border attrs) are
available under `vk_popup_*` names via convenience macros.

| API | Description |
|-----|-------------|
| `vk_popup_create(w, h, style, ...)` | Create a popup with NULL-terminated button labels |
| `vk_popup_set_client(popup, widget)` | Set or clear the client area widget |
| `vk_popup_get_client(popup)` | Return the client widget (or NULL) |
| `vk_popup_add_button(popup, text)` | Add a button dynamically |
| `vk_popup_get_button(popup, index)` | Return a button by index |
| `vk_popup_get_button_count(popup)` | Return number of buttons |
| `vk_popup_get_button_bar(popup)` | Return the button bar box |
| `vk_popup_get_result(popup)` | Return the close result (button index or -1) |
| `vk_popup_close(popup, result)` | Set result and emit `VK_EVENT_ON_CLOSE` |
| `vk_popup_set_colors(popup, fg, bg)` | Set interior (layout) colors |
| `vk_popup_set_button_colors(popup, fg, bg)` | Set button and button bar colors |
| `vk_popup_set_button_attrs(popup, attrs)` | Set button text attributes |
| `vk_popup_redraw(popup)` | Repaint the popup and everything in it |
| `vk_popup_destroy(popup)` | Destroy popup and all owned children |

## Calendars

`vk_calendar_t` is a date display widget derived from `vk_widget_t`. It
renders an XFCE4-style month view: a header row with `< Month Year >`
navigation arrows, a day-of-week row (Su Mo Tu We Th Fr Sa), and a 7x6
grid of day numbers. The widget size is fixed at 22 columns by 8 rows.

Days from the current month use the widget's base colors (set via
`vk_widget_set_colors()`). Today's date uses the highlight colors. Days
from adjacent months use the dimmed colors. The month name and day-of-week
labels use the header colors. Each color region has an independent `attr_t`
field (set via the `_attrs` variant of each setter) so the caller controls
bold, dim, reverse, etc. without any hardcoded attributes in the widget.

`vk_calendar_create()` initializes the calendar to the current month and
records today's date for highlight rendering.

| API | Description |
|-----|-------------|
| `vk_calendar_create()` | Create a calendar showing the current month |
| `vk_calendar_set_month(cal, month, year)` | Set displayed month (0-11) and year |
| `vk_calendar_get_month(cal, &month, &year)` | Get displayed month and year |
| `vk_calendar_prev_month(cal)` | Navigate to previous month (wraps year) |
| `vk_calendar_next_month(cal)` | Navigate to next month (wraps year) |
| `vk_calendar_set_highlight(cal, fg, bg)` | Set today highlight colors |
| `vk_calendar_set_highlight_attrs(cal, attrs)` | Set today highlight attributes |
| `vk_calendar_set_dimmed(cal, fg, bg)` | Set adjacent month day colors |
| `vk_calendar_set_dimmed_attrs(cal, attrs)` | Set adjacent month day attributes |
| `vk_calendar_set_header_colors(cal, fg, bg)` | Set month name / day-of-week colors |
| `vk_calendar_set_header_attrs(cal, attrs)` | Set month name / day-of-week attributes |
| `vk_calendar_redraw(cal)` | Repaint the calendar |
| `vk_calendar_destroy(cal)` | Destroy the calendar |

## Listbox Highlight Attributes

`vk_listbox_set_highlight_attrs(listbox, attrs)` sets the ncurses
attributes applied to the highlighted (current) item. The default is
`A_NORMAL`. This supplements `vk_listbox_set_highlight()` which sets
only the color pair. The attrs are applied via `mvwchgat` alongside
the highlight color.

## Linked Lists

The framework uses Linux kernel-style intrusive linked lists (`list.h`)
throughout:

- `vk_widget_t.list` -- widget membership in a container
- `vk_container_t.widget_list` -- children of a container
- `vk_deck_t.widget_list` -- z-ordered widget stack (head->next = top)
- `vk_listbox_t.item_list` -- items in a listbox/menu (`vk_item_t` nodes)
- `vk_menubar_t.item_list` -- items in a menubar (`vk_item_t` nodes)

## Graphs

`vk_graph_t` is a bar graph widget derived from `vk_widget_t`. It plots a
single series of y-values as vertical bars. `vk_graph_set_bar_width()`
sets a fixed cell width per bar (default 1). Bars pack from the **right**
(newest / last index flush right) and work left; oldest bars clip if they
do not fit. Leftover width stays on the left.

Three bar styles are supported:

| Style | Constant | Render |
|-------|----------|--------|
| Block | `VK_GRAPH_BAR_BLOCK` | U+2588 FULL BLOCK + U+2580..U+2587 partial blocks (1/8-cell sub-cell top) |
| Braille | `VK_GRAPH_BAR_BRAILLE` | 2x4 braille dots per cell (4x vertical resolution) |
| ASCII | `VK_GRAPH_BAR_ASCII` | `#` character with 1/8-cell rounding |

### Layout

When the widget is wide and tall enough for formatted `y_max` plus a
spine column and a bottom row, the graph reserves a **Y gutter** (left)
and an **X row** (bottom). Bars occupy the inner rectangle. Smaller
widgets skip axes and plot full-width bars.

### Y-axis

Always labels **y_max** (top) and **y_min** (bottom), right-aligned,
as `value * unit_scale` (`%.0f`) plus `unit_label` if set. Extra ticks
are added equally spaced only when consecutive labels have at least one
blank row between them. The gutter spine is `|`; a `-` tick is drawn
into the plot at each labeled row. Axis text uses widget fg/bg.

### X-axis

Always labels the **first** and **last** visible bars. Custom strings
come from `vk_graph_set_x_labels`; otherwise the bar index is shown.
Extra labels are equally spaced and omitted if they would overlap.
The axis line is `-`; `|` marks each labeled column. Labels are not
drawn under every bar.

Data is provided via `vk_graph_set_data()` which stores a copy of a
y-value series. The visible index window is controlled by `x_range`
(`vk_graph_set_x_range`): when `x_max <= x_min` all bars are shown.
The value axis is controlled by `y_range` (`vk_graph_set_y_range`).

Bar colours and attributes are set via `vk_graph_set_colors()` and
`vk_graph_set_attrs()`.

### Virtual Method Overrides

The base `_bar_count` returns `data_count` and `_bar_value` returns
`data[index]`. A derived class (e.g. `vk_histogram_t`) overrides these
to expose bars computed from its own dataset while reusing all geometry,
rendering, and range logic.

| API | Description |
|-----|-------------|
| `vk_graph_create(width, height)` | Create a bar graph widget |
| `vk_graph_set_type(graph, type)` | Set graph type (`VK_GRAPH_BAR`) |
| `vk_graph_set_bar_style(graph, style)` | `VK_GRAPH_BAR_BLOCK`, `_BRAILLE`, or `_ASCII` |
| `vk_graph_set_x_range(graph, min, max)` | Visible index window (max <= min = all) |
| `vk_graph_set_y_range(graph, min, max)` | Value axis range |
| `vk_graph_set_unit_scale(graph, scale)` | Display multiplier applied to tick values |
| `vk_graph_set_unit_label(graph, label)` | Unit label appended to tick values |
| `vk_graph_set_x_labels(graph, labels, count)` | Custom X-axis label strings (0/NULL = indices) |
| `vk_graph_set_data(graph, values, count)` | Replace plotted series (owned copy) |
| `vk_graph_set_colors(graph, fg, bg)` | Bar foreground/background colour pair |
| `vk_graph_set_attrs(graph, attrs)` | Bar ncurses attributes |
| `vk_graph_redraw(graph)` | Render current state into the canvas |
| `vk_graph_destroy(graph)` | Destroy the graph |

## Histograms

`vk_histogram_t` derives from `vk_graph_t` and implements a frequency
histogram. It stores a set of raw sample values, bins them into
`nbins` frequency buckets over `[bin_min, bin_max]`, and plots the
per-bin counts by overriding `vk_graph`'s `_bar_count` / `_bar_value`
virtuals. All geometry, bar style, colour, axis labels, and x-range
logic is inherited from `vk_graph`.

> `vk_histogram` inherits `unit_scale`, `unit_label`, and `x_labels` from
> `vk_graph` via `VK_GRAPH(hist)`. Moon-flare can pass time strings via
> `vk_graph_set_x_labels()` on the histogram's graph parent.

### Binning

Samples are distributed into bins using the formula:

```
bin = (sample - bin_min) / (bin_max - bin_min) * nbins
```

Samples outside `[bin_min, bin_max]` are dropped; a sample exactly at
`bin_max` lands in the last bin.

### Auto-scaling

`vk_histogram_redraw()` recomputes the bin counts, finds the tallest
bin, and calls `vk_graph_set_y_range()` with `[0, max_count]` before
rendering. This means the y-axis auto-scales to the data.

### Usage Pattern

```c
vk_histogram_t *hist = vk_histogram_create(74, 12);
vk_histogram_set_bins(hist, 10);
vk_histogram_set_range(hist, 0.0, 100.0);
vk_histogram_set_samples(hist, values, count);
vk_graph_set_bar_style(VK_GRAPH(hist), VK_GRAPH_BAR_BLOCK);
vk_graph_set_colors(VK_GRAPH(hist), COLOR_GREEN, COLOR_BLACK);
vk_histogram_redraw(hist);
```

### API

| API | Description |
|-----|-------------|
| `vk_histogram_create(width, height)` | Create a histogram widget |
| `vk_histogram_set_samples(hist, values, count)` | Replace raw sample set (owned copy) |
| `vk_histogram_set_bins(hist, nbins)` | Set number of frequency bins (min 1, default 10) |
| `vk_histogram_set_range(hist, min, max)` | Set value domain `[min, max]` (default 0..100) |
| `vk_histogram_redraw(hist)` | Rebin, auto-scale y-axis, and render |
| `vk_histogram_destroy(hist)` | Destroy the histogram |

Parent APIs (via `VK_GRAPH(hist)`) for bar style, colours, and x-range:

| API | Description |
|-----|-------------|
| `vk_graph_set_bar_style(VK_GRAPH(hist), style)` | `VK_GRAPH_BAR_BLOCK`, `_BRAILLE`, or `_ASCII` |
| `vk_graph_set_colors(VK_GRAPH(hist), fg, bg)` | Bar colour pair |
| `vk_graph_set_x_range(VK_GRAPH(hist), min, max)` | Visible bin window |

> **Derivation note:** `vk_histogram_t` overrides only the data-access
> virtuals (`_bar_count` returning `nbins`, `_bar_value` returning
> `bin_counts[index]`). Rendering, layout, and colour handling all
> flow through the `vk_graph` implementation, mirroring how
> `vk_meter_t` overrides only `vk_progress`'s `_fill_color`.
