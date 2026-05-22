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
- mouse hover, click, and basic slider mouse setting
- visual state for focus, hover, press, disabled, and text editing
- draw items with rects, labels, values, style ids, and nav metadata
- nav override storage, save/load, dirty state, and validation
- optional ImGui menu metadata and nav editor panels
- data-backed list, paged-list, profile-list, settings, and binds screens

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
  Gubsy avoids immediate focus changes from stale mouse position while keyboard
  or gamepad navigation is being used. `gmenu` currently focuses hovered widgets
  whenever the mouse is over them and not pressed.

- Rich slider mouse behavior.
  Gubsy separates track drag preview from commit, supports dragging while held,
  and commits on release. `gmenu` has simpler click-to-set behavior.

- Option-cycle hit regions.
  Gubsy can distinguish left button, right button, value area, and embedded text
  input regions inside an option widget. `gmenu` currently treats the widget
  more coarsely.

- Text edit commit behavior.
  `gmenu` now has `Widget::on_commit` for modified text input commit. This needs
  to be exercised in more real screens as composed-row work proceeds.

- Rejection and movement feedback.
  Gubsy plays focus, confirm, cant, left, and right sounds from menu behavior.
  `gmenu` now reports this through optional feedback callbacks, but richer
  slider/option-cycle interaction will add more cases.

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
  such as auxiliary text input. In `gmenu`, this should probably become multiple
  widgets assigned to multiple layout slots, or an explicit composed-row helper
  that still emits normal widgets.

- Gubsy-specific rows can be rebuilt in gubsy.
  If gubsy needs a row that looks like one complex control, gubsy can build it
  from several `gmenu` widgets and render them as one visual row. The core
  library should only need enough support for focus, nav, hit testing, and
  feedback across those sub-widgets.

- Auxiliary text input.
  Do not add `aux_text_buffer` directly to the core widget unless we prove one
  widget really needs two text edit states. Prefer sub-widgets first.

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

## Editor And Debug Gaps

These are not blockers for runtime menus, but they matter for gubsy-style live
editing.

- The ImGui nav editor is functional but not yet as comfortable as gubsy's
  editor. It needs better screen selection, selected-widget inspection, and
  clearer validation display.

- The combined menu editor should present layout editing and nav editing as one
  tool, while still delegating rectangle edits to `glayout`.

- Layout persistence is library-ready but demo-level. A real gubsy integration
  needs host-owned save paths and save commands wired through cleanly.

- Binds debug belongs in `ginput` or the host app, not in `gmenu`.

- Video/resolution debug belongs in the renderer/config layer, not in `gmenu`.

## Likely Next Implementation Order

1. Extend feedback coverage as richer interactions are added.
   The core feedback API exists for focus moved, activated, rejected, adjusted
   left/right, text edit started, and text edit ended. Keep app mutations in
   command callbacks.

2. Add mouse focus lock/unlock.
   Keep behavior explicit and easy to debug.

3. Rework slider and option-cycle interaction.
   Preserve simple use, but expose enough draw/event state for gubsy-style
   renderers.

4. Add composed-row helpers.
   Generate multiple ordinary widgets for rows that need multiple controls.

5. Improve ImGui nav/menu editor.
   Add screen picker, active widget inspector, and better validation.

6. Wire a gubsy-like demo screen.
   Prove profile, settings, binds, and mixed sub-widget rows can be rebuilt
   without adding gubsy-specific fields to core widgets.
