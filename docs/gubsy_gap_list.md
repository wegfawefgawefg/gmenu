# Gubsy Gap List

This document tracks what `gmenu` still needs before gubsy can be rebuilt on
top of the ripped-out libraries without copying the old menu spaghetti.

The goal is capability parity, not field-for-field parity. If a gubsy feature
was mostly render decoration, it should usually stay out of core widget
behavior.

## Current Coverage

`gmenu` already covers these gubsy menu responsibilities:

- screen registration, root, push, pop, replace, and clear
- focus state and explicit directional navigation
- buttons, labels, cards, toggles, sliders, option cycles, and text input
- command callbacks with integer payloads
- internal feedback events flushed to optional callbacks after update
- page previous and page next inputs
- mouse hover, click, slider drag preview/commit, and option-cycle hit regions
- mouse focus lock/unlock after keyboard/gamepad navigation
- visual state for focus, hover, press, disabled, and text editing
- draw items with rects, labels, values, style ids, and nav metadata
- nav override storage, save/load, dirty state, and validation
- optional ImGui menu metadata and nav editor panels
- registered-screen metadata for debug/editor screen pickers
- data-backed list, paged-list, profile-list, settings, and binds screens
- composed-row helper that emits normal widgets and nav links

`glayout` already covers these gubsy layout responsibilities:

- layout pool and closest layout selection
- resolution and form-factor variants
- slot/object add, delete, duplicate through copy/paste, rename, move, and resize
- multiselect and group move
- grid snapping, undo, redo, dirty state, and save requests
- SDL overlay draw data and optional ImGui editor/browser panels

## Core Interaction Gaps

These belong in `gmenu` because they affect behavior, not just drawing.

- Mouse focus lock/unlock behavior.
  `gmenu` now avoids immediate focus changes from stale mouse position while
  keyboard or gamepad navigation is being used. Hover focus unlocks when the
  mouse moves or clicks.

- Rich slider mouse behavior.
  `gmenu` separates track drag preview from commit, supports dragging while
  held, and commits through `Widget::on_commit` on release.

- Option-cycle hit regions.
  `gmenu` exposes left button, right button, and value hit rects through
  `DrawItem::controls`. Embedded text input should still be modeled as a
  separate widget or composed-row helper, not as another field on option-cycle.

- Text edit commit behavior.
  `gmenu` has `Widget::on_commit` for modified text input commit. The SDL demo
  uses it for profile-name and composed resolution text fields.

- Rejection and movement feedback.
  Gubsy plays focus, confirm, cant, left, and right sounds from menu behavior.
  `gmenu` reports this through optional feedback callbacks, including richer
  slider and option-cycle adjustment cases.

- Per-widget activation policy.
  Gubsy has `select_enters_text` and `play_select_sound`. `select_enters_text`
  belongs in `gmenu`; sound policy should be host-side feedback handling rather
  than a widget field.

## Commands And Feedback

`gmenu` should keep commands and feedback separate.

- Commands are explicit widget actions.
  Commands are direct callbacks and may perform real app work: push screens,
  save settings, rename profiles, start gameplay, change engine state, or write
  files.

- Feedback is generic menu-mechanic notification.
  Feedback covers things like focus moved, rejected input, activated widget,
  adjusted left/right, text edit started, and text edit ended. Feedback is for
  sounds, debug overlays, telemetry, and light UI reactions.

- Feedback should use internal events and optional callbacks.
  During `Menu::update`, record small internal feedback events. After the update
  logic has finished, flush those events through optional feedback callbacks.
  This gives callback ergonomics without firing host code from the middle of
  focus/nav/text/slider logic.

- Do not route commands through feedback.
  A profile save, settings write, screen transition, or game action should be a
  command/action, not a feedback event. Text commit should be an explicit widget
  action such as `on_commit`; text edit start/end can still be feedback.

Proposed callback shape:

```cpp
struct FeedbackHooks {
    void* user = nullptr;
    void (*focus_moved)(void* user, WidgetId from, WidgetId to) = nullptr;
    void (*rejected)(void* user, WidgetId widget) = nullptr;
    void (*activated)(void* user, WidgetId widget) = nullptr;
    void (*adjusted_left)(void* user, WidgetId widget) = nullptr;
    void (*adjusted_right)(void* user, WidgetId widget) = nullptr;
    void (*text_edit_started)(void* user, WidgetId widget) = nullptr;
    void (*text_edit_ended)(void* user, WidgetId widget) = nullptr;
};
```

## Widget Model Gaps

