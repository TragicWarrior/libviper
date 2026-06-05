# VK Klass Reference

## Hierarchy

```
vk_object_t
│
├─ vk_screen_t
│
└─ vk_widget_t
   │
   ├─ vk_container_t
   │  │
   │  ├─ vk_box_t
   │  │
   │  └─ vk_frame_t
   │     │
   │     └─ vk_scroller_t
   │
   ├─ vk_label_t
   │  │
   │  └─ vk_marquee_t
   │
   ├─ vk_button (todo)
   │
   ├─ vk_spinner (todo)
   │
   └─ vk_listbox_t
      │
      └─ vk_menu_t
```

## Klass Framework Overview

The VK klass system is a hand-rolled object-oriented type system in C. It
provides single inheritance, virtual dispatch via function pointers, and a
rudimentary RTTI mechanism.

## Struct Embedding (Inheritance)

Each derived klass embeds its parent as the first struct member (`parent_klass`).
A pointer to any derived type can be safely cast to any of its ancestors.
Cast macros are defined in `viper.h`:

| Macro | Target Type |
|-------|-------------|
| `VK_OBJECT(x)` | `vk_object_t *` |
| `VK_SCREEN(x)` | `vk_screen_t *` |
| `VK_WIDGET(x)` | `vk_widget_t *` |
| `VK_CONTAINER(x)` | `vk_container_t *` |
| `VK_FRAME(x)` | `vk_frame_t *` |
| `VK_SCROLLER(x)` | `vk_scroller_t *` |
| `VK_BOX(x)` | `vk_box_t *` |
| `VK_LABEL(x)` | `vk_label_t *` |
| `VK_MARQUEE(x)` | `vk_marquee_t *` |
| `VK_LISTBOX(x)` | `vk_listbox_t *` |
| `VK_MENU(x)` | `vk_menu_t *` |

## Klass Templates

Each type declares a global singleton "klass descriptor" using:

```c
declare_klass(KLASS_NAME) { .size = ..., .name = ..., .ctor = ..., ... };
require_klass(KLASS_NAME);    // extern reference from other files
```

These are: `VK_OBJECT_KLASS`, `VK_SCREEN_KLASS`, `VK_WIDGET_KLASS`, `VK_CONTAINER_KLASS`,
`VK_FRAME_KLASS`, `VK_SCROLLER_KLASS`, `VK_BOX_KLASS`, `VK_LABEL_KLASS`,
`VK_MARQUEE_KLASS`, `VK_LISTBOX_KLASS`, `VK_MENU_KLASS`.
The template carries the type's size, name, constructor, destructor, and
optional kmio handler. It serves as both the type descriptor and the vtable
seed.

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
vk_scroller_create(width, height)
vk_box_create(width, height, orientation, slots)
vk_label_create(width)
vk_marquee_create(width)
vk_listbox_create(width, height)
vk_menu_create(width, height)
```

## Constructor Chaining

Each derived ctor explicitly calls its parent's ctor through the parent's
klass singleton before installing its own methods:

```
_vk_screen_ctor     -> vk_object_construct (no parent ctor call)
_vk_container_ctor  -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_frame_ctor      -> VK_CONTAINER_KLASS->ctor(object, argp)
_vk_scroller_ctor   -> VK_FRAME_KLASS->ctor(object, argp)
_vk_box_ctor        -> VK_CONTAINER_KLASS->ctor(object, argp)
_vk_label_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_marquee_ctor    -> VK_LABEL_KLASS->ctor(object, argp)
_vk_listbox_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_menu_ctor       -> VK_LISTBOX_KLASS->ctor(object, argp)
```

The `(argp == NULL)` check in each ctor distinguishes "called directly" from
"called by a superclass" for `va_list` forwarding.

After the parent constructs, the derived ctor overwrites the function pointer
slots in its own struct region with its implementations.

## Destructor Chaining (Demote Pattern)

Destruction cascades from derived to base. A derived dtor:

1. Cleans up its own resources
2. Calls `vk_object_demote(object, parent_type)` to rewrite `.name` and `.size`
3. Delegates to the parent's destroy function

This continues until `vk_object_destroy()` calls `free()`.

### Example: vk_menu_t

```
_vk_menu_dtor
  -> vk_object_demote(object, vk_listbox_t)
  -> vk_listbox_destroy
    -> _vk_listbox_dtor
      -> vk_object_demote(object, vk_widget_t)
      -> vk_widget_destroy
        -> _vk_widget_dtor
          -> vk_object_demote(object, vk_object_t)
          -> vk_object_destroy
            -> free(object)
