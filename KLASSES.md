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
   │  │
   │  └─ vk_frame_t
   │     │
   │     └─ vk_window_t
   │
   ├─ vk_label_t
   │  │
   │  └─ vk_marquee_t
   │
   ├─ vk_deck_t
   │
   ├─ vk_textbox_t
   │
   ├─ vk_button (todo)
   │
   ├─ vk_spinner (todo)
   │
   └─ vk_listbox_t
      │
      └─ vk_selectbox_t
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
| `VK_LABEL(x)` | `vk_label_t *` |
| `VK_MARQUEE(x)` | `vk_marquee_t *` |
| `VK_LISTBOX(x)` | `vk_listbox_t *` |
| `VK_WINDOW(x)` | `vk_window_t *` |
| `VK_SELECTBOX(x)` | `vk_selectbox_t *` |
| `VK_TEXTBOX(x)` | `vk_textbox_t *` |
| `VK_DECK(x)` | `vk_deck_t *` |

## Klass Templates

Each type declares a global singleton "klass descriptor" using:

```c
declare_klass(KLASS_NAME) { .size = ..., .name = ..., .ctor = ..., ... };
require_klass(KLASS_NAME);    // extern reference from other files
```

These are: `VK_OBJECT_KLASS`, `VK_SCREEN_KLASS`, `VK_WIDGET_KLASS`, `VK_CONTAINER_KLASS`,
`VK_FRAME_KLASS`, `VK_SCROLLER_KLASS`, `VK_WINDOW_KLASS`, `VK_BOX_KLASS`,
`VK_LABEL_KLASS`, `VK_MARQUEE_KLASS`, `VK_LISTBOX_KLASS`,
`VK_SELECTBOX_KLASS`, `VK_TEXTBOX_KLASS`, `VK_DECK_KLASS`.
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
vk_scroller_create(flags)
vk_box_create(width, height, orientation, slots)
vk_label_create(width)
vk_marquee_create(width)
vk_listbox_create(width, height)
vk_window_create(width, height)
vk_selectbox_create(width, height, mode)
vk_textbox_create(width, height)
vk_deck_create(void)
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
_vk_label_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_marquee_ctor    -> VK_LABEL_KLASS->ctor(object, argp)
_vk_window_ctor     -> VK_FRAME_KLASS->ctor(object, argp)
_vk_listbox_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_selectbox_ctor  -> VK_LISTBOX_KLASS->ctor(object, argp)
_vk_textbox_ctor    -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_deck_ctor       -> VK_WIDGET_KLASS->ctor(object, argp)
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

### Example: vk_window_t

```
_vk_window_dtor
  -> free title, detach child
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
  -> detach from host (clear host->vscroller or host->hscroller)
  -> vk_object_demote(object, vk_widget_t)
  -> vk_widget_destroy
    -> _vk_widget_dtor
      -> vk_object_demote(object, vk_object_t)
      -> vk_object_destroy
        -> free(object)
```

