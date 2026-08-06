# Plan: Universal scroller apply/nudge + ON_SCROLL (libvdk 7.0)

**Branch:** `feat/scroller-apply-7.0`  
**Version when done:** **7.0.0** (major: new public protocol; SOVERSION 7)  
**Depends on:** `vk_listbox_set_scroll_pos` already on this branch (6.1.2 work)

## Goals

1. Scroller stays **paint chrome** for the thumb (`VkScrollInfoFunc` read path).
2. Add a **write path**: optional apply callback + `vk_scroller_nudge()`.
3. After a successful position change, emit **`VK_EVENT_ON_SCROLL`** on the **scroll source** object so clients can register optional handlers (`vk_object_register_event`).
4. Listbox / textbox / viewport remain the owners of scroll state; they implement apply (or stock helpers).
5. Correctness over speed; small reviewable tasks.

## Non-goals (this release)

- Changing Manage Apps wheel policy (selection move) unless separately requested.
- Making scroller own content scroll state.
- Automatic wheel binding inside libvdk (apps still deliver BUTTON4/5); nudge is the shared primitive they call.

## API design (freeze)

### Existing (unchanged)

```c
typedef void (*VkScrollInfoFunc)(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x);
```

### New

```c
/* Absolute apply: set scroll position on the content source.
   Implementor clamps and mutates; return 0 if position changed,
   1 if unchanged (no-op), -1 on error.
   On change, implementor MUST emit VK_EVENT_ON_SCROLL on the source
   object before returning 0. */
typedef int (*VkScrollApplyFunc)(vk_widget_t *source,
    int scroll_y, int scroll_x);
```

```c
int vk_scroller_set_scroll_apply(vk_scroller_t *scroller,
    VkScrollApplyFunc func);

/* Nudge by deltas in content units (list rows, text lines, etc.).
   Vertical scroller uses dy; horizontal uses dx; the other axis is 0
   unless both bars share one source (then both may be non-zero).
   Returns: 0 changed, 1 no change, -1 error.
   Does not paint; caller updates source + scroller. */
int vk_scroller_nudge(vk_scroller_t *scroller, int dy, int dx);
```

### Nudge algorithm

1. Require scroller, `scroll_source`, `scroll_info_func`, `scroll_apply_func`.
2. Call info → content_h, content_w, scroll_y, scroll_x.
3. Visible size: from **host** widget metrics (vertical: host height; horizontal: host width). Match thumb math in `_vk_scroller_draw_scrollbar` (track uses host size).
4. `new_y = scroll_y + dy`, `new_x = scroll_x + dx`.
5. Clamp:  
   - `max_y = max(0, content_h - visible_h)`  
   - `max_x = max(0, content_w - visible_w)`  
   - clamp new_y/new_x into [0, max_*]
6. If new equals old, return 1 (no apply).
7. Call `apply(source, new_y, new_x)`; return its result.

### ON_SCROLL contract

| Who emits | When |
|-----------|------|
| Content implementors of apply / direct scroll APIs | Position actually changes |
| `vk_scroller_nudge` | Does **not** emit itself — apply (or set_scroll_pos path) emits |
| Listbox `set_scroll_pos` | Emit if pos changed |
| Textbox scroll_* | Already emits (keep) |
| Viewport set_scroll / scroll_by | Emit if pos changed after clamp |

Emit target: **`VK_OBJECT(source)`** (the content widget), not the scroller.

Do not emit on pure selection change (`ON_SELECT` only).

### Stock helpers (public, optional convenience)

```c
int vk_listbox_scroll_apply(vk_widget_t *source, int scroll_y, int scroll_x);
int vk_textbox_scroll_apply(vk_widget_t *source, int scroll_y, int scroll_x);
int vk_viewport_scroll_apply(vk_widget_t *source, int scroll_y, int scroll_x);
```

Each: cast, set position via existing API, emit ON_SCROLL if changed, return 0/1/-1.

Wire in demos:  
`vk_scroller_set_scroll_apply(scr, vk_listbox_scroll_apply);`

### Consumer example (vwm menu)

```c
// at Apps dropdown build (already has scroller):
vk_scroller_set_scroll_apply(scroller, vk_listbox_scroll_apply);

// wheel:
vk_scroller_nudge(scroller, BUTTON4 ? -1 : +1, 0);
vk_listbox_update(listbox);
vk_window_update(menu);
// optional: register ON_SCROLL if extra UI sync needed
```

Need a way for mainmenu to find the attached scroller: `VK_WIDGET(listbox)->vscroller` is in public widget layout via attach, or store pointer at create. Prefer `vk_widget_get_vscroller` if missing — **only add getter if no clean access**; checking: host stores `vscroller` on widget struct but may not have public getter. Task: add `vk_widget_get_vscroller` / `get_hscroller` if needed for encapsulation (or keep apps holding the scroller pointer — Apps menu already has local `scroller` at create; wheel path only has listbox — **must get scroller from listbox**).

Add:
```c
vk_scroller_t *vk_widget_get_vscroller(vk_widget_t *widget);
vk_scroller_t *vk_widget_get_hscroller(vk_widget_t *widget);
```

## Version / packaging

- `PROJECT_VERSION` → `7.0.0`
- `SOVERSION` → `7` (breaking public ABI: new symbols + scroller struct growth)
- CHANGELOG: 7.0.0 section documenting protocol
- Any README/KLASSES notes if present

## Task breakdown (qwen parcels — small, sequential)

| ID | Scope | Done when |
|----|--------|-----------|
| T1 | API surface only: typedef, scroller struct field, set_scroll_apply, get_v/hscroller decls in vdk.h + headers; init field NULL in ctor | Compiles (may be unused) |
| T2 | Implement `vk_scroller_set_scroll_apply` + `vk_scroller_nudge` + getters | Unit-level: build; nudge no-ops without apply |
| T3 | `vk_listbox_set_scroll_pos` emit ON_SCROLL on change; `vk_listbox_scroll_apply` | Build |
| T4 | `vk_textbox_scroll_apply` (reuse existing scroll setters; ensure emit) | Build |
| T5 | Viewport: emit ON_SCROLL on real change; `vk_viewport_scroll_apply` | Build |
| T6 | Demo: wire listbox/textbox scrollers with apply; document in CHANGELOG draft | Demo builds |
| T7 | Version 7.0.0 + SOVERSION 7 + CHANGELOG finalize | Release notes accurate |
| T8 | vwm (separate branch): menu wheel uses `vk_scroller_nudge` + apply at build | vwm builds against local libvdk 7 |

Orchestrator reviews each task before starting the next. No hurry.

## Testing checklist

- [ ] Nudge without apply → -1, no crash  
- [ ] Listbox: nudge pans view, curr_item unchanged, ON_SCROLL fires once per real change  
- [ ] Listbox: set_next still ensure-visible + ON_SELECT, not spurious ON_SCROLL  
- [ ] Textbox: existing demo ON_SCROLL still works; apply path consistent  
- [ ] Viewport: scroll_by emits ON_SCROLL  
- [ ] vwm Apps menu wheel pans without moving highlight  
- [ ] Scroller thumb tracks after nudge + update  

## Out of tree

- Do not commit `.swp`, demos build artifacts, untracked junk unless asked.