```

### Example: vk_scroller_t

```
_vk_scroller_dtor
  -> detach child from container
  -> vk_object_demote(object, vk_widget_t)
  -> vk_widget_destroy
    -> _vk_widget_dtor
      -> vk_object_demote(object, vk_object_t)
      -> vk_object_destroy
        -> free(object)
```

> **Note:** `vk_frame_t`, `vk_scroller_t`, and `vk_box_t` skip the container
> dtor and demote directly to `vk_widget_t` because `vk_container_t` does not
> install its own dtor during construction (left NULL from calloc).
> Similarly, `vk_label_t` and `vk_marquee_t` demote directly to `vk_widget_t`.

## RTTI (vk_object_assert)

`vk_object_assert(object, type)` compares the object's runtime `.name` string
and `.size` against a compile-time type. Returns 1 if the object IS that exact
type, 0 otherwise. Used as a guard in public APIs to verify klass identity
before dispatching.

## Virtual Methods

Each klass level defines function pointer slots in its struct for polymorphic
dispatch. Public APIs call through these pointers.

| Klass | Slots |
|-------|-------|
| `vk_object_t` | `ctor`, `dtor`, `kmio` |
| `vk_screen_t` | `ctor`, `dtor` |
| `vk_widget_t` | `ctor`, `dtor`, `_draw`, `_move`, `_resize`, `_on_resize`, `_recreate`, `_on_recreate`, `_erase` |
| `vk_container_t` | `ctor`, `dtor`, `add_widget`, `remove_widget`, `vacate`, `rotate` |
| `vk_box_t` | `ctor`, `dtor`, `_update` |
| `vk_frame_t` | `ctor`, `dtor`, `_set_border_style`, `_set_child`, `_draw_border`, `_update` |
| `vk_scroller_t` | `ctor`, `dtor`, `_draw_scrollbar` |
| `vk_listbox_t` | `ctor`, `dtor`, `_add_item`, `_set_item`, `_remove_item`, `_get_item`, `_get_item_count`, `_get_selected`, `_exec_item`, `_update`, `_reset` |
| `vk_label_t` | `ctor`, `dtor`, `_update` |
| `vk_marquee_t` | `ctor`, `dtor` (overrides label's `_update`) |
| `vk_menu_t` | `ctor`, `dtor`, `_set_frame`, `_add_separator`, `_update`, `_reset` |

## Screens and Desktops

`vk_screen_t` manages the ncurses terminal (SCREEN) and provides virtual
desktops. It derives directly from `vk_object_t`, not `vk_widget_t`.

`vk_screen_create()` calls `newterm()` on stdin/stdout, initializes colors,
keypad, raw mode, and creates one default desktop. Each desktop holds its own
full-screen canvas (WINDOW) and an array of attached widgets.

| Function | Purpose |
|----------|---------|
| `vk_screen_add_desktop` | Add a new virtual desktop, returns its id |
| `vk_screen_del_desktop` | Remove a desktop by id (minimum one must remain) |
| `vk_screen_switch_desktop` | Set the active desktop by id |
| `vk_screen_get_window` | Return the active desktop's canvas |
| `vk_screen_attach_widget` | Attach a widget to a desktop |
| `vk_screen_detach_widget` | Detach a widget from a desktop |
| `vk_screen_resize` | Handle terminal resize (updates all desktop canvases) |
| `vk_screen_poll_resize` | Poll actual terminal size via ioctl; calls resize if changed |
| `vk_screen_teleport` | Migrate the entire UI to a different PTY |
| `vk_screen_refresh` | Blit the active desktop canvas to stdscr and refresh |
| `vk_screen_destroy` | Tear down screen, restore evicted terminal, free resources |

### Teleport

`vk_screen_teleport(screen, pty_path)` migrates the UI to a different
terminal. The sequence is:

1. **Release previous PTY** -- if an earlier teleport evicted a shell,
   restore that PTY's termios, SIGCONT the shell, and SIGINT it so
   readline redraws a clean prompt.
2. **Evict target PTY** -- find the session leader on the target PTY
   and SIGSTOP it so our process can claim the terminal. Session leader
   discovery uses utmpx (POSIX) with a `/proc` fallback for modern
   terminal emulators that don't write utmpx entries.
3. **Save termios** -- snapshot the target PTY's terminal attributes
   before ncurses changes them.
4. **Create new SCREEN** -- open the PTY, call `newterm()`, initialize
   colors/keypad/raw mode.
5. **Recreate widgets** -- rebuild all desktop canvases and widget trees
   on the new screen via `vk_widget_recreate()`.
6. **Tear down old terminal** -- `endwin()` the old SCREEN, close old
   file handles.
7. **Drain input** -- flush stale terminal response bytes from the
   `newterm`/`keypad` initialization sequences.

Old ncurses WINDOWs are intentionally leaked during teleport because
`delwin` on windows bound to a different SCREEN corrupts ncurses internal
state. The old SCREEN is shut down with `endwin()` but not `delscreen()`
for the same reason. This is bounded: teleport is a rare operation and
each invocation leaks only the previous set of canvases.

On destroy, the screen performs the same PTY handoff: restore termios,
SIGCONT, SIGINT.

### Post-Teleport Resize

After teleport, our process is not in the target terminal's session, so
SIGWINCH from that terminal goes to the stopped shell, not us.
`vk_screen_poll_resize()` works around this by calling
`ioctl(TIOCGWINSZ)` on the screen's `fd_out` to read the actual terminal
dimensions, comparing against stored values, and calling
`vk_screen_resize()` if they differ. Callers should invoke it each tick
of their event loop (the demo does this alongside `KEY_RESIZE` checks).

## KMIO (Keyboard/Mouse I/O)

The `kmio` function pointer on `vk_object_t` is the input handler. Keystrokes
are pushed to an object via `vk_object_push_keystroke()`.

- **vk_container_t** -- forwards keystrokes to the focused child widget in
  its list. `KEY_TAB` rotates focus.
- **vk_box_t** -- `KEY_TAB` cycles focused slot; all other keystrokes
  forward to the widget in the focused slot.
- **vk_frame_t** -- forwards keystrokes to its child widget.
- **vk_scroller_t** -- forwards keystrokes to its child widget.
- **vk_listbox_t** -- handles `KEY_UP`, `KEY_DOWN`, and `Enter` (execute item).
- **vk_menu_t** -- same as listbox but skips separators during navigation.

## Frames and Scrollers

`vk_frame_t` is a bordered single-child container. It supports three border
styles:

| Constant | Style |
|----------|-------|
| `VK_FRAME_NONE` | No border |
| `VK_FRAME_ASCII` | Drawn with `+`, `-`, `\|` |
| `VK_FRAME_SINGLE` | ncurses ACS single-line box |
| `VK_FRAME_DOUBLE` | Unicode double-line box (U+2550 et al.) |

The border occupies one cell on each side, so the child is sized to
`(width-2, height-2)`. Border colors default to -1 (inherit from widget
fg/bg) but can be overridden independently. Minimum size is 3x3.

`vk_scroller_t` inherits from `vk_frame_t` and draws scrollbar indicators
atop the frame borders. Scrollbar display is controlled by flags:

| Flag | Effect |
|------|--------|
| `VK_SCROLLBAR_NONE` | No scrollbars |
| `VK_SCROLLBAR_VERTICAL` | Vertical scrollbar on right border |
| `VK_SCROLLBAR_HORIZONTAL` | Horizontal scrollbar on bottom border |
| `VK_SCROLLBAR_BOTH` | Both scrollbars |

Scroll state is obtained through a `VkScrollInfoFunc` callback which the
caller registers. The callback receives the child widget and returns content
dimensions and current scroll offsets. The scroller itself does not drive
scrolling -- the child manages its own scroll position and the scroller
queries it for indicator rendering.

Because `vk_object_assert` uses exact type matching, `vk_scroller_t` provides
its own wrapper functions for all inherited frame APIs (e.g.
`vk_scroller_set_child` delegates to `frame->_set_child` after asserting
`vk_scroller_t`). The scroller ctor overwrites `VK_FRAME(scroller)->_update`
to point at the scroller's own update function which adds the scrollbar draw
step.

## Boxes

`vk_box_t` is a layout container that arranges child widgets in a row
(horizontal) or column (vertical). It derives from `vk_container_t`.

| Constant | Orientation |
|----------|-------------|
| `VK_BOX_HORIZONTAL` | Children arranged left-to-right in columns |
| `VK_BOX_VERTICAL` | Children arranged top-to-bottom in rows |

The number of slots is fixed at creation (default 1). Each slot holds one
widget, placed via `vk_box_set_widget(box, slot, widget)`. Slots are evenly
sized based on the box dimensions; the last slot absorbs any remainder.

The box ctor extracts `orientation` and `slots` from the va_arg chain after
the parent (container -> widget) consumes `width` and `height`. This is the
first klass to consume extra construction parameters beyond the base pair.

The box's `_update` erases, then for each slot: positions, resizes, and draws
the child widget. The box does not propagate resize to grandchildren (e.g. a
frame's child won't be resized if the box resizes the frame).

## Resize Propagation

`vk_widget_t` has an `_on_resize` hook slot. When `vk_widget_resize()` 
successfully resizes the canvas, it fires `widget->_on_resize(widget)` if
non-NULL. Container klasses install their own handlers to propagate resize
to children:

- **vk_frame_t** -- resizes child to `(width - 2, height - 2)`
- **vk_box_t** -- recalculates slot dimensions and resizes each child

Propagation is recursive: resizing a box triggers its `_on_resize`, which
resizes each slot's widget, which in turn fires their `_on_resize` hooks
(e.g. a frame inside a box will resize its own child).

## Recreate Propagation

`vk_widget_t` has `_recreate` and `_on_recreate` hook slots. Canvas
recreation is triggered during `vk_screen_teleport()` when all widget
canvases must be rebuilt on a new ncurses SCREEN.

`vk_widget_recreate(widget)` calls `widget->_recreate(widget)` to create
the new canvas, then fires `widget->_on_recreate(widget)` if non-NULL.

The base `_recreate` creates a fresh `newwin`. Container klasses override
it to propagate through the widget tree:

- **vk_frame_t** -- creates its canvas, then recreates the child
- **vk_box_t** -- creates its canvas, then recreates each slot's widget

Both update child surface pointers before propagating so children blit to
the correct parent canvas.

The `_on_recreate` hook is for content-bearing widgets that need to repaint
after canvas recreation. Widget types install it in their ctors:

- **vk_listbox_t** -- calls its `_update` to repaint items
- **vk_menu_t** -- calls its `_update` to repaint items and separators

Callers can also set `_on_recreate` directly on plain `vk_widget_t`
instances for custom content (the same way `_on_resize` is set).

## Widget Attributes

`vk_widget_t` carries `fg`, `bg` (color), and `attrs` (ncurses attributes
like `A_BOLD`, `A_UNDERLINE`). Set via `vk_widget_set_attrs()`. Widgets
that render text (labels, marquees) combine colors and attrs into their
`wattron`/`wbkgd` calls automatically.

## Labels

`vk_label_t` is a single-line text widget derived from `vk_widget_t`. It
has a fixed height of 1.

| Constant | Justification |
|----------|---------------|
| `VK_JUSTIFY_LEFT` | Left-aligned (default) |
| `VK_JUSTIFY_RIGHT` | Right-aligned |
| `VK_JUSTIFY_CENTER` | Centered |

Text longer than the widget width is truncated. The label does not scroll;
see `vk_marquee_t` for overflow scrolling.

## Marquees

`vk_marquee_t` derives from `vk_label_t`. When text fits the widget width,
it renders identically to a label. When text overflows, it scrolls.

The marquee is a state machine driven by the caller via `vk_marquee_run()`.
Each call is one tick. The caller controls the tick rate externally (e.g.
via `wtimeout` on the input loop).

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

Because `vk_object_assert` uses exact type matching, `vk_marquee_t`
provides its own wrapper functions for text get/set (delegating to the
label's text field internally).

## Linked Lists

The framework uses Linux kernel-style intrusive linked lists (`list.h`)
throughout:

- `vk_widget_t.list` -- widget membership in a container
- `vk_container_t.widget_list` -- children of a container
- `vk_listbox_t.item_list` -- items in a listbox/menu (`vk_item_t` nodes)