> **Note:** `vk_frame_t`, `vk_window_t`, and `vk_box_t` skip the container
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
| `vk_scroller_t` | `ctor`, `dtor`, `_update`, `_draw_scrollbar` |
| `vk_window_t` | `ctor`, `dtor`, `_draw_title` |
| `vk_listbox_t` | `ctor`, `dtor`, `_add_item`, `_set_item`, `_remove_item`, `_get_item`, `_get_item_count`, `_get_selected`, `_exec_item`, `_add_separator`, `_update`, `_reset` |
| `vk_selectbox_t` | `ctor`, `dtor`, `_update` (overrides listbox's `_update`, `_on_resize`, `_on_recreate`) |
| `vk_label_t` | `ctor`, `dtor`, `_update` |
| `vk_marquee_t` | `ctor`, `dtor` (overrides label's `_update`) |
| `vk_textbox_t` | `ctor`, `dtor`, `_update` |
| `vk_deck_t` | `ctor`, `dtor`, `_update` (overrides widget's `_draw`, `_erase`, `_resize`, `_recreate`) |

## Public API Inline Convention

All public API functions in the `.c` files are declared `inline` as a compiler
hint to reduce call overhead. These are thin wrappers that validate arguments,
assert klass identity, and dispatch through the virtual method pointers.

## Headers

`vdk.h` is the public header for VDK. It declares all VDK types, constants,
cast macros, callback typedefs, and function prototypes. It includes
`vdk_color.h`. VDK source files include `vdk.h` through their private
headers and have no dependency on `viper.h`.

`viper.h` is the legacy libviper header. It includes `vdk.h` for backwards
compatibility, so existing code that includes `viper.h` continues to work.

## Color System

VDK uses ncurses color pairs. The `vdk_color.h` header provides:

- `vdk_color_init()` — optional convenience that calls `start_color()` and
  registers all 64 color pairs (8×8 matrix) via `init_pair()`. Must be
  called after `vk_screen_create()`.
- `vdk_color_pair(fg, bg)` — static inline function that maps an (fg, bg)
  pair to an ncurses pair index using fast arithmetic. No globals.
- `VDK_COLORS(fg, bg)` — convenience macro: `COLOR_PAIR(vdk_color_pair(fg, bg))`.

Widget internals use `COLOR_PAIR(vdk_color_pair(fg, bg))` directly (the
ncurses way) rather than the `VDK_COLORS` macro.

If the caller does not call `vdk_color_init()`, they are responsible for
calling `start_color()` and `init_pair()` themselves.

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
| `vk_screen_switch_surface` | Set the active surface by id |
| `vk_screen_get_window` | Return the active surface's canvas |
| `vk_screen_attach_widget` | Attach a widget to a surface |
| `vk_screen_detach_widget` | Detach a widget from a surface |
| `vk_screen_resize` | Handle terminal resize (updates all surface canvases) |
| `vk_screen_poll_resize` | Poll actual terminal size via ioctl; calls resize if changed |
| `vk_screen_teleport` | Migrate the entire UI to a different PTY |
| `vk_screen_set_wallpaper` | Register a wallpaper callback (`VkSurfaceBkgdFunc`) |
| `vk_screen_paint_wallpaper` | Manually invoke the wallpaper callback on the active surface |
| `vk_screen_refresh` | Composite the active surface: erase, wallpaper, widget blit, refresh |
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
4. Overwrite to stdscr and refresh

This means the caller only needs to update widget state and call
`vk_screen_refresh()` — no manual erase, widget draw, or wallpaper
calls required. The wallpaper shows through wherever widgets don't cover.

`vk_screen_paint_wallpaper()` is also available for callers who need to
invoke the wallpaper outside the normal refresh cycle.

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
5. **Recreate widgets** -- rebuild all surface canvases and widget trees
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
- **vk_window_t** -- forwards keystrokes to its child widget.
- **vk_listbox_t** -- handles `KEY_UP`, `KEY_DOWN`, and `Enter` (execute item).
  Skips separators during navigation.
- **vk_selectbox_t** -- intercepts `Enter` and `Space` to toggle the current
  item's checked state. All other keystrokes delegate to `VK_LISTBOX_KLASS->kmio`
  for standard navigation. In radio mode, toggling unchecks all items first.
- **vk_textbox_t** -- handles `KEY_UP`, `KEY_DOWN`, `KEY_PPAGE`, `KEY_NPAGE`,
  `KEY_HOME`, `KEY_END` for scrolling.
- **vk_deck_t** -- `KEY_TAB` rotates the z-order stack (top moves to
  bottom); all other keystrokes forward to the topmost widget.

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

Bordered hosts (frame, window) draw attached scrollers in `_update` after the
border and before children. Borderless hosts (listbox) draw scrollers after
their own content. On resize, the host repositions attached scrollers. On
recreate (teleport), the host updates scroller surfaces.

When there is nothing to scroll (content fits the viewport), the scroller
skips drawing entirely so the host's border is preserved underneath.

On a bordered host (frame/window), the scrollbar replaces the right border
column (vertical) or bottom border row (horizontal). On a borderless host
(e.g. listbox), the scrollbar hugs the edge and the host renders content
1 column/row narrower.

Scroll state is obtained through a `VkScrollInfoFunc` callback. The caller
sets both the callback and a scroll source widget via
`vk_scroller_set_scroll_source()`. The scroller queries the source during
`_update` for content dimensions and scroll offsets. The scroller does not
drive scrolling -- the source widget manages its own scroll position.

Because `vk_object_assert` uses exact type matching, `vk_window_t` provides
its own wrapper functions for inherited frame APIs (asserting `vk_window_t`).
Both `vk_window_t` and `vk_frame_t` override `_update` to include scroller
drawing.

## Windows

`vk_window_t` inherits from `vk_frame_t` and adds a title drawn on the top
border and a user decoration callback. Like `vk_scroller_t`, it overrides
`_update` and `_recreate`.

The title is rendered on border row 0 using the border colors with `A_BOLD`.
Justification is controlled by:

| Constant | Alignment |
|----------|-----------|
| `VK_JUSTIFY_LEFT` | Title starts at column 2 |
| `VK_JUSTIFY_CENTER` | Title centered on border |
| `VK_JUSTIFY_RIGHT` | Title ends at column width-2 |

If the border style is `VK_FRAME_NONE`, no title is drawn.

A `VkWindowDecorateFunc` callback can be registered via
`vk_window_set_decorate()`. It fires during `_update` after the border and
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
widget, placed via `vk_box_set_widget(box, slot, widget)`. Slots are evenly
sized based on the box dimensions; the last slot absorbs any remainder.

The box ctor extracts `orientation` and `slots` from the va_arg chain after
the parent (container -> widget) consumes `width` and `height`. This is the
first klass to consume extra construction parameters beyond the base pair.

The box's `_update` erases, then for each slot: positions, resizes, and draws
the child widget. The box does not propagate resize to grandchildren (e.g. a
frame's child won't be resized if the box resizes the frame).

## Decks

`vk_deck_t` is a z-order manager derived from `vk_widget_t`. It maintains a
linked list of child widgets in stacking order and composites them bottom-to-top
during the screen refresh cycle.

Unlike other widget types, the deck has **no canvas of its own**. The ctor
frees the canvas allocated by the widget base class and sets it to NULL.
During `_draw`, the deck iterates its children in reverse (bottom-to-top)
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
| `vk_deck_update(deck)` | Manually composite children (delegates to `_draw`) |
| `vk_deck_destroy(deck)` | Detach all children and destroy |

The deck's `_draw` sets each child's `surface` pointer to `deck->surface`
before drawing. This means children do not need a valid surface at add time —
the binding happens lazily during the screen refresh cycle.

`_recreate` (teleport) propagates `vk_widget_recreate()` to all children so
they rebuild their canvases on the new SCREEN. The deck itself has nothing
to recreate.

## Resize Propagation

`vk_widget_t` has an `_on_resize` hook slot. When `vk_widget_resize()` 
successfully resizes the canvas, it fires `widget->_on_resize(widget)` if
non-NULL. Container klasses install their own handlers to propagate resize
to children:

- **vk_frame_t** -- resizes child to `(width - 2, height - 2)`;
  resizes and repositions attached scrollers
- **vk_box_t** -- recalculates slot dimensions and resizes each child
- **vk_listbox_t** -- resizes and repositions attached scrollers, repaints
- **vk_selectbox_t** -- resizes and repositions attached scrollers, repaints
  (overrides listbox's `_on_resize` because listbox calls its static `_update`
  directly rather than through the function pointer)
- **vk_textbox_t** -- resizes and repositions attached scrollers, reflows
  text, repaints

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

- **vk_frame_t** -- creates its canvas, updates scroller surfaces, recreates
  scrollers, then recreates the child
- **vk_window_t** -- same as frame (scrollers first, then child)
- **vk_box_t** -- creates its canvas, then recreates each slot's widget
- **vk_deck_t** -- propagates recreate to children (deck has no canvas to recreate)

All update child and scroller surface pointers before propagating so
widgets blit to the correct parent canvas.

The `_on_recreate` hook is for content-bearing widgets that need to repaint
after canvas recreation. Widget types install it in their ctors:

- **vk_listbox_t** -- updates attached scroller surfaces, recreates them,
  then calls its `_update` to repaint items and scrollers
- **vk_selectbox_t** -- same as listbox (overrides `_on_recreate` for the
  same dispatch reason as `_on_resize`)
- **vk_textbox_t** -- updates attached scroller surfaces, recreates them,
  then calls its `_update` to repaint text and scrollers

Callers can also set `_on_recreate` directly on plain `vk_widget_t`
instances for custom content (the same way `_on_resize` is set).

## Widget State Flags

`vk_widget_t` carries a `uint32_t state` bitfield. The ctor initializes it
to `STATE_VISIBLE`. State can be read with `vk_widget_get_state()` and
written with `vk_widget_set_state()`.

| Flag | Bit | Effect |
|------|-----|--------|
| `STATE_VISIBLE` | `1 << 1` | Widget is drawn during refresh. Cleared = hidden. |
| `STATE_FROZEN` | `1 << 3` | Widget skips drawing but remains visible in layout. |
| `STATE_NORESIZE` | `1 << 7` | `vk_widget_resize()` returns -1 immediately. |

### Visibility

`vk_widget_show()` and `vk_widget_hide()` set/clear `STATE_VISIBLE`.
`vk_widget_is_visible()` queries it. When not visible, `vk_widget_draw()`
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

When `STATE_NORESIZE` is set, `vk_widget_resize()` refuses to change the
widget's dimensions and returns -1. Useful for fixed-size widgets that
should not be affected by parent layout changes.

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

## Textboxes

`vk_textbox_t` is a multi-line read-only text display derived from
`vk_widget_t`. It supports word wrapping, vertical scrolling via keyboard,
and attached scrollers.

Text is set via `vk_textbox_set_text()`, which stores a copy and reflows
it into a cached line array. The reflow engine honors hard newlines and
breaks long lines at word boundaries (spaces/tabs) when word wrap is
enabled. If no word boundary exists within the paint width, the line is
hard-broken at the column limit.

Scrolling is handled internally by `_vk_textbox_kmio`:

| Key | Action |
|-----|--------|
| `KEY_UP` | Scroll up one line |
| `KEY_DOWN` | Scroll down one line |
| `KEY_PPAGE` | Scroll up one page |
| `KEY_NPAGE` | Scroll down one page |
| `KEY_HOME` | Jump to top |
| `KEY_END` | Jump to bottom |

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

### KMIO

`Enter` and `Space` toggle the current item. In radio mode, all items are
unchecked first, then the current item is checked. All other keystrokes
(navigation, wrapping) delegate to `VK_LISTBOX_KLASS->kmio`.

### Wrapper Functions

Because `vk_object_assert` uses exact type matching, `vk_selectbox_t`
provides its own wrapper functions (e.g. `vk_selectbox_add_item`,
`vk_selectbox_set_wrap`, `vk_selectbox_set_highlight`) that assert
`vk_selectbox_t` then delegate to the listbox's function pointers.

### _update Override

The selectbox overrides `VK_LISTBOX(selectbox)->_update` to point to its
own rendering function. This ensures that internal listbox navigation
(which calls `listbox->_update(listbox)` through the function pointer)
dispatches to the selectbox's renderer, which prepends indicator glyphs.

The selectbox also overrides `_on_resize` and `_on_recreate` because the
listbox's versions call `_vk_listbox_update` directly (static function,
not through the function pointer). The selectbox's overrides follow the
same scroller-propagation pattern as the listbox's.

## Linked Lists

The framework uses Linux kernel-style intrusive linked lists (`list.h`)
throughout:

- `vk_widget_t.list` -- widget membership in a container
- `vk_container_t.widget_list` -- children of a container
- `vk_deck_t.widget_list` -- z-ordered widget stack (head->next = top)
- `vk_listbox_t.item_list` -- items in a listbox/menu (`vk_item_t` nodes)
