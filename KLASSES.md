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
   ├─ vk_button_t
   │
   ├─ vk_input_t
   │
   ├─ vk_filler_t
   │
   ├─ vk_activity_t
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
| `VK_BUTTON(x)` | `vk_button_t *` |
| `VK_INPUT(x)` | `vk_input_t *` |
| `VK_FILLER(x)` | `vk_filler_t *` |
| `VK_FILEDIALOG(x)` | `vk_filedialog_t *` |

## Klass Templates

Each type declares a global singleton "klass descriptor" using:

```c
declare_klass(KLASS_NAME) { .size = ..., .name = ..., .ctor = ..., ... };
require_klass(KLASS_NAME);    // extern reference from other files
```

These are: `VK_OBJECT_KLASS`, `VK_SCREEN_KLASS`, `VK_WIDGET_KLASS`, `VK_CONTAINER_KLASS`,
`VK_FRAME_KLASS`, `VK_SCROLLER_KLASS`, `VK_WINDOW_KLASS`, `VK_BOX_KLASS`,
`VK_LABEL_KLASS`, `VK_MARQUEE_KLASS`, `VK_LISTBOX_KLASS`,
`VK_SELECTBOX_KLASS`, `VK_TEXTBOX_KLASS`, `VK_DECK_KLASS`,
`VK_BUTTON_KLASS`, `VK_INPUT_KLASS`, `VK_FILLER_KLASS`,
`VK_FILEDIALOG_KLASS`.
The template carries the type's size, name, constructor, and destructor.
It serves as both the type descriptor and the vtable seed.

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
vk_button_create(text)
vk_input_create(width)
vk_filler_create(void)
vk_filedialog_create(width, height, style, multiselect)
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
_vk_button_ctor     -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_input_ctor      -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_filler_ctor     -> VK_WIDGET_KLASS->ctor(object, argp)
_vk_filedialog_ctor -> VK_BOX_KLASS->ctor(object, argp)
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
| `vk_object_t` | `ctor`, `dtor`, `kmio` |
| `vk_screen_t` | `ctor`, `dtor` |
| `vk_widget_t` | `ctor`, `dtor`, `_draw`, `_move`, `_resize`, `_recreate`, `_erase` |
| `vk_container_t` | `ctor`, `dtor`, `add_widget`, `remove_widget`, `vacate`, `rotate` |
| `vk_box_t` | `ctor`, `dtor`, `_update` |
| `vk_frame_t` | `ctor`, `dtor`, `_set_border_style`, `_set_child`, `_draw_border`, `_update` |
| `vk_scroller_t` | `ctor`, `dtor`, `_update`, `_draw_scrollbar` |
| `vk_window_t` | `ctor`, `dtor`, `_draw_title` |
| `vk_listbox_t` | `ctor`, `dtor`, `_add_item`, `_set_item`, `_remove_item`, `_get_item`, `_get_item_count`, `_get_selected`, `_exec_item`, `_add_separator`, `_update`, `_reset` |
| `vk_selectbox_t` | `ctor`, `dtor`, `_update` (overrides listbox's `_update`) |
| `vk_label_t` | `ctor`, `dtor`, `_update` |
| `vk_marquee_t` | `ctor`, `dtor` (overrides label's `_update`) |
| `vk_textbox_t` | `ctor`, `dtor`, `_update` |
| `vk_deck_t` | `ctor`, `dtor`, `_update` (overrides widget's `_draw`, `_erase`, `_resize`, `_recreate`) |
| `vk_button_t` | `ctor`, `dtor`, `_update` |
| `vk_input_t` | `ctor`, `dtor`, `_update` |
| `vk_filler_t` | `ctor`, `dtor` |
| `vk_filedialog_t` | `ctor`, `dtor` |

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
color system API. VDK source files include `vdk.h` through their private
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
| `vk_screen_set_surface` | Set the active surface by id |
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

**Caveat:** wallpaper callbacks should paint the surface canvas manually
with `wattron`/`mvwaddch` (or `setcchar`/`mvwadd_wch`). Avoid `wbkgd()`
on the surface canvas — it sets the window's background color pair, which
causes ncurses pair 0 to inherit that color instead of the terminal
default (white-on-black). This breaks any rendering that relies on pair 0,
such as deck drop shadows.

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
8. **Emit `VK_EVENT_ON_TELEPORT`** -- fires all handlers registered on
   the screen object (e.g. to reinitialize colors via `vdk_color_init()`).

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
  keys and calls `_update` to repaint.
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
| `VK_EVENT_ON_RESIZE` | 1 | `vk_widget_resize()` after canvas resize succeeds |
| `VK_EVENT_ON_RECREATE` | 2 | `vk_widget_recreate()` after `_recreate` succeeds |
| `VK_EVENT_ON_TELEPORT` | 3 | `vk_screen_teleport()` after terminal migration completes |
| `VK_EVENT_ON_CLICK` | 10 | `vk_button_press()` |
| `VK_EVENT_ON_SELECT` | 11 | listbox/selectbox navigation and check/radio selection |
| `VK_EVENT_ON_UNSELECT` | 12 | selectbox uncheck |
| `VK_EVENT_ON_ACTIVATE` | 13 | listbox/selectbox `exec_curr` |
| `VK_EVENT_ON_SUBMIT` | 14 | reserved for input submission |
| `VK_EVENT_ON_FOCUS` | 20 | `vk_box_set_subfocus()` on the newly focused slot widget |
| `VK_EVENT_ON_UNFOCUS` | 21 | `vk_box_set_subfocus()` on the previously focused slot widget |
| `VK_EVENT_ON_SCROLL` | 22 | textbox scroll functions |

### Internal Use

Widget klasses register their own event handlers in their constructors for
resize and recreate propagation. For example, `vk_box_t` registers an
`ON_RESIZE` handler that recalculates slot dimensions, and `vk_listbox_t`
registers `ON_RESIZE` and `ON_RECREATE` handlers that propagate to attached
scrollers and repaint. These internal handlers coexist with any application
handlers on the same event — `vk_object_emit` fires all of them.

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

Both `vk_window_t` and `vk_frame_t` override `_update` to include scroller
drawing.

## Windows

`vk_window_t` inherits from `vk_frame_t` and adds a title drawn on the top
border and a user decoration callback. Like `vk_scroller_t`, it overrides
`_update` and `_recreate`. All frame APIs (border style, border colors,
child management, update) are available under `vk_window_*` names via
convenience macros.

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
space evenly, with the last expand child taking any remainder.

The box's `_update` erases, then for each slot: positions, resizes (if
expand), and draws the child widget. The box does not propagate resize to
grandchildren (e.g. a frame's child won't be resized if the box resizes
the frame).

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
| `vk_deck_set_shadow(deck, enabled)` | Enable or disable drop shadows on all children |
| `vk_deck_update(deck)` | Manually composite children (delegates to `_draw`) |
| `vk_deck_destroy(deck)` | Detach all children and destroy |

The deck's `_draw` sets each child's `surface` pointer to `deck->surface`
before drawing. This means children do not need a valid surface at add time —
the binding happens lazily during the screen refresh cycle. The deck can be
attached and detached from surfaces at runtime, acting as a floating overlay.

### Shadows

When shadows are enabled via `vk_deck_set_shadow(deck, TRUE)`, each child
widget casts a drop shadow — an L-shaped strip (right edge + bottom edge,
offset by 1 cell) rendered in white-on-black. The shadow reads the existing
characters from the surface and recolors them, preserving the glyph beneath.
Shadows are drawn before each widget during bottom-to-top compositing, so
higher widgets correctly occlude the shadows of lower ones.

`_recreate` (teleport) propagates `vk_widget_recreate()` to all children so
they rebuild their canvases on the new SCREEN. The deck itself has nothing
to recreate.

## Resize Propagation

When `vk_widget_resize()` successfully resizes the canvas, it emits
`VK_EVENT_ON_RESIZE`. Container klasses register their own `ON_RESIZE`
handlers in their constructors to propagate resize to children:

- **vk_frame_t** -- resizes child to `(width - 2, height - 2)`;
  resizes and repositions attached scrollers
- **vk_box_t** -- recalculates slot dimensions and resizes each child
- **vk_listbox_t** -- resizes and repositions attached scrollers, repaints
- **vk_selectbox_t** -- resizes and repositions attached scrollers, repaints
- **vk_textbox_t** -- resizes and repositions attached scrollers, reflows
  text, repaints

Propagation is recursive: resizing a box emits `ON_RESIZE`, which fires
the box's handler to resize each slot's widget, which in turn emits their
own `ON_RESIZE` events (e.g. a frame inside a box will resize its own child).

## Recreate Propagation

Canvas recreation is triggered during `vk_screen_teleport()` when all
widget canvases must be rebuilt on a new ncurses SCREEN.

`vk_widget_recreate(widget)` calls `widget->_recreate(widget)` to create
the new canvas, then emits `VK_EVENT_ON_RECREATE`.

The base `_recreate` creates a fresh `newwin` and resets `composer` to
the new canvas (clearing `VK_STATE_FROZEN`). Container klasses override
`_recreate` to propagate through the widget tree:

- **vk_frame_t** -- creates its canvas, updates scroller surfaces, recreates
  scrollers, then recreates the child
- **vk_window_t** -- same as frame (scrollers first, then child)
- **vk_box_t** -- creates its canvas, then recreates each slot's widget
- **vk_deck_t** -- propagates recreate to children (deck has no canvas to recreate)

All update child and scroller surface pointers before propagating so
widgets blit to the correct parent canvas. All custom `_recreate` methods
reset `composer = canvas` and clear `VK_STATE_FROZEN`, matching the base
behavior.

Content-bearing widgets register `ON_RECREATE` event handlers in their
constructors to repaint after canvas recreation:

- **vk_listbox_t** -- updates attached scroller surfaces, recreates them,
  then calls its `_update` to repaint items and scrollers
- **vk_selectbox_t** -- same as listbox
- **vk_textbox_t** -- updates attached scroller surfaces, recreates them,
  then calls its `_update` to repaint text and scrollers

Application code can register additional `ON_RECREATE` handlers on any
object via `vk_object_register_event()`.

## Widget State Flags

`vk_widget_t` carries a `uint32_t state` bitfield. The ctor initializes it
to `VK_STATE_VISIBLE`. State can be read with `vk_widget_get_state()` and
written with `vk_widget_set_state()`.

| Flag | Bit | Effect |
|------|-----|--------|
| `VK_STATE_VISIBLE` | `1 << 1` | Widget is drawn during refresh. Cleared = hidden. |
| `VK_STATE_FROZEN` | `1 << 3` | Widget skips drawing but remains visible in layout. |
| `VK_STATE_NORESIZE` | `1 << 7` | `vk_widget_resize()` returns -1 immediately. |
| `VK_STATE_EXPAND` | `1 << 8` | Container resizes widget to fill its slot. Off by default. |

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

When `VK_STATE_NORESIZE` is set, `vk_widget_resize()` refuses to change the
widget's dimensions and returns -1. Useful for fixed-size widgets that
should not be affected by parent layout changes.

### Expand

When a widget is placed inside a container (`vk_box_t` slot or `vk_frame_t`
child), the container only resizes it to fill the available space if
`VK_STATE_EXPAND` is set. The flag is **off** by default so widgets keep
their natural dimensions. Use `vk_widget_set_expand()` before placing the
widget in a container to opt in.

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

## Buttons

`vk_button_t` is a push button derived from `vk_widget_t`. It supports
two families of relief style: **3D** (the default) and **basic**.

Display width is measured via `wcswidth`, so UTF-8 text sizes correctly.

### 3D relief (`VK_FRAME_SINGLE`, `VK_FRAME_ASCII`)

The widget is 3 rows tall (border, centered text, border) and
`1 + display_width + 1` columns wide. The bevel is a two-tone color
split that simulates a raised look. Starting clockwise from the
bottom-left corner (inclusive), the highlight color (white on face)
covers the left edge, top-left corner, and top edge. From the top-right
corner (inclusive), the shadow color (black on face) covers the right
edge, bottom edge, and corners. On press, the highlight and shadow swap
to give a sunken appearance. `VK_FRAME_SINGLE` uses WACS_* box-drawing
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
| `vk_button_update` | Redraw the button |

Normal colors are set via `vk_widget_set_colors()`. The button is
IO-agnostic; the application calls `vk_button_press()` /
`vk_button_release()` to toggle state.

## Inputs

`vk_input_t` is a single-line text input derived from `vk_widget_t`. It
supports two relief style families matching the button widget:

### 3D Relief (`VK_FRAME_SINGLE`, `VK_FRAME_ASCII`)

The widget is 3 rows tall with a sunken border (inverted bevel from
buttons): black shadow on top/left, white highlight on bottom/right.
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
| `vk_input_update(input)` | Redraw the input |
| `vk_input_destroy(input)` | Destroy the input |

The input is IO-agnostic; the application installs a `kmio` handler that
maps keystrokes to the editing API functions.

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
`textbox->_update(textbox)`. The textbox itself is IO-agnostic.

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
`vk_selectbox_update()`). All listbox item management and navigation APIs
are available under `vk_selectbox_*` names via convenience macros.

### _update Override

The selectbox overrides `VK_LISTBOX(selectbox)->_update` to point to its
own rendering function. This ensures that internal listbox navigation
(which calls `listbox->_update(listbox)` through the function pointer)
dispatches to the selectbox's renderer, which prepends indicator glyphs.

The selectbox registers its own `ON_RESIZE` and `ON_RECREATE` event
handlers because the listbox's handlers call `_vk_listbox_update` directly
(static function, not through the function pointer). The selectbox's
handlers follow the same scroller-propagation pattern as the listbox's.

## Fillers

`vk_filler_t` is a minimal blank widget derived from `vk_widget_t`. It is
created at 1x1 with `VK_STATE_EXPAND` set, so it absorbs all available
space when placed in a container. Use it as a spacer in non-homogeneous
boxes to push other widgets apart.

| API | Description |
|-----|-------------|
| `vk_filler_create()` | Create a 1x1 filler with expand enabled |
| `vk_filler_destroy(filler)` | Destroy the filler |

## Activity Indicators

`vk_activity_t` is a 1x1 widget derived from `vk_widget_t` that animates
a spinning/cycling indicator. It supports four animation styles:

| Style | Constant | Frames |
|-------|----------|--------|
| Spinner | `VK_ACTIVITY_SPINNER` | ASCII `\|/-\` (4 frames) |
| Dots | `VK_ACTIVITY_DOTS` | Braille characters U+280B series (10 frames) |
| Circles | `VK_ACTIVITY_CIRCLES` | Quarter-circle U+25D0 series (4 frames) |
| Bar | `VK_ACTIVITY_BAR` | Growing block U+2581 series (8 frames) |

The widget uses a caller-driven tick model like `vk_marquee_t`: the
application calls `vk_activity_run()` each iteration of the event loop.
A `speed` parameter controls how many ticks elapse before advancing to
the next frame (default 1 = advance every tick).

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
- Virtual dispatch: `VK_BOX(dialog)->_update(VK_BOX(dialog))` for box layout

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

The filedialog dtor saves pointers to all children, removes them from the
container, demotes to `vk_box_t`, destroys the box, then destroys each
child widget individually (scroller, button bar, buttons, file list, input).

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
| `vk_filedialog_update(dialog)` | Update all children and composite |
| `vk_filedialog_destroy(dialog)` | Destroy dialog and all children |

## Linked Lists

The framework uses Linux kernel-style intrusive linked lists (`list.h`)
throughout:

- `vk_widget_t.list` -- widget membership in a container
- `vk_container_t.widget_list` -- children of a container
- `vk_deck_t.widget_list` -- z-ordered widget stack (head->next = top)
- `vk_listbox_t.item_list` -- items in a listbox/menu (`vk_item_t` nodes)