These need design before implementation. The old gubsy shape worked, but it
mixed visual decoration, sub-controls, and behavior into one large widget
record.

- Avoid super compound widgets.
  Do not recreate the gubsy pattern where one widget record accumulates badges,
  tertiary overlays, multiple editable buffers, special hit regions, and
  per-case behavior flags. That made individual rows work, but it made the core
  widget model harder to reason about and harder to reuse.

- Sub-widgets or composed rows.
  Gubsy sometimes put multiple editable or clickable regions inside one widget,
  such as auxiliary text input. `gmenu` now provides `append_composed_row` for
  this case. It emits normal widgets assigned to normal layout slots and wires
  missing horizontal/up/down nav.

- Gubsy-specific rows can be rebuilt in gubsy.
  If gubsy needs a row that looks like one complex control, gubsy can build it
  from several `gmenu` widgets and render them as one visual row. The core
  library should only need enough support for focus, nav, hit testing, and
  feedback across those sub-widgets.

- Auxiliary text input.
  Do not add `aux_text_buffer` directly to the core widget unless we prove one
  widget really needs two text edit states. Prefer composed rows first.

- Discrete slider options.
  Decide whether this is an option-cycle, a slider with step buttons, or a
  composed row. Avoid adding a vague `has_discrete_options` flag until the
  behavior is clear.

- Placeholder text.
  This is harmless display metadata for text inputs, but it may belong in
  renderer-side metadata unless common helpers need it.

## Renderer Metadata Gaps

These are useful for rebuilding gubsy visuals, but they should not change core
interaction. Prefer renderer/app data keyed by `WidgetId` or `StyleId` unless
the field is common enough to justify being in `DrawItem`.

- tertiary text
- tertiary overlay text
- badge text
- badge color
- custom value formatting
- slider display scale, offset, and precision
- show/hide slider track
- texture, font, animation, and transition data

Current recommendation: keep `gmenu::Widget` and `gmenu::DrawItem` boring.
Expose ids, rects, state, labels, values, and feedback hooks. Let the game
renderer own rich presentation data.

`DrawItem::controls` is interaction geometry, not renderer style. It exists so
the renderer and input code can agree on ordinary sub-regions such as slider
tracks and option-cycle left/right/value regions without adding gubsy-specific
compound widget fields.

## Editor And Debug Gaps

These are not blockers for runtime menus, but they matter for gubsy-style live
editing.

- The ImGui nav editor is functional but not yet as comfortable as gubsy's
  editor. It now has selected-widget inspection, selectable effective-nav rows,
  clearer validation tables, and a registered-screen picker that can jump the
  menu root to another screen. It can also request nav override save/load while
  leaving file ownership to the host.

- The combined menu editor should present layout editing and nav editing as one
  tool, while still delegating rectangle edits to `glayout`. The ImGui demo now
  has a combined editor window with menu/layout/nav tabs; the layout tab opens
  the `glayout` editor and the nav tab embeds the `gmenu` nav editor panel.

- Layout persistence is library-ready but demo-level. A real gubsy integration
  needs host-owned save paths and save commands wired through cleanly.

- Binds debug belongs in `ginput` or the host app, not in `gmenu`.

- Video/resolution debug belongs in the renderer/config layer, not in `gmenu`.

## Likely Next Implementation Order

1. Extend feedback coverage as richer interactions are added.
   The core feedback API exists for focus moved, activated, rejected, adjusted
   left/right, text edit started, and text edit ended. Keep app mutations in
   command callbacks.

2. Keep mouse focus lock/unlock covered while changing input behavior.
   This is implemented for keyboard/gamepad navigation and should remain covered
   as richer pointer interactions are added.

3. Keep slider and option-cycle interaction covered.
   Slider drag preview/commit and option-cycle left/right/value regions are now
   implemented. Preserve this behavior while composed rows and richer demos are
   added.

4. Keep composed-row helpers covered.
   `append_composed_row` exists for rows that need multiple controls. Use it in
   richer demo/gubsy-style screens before adding new widget fields.

5. Keep improving ImGui nav/menu editor.
   Selected-widget inspection, clearer validation, and registered-screen root
   jumping are implemented. The demo has a combined editor shell with
   menu/layout/nav tabs. Keep improving it as the demo grows.

6. Wire a gubsy-like demo screen.
   The SDL demo covers profile editing/picking, settings, input binds, and a
   mixed composed resolution row without adding gubsy-specific fields to core
   widgets. Keep expanding this as real gubsy migration finds missing cases.
