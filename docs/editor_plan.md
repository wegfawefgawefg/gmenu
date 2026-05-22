# Editor Split Plan

`gmenu` should provide the game menu editor experience, but it should not own
generic rectangle editing. The editor should feel like one tool to a game, while
the implementation stays split by responsibility.

## Library Boundaries

### glayout

`glayout` owns layout slot editing.

- Layout pool management.
- Layout variants by resolution and form factor.
- Slot/object add, remove, duplicate, rename.
- Slot/object move and resize.
- Multi-select and group move.
- Grid and snap behavior.
- Copy, paste, delete.
- Undo and redo.
- Dirty/save state for layout files.
- Editor overlay data for rectangles and handles.
- Optional ImGui panel for layout editing.

This is reusable outside menus. HUDs and in-game UI can use it without pulling
in menu focus or actions.

### gmenu

`gmenu` owns menu semantics.

- Screens and screen stack.
- Widgets and widget draw data.
- Focus and input behavior.
- Actions and command dispatch.
- Explicit widget navigation links.
- Menu debug overlay data.
- Nav graph editor.
- Optional editor integration that combines menu data with `glayout`.

`gmenu` can expose one menu-editor UX, but its layout tab should use the
`glayout` editor internally.

### game or engine layer

The consuming game owns policy.

- Which menus exist.
- What actions do.
- Profile, save, mod, lobby, and settings semantics.
- Persistence location and project-specific file layout.
- Rendering style, assets, fonts, animation, and sound.

## Current State

- `gmenu` runtime can build screens from C++ screen definitions/builders.
- `gmenu` widgets refer to `glayout` slots by label.
- `gmenu` `DrawItem` exposes widget ids, rectangles, state, and explicit nav ids.
- The SDL demo has a basic F1 `glayout` core edit overlay for one in-memory layout.
- The SDL demo has an F2 read-only nav overlay.

This is useful for debugging, but it is not the gubsy editor yet.

## Next Work

1. Finish the reusable `glayout` slot editor.
   - Port the useful gubsy layout-editor behavior into `glayout`.
   - Keep it independent from menus and SDL/ImGui where possible.
   - Provide draw data and optional ImGui helpers.

2. Add `gmenu` nav override storage.
   - Store explicit nav links outside transient rebuilt widgets.
   - Key by screen id plus widget id or stable widget key.
   - Apply overrides after a screen is built.
   - Keep C++ screen builders usable.

3. Add `gmenu` nav editor state.
   - Select source widget.
   - Arm up/down/left/right.
   - Pick target widget.
   - Clear links.
   - Draw directional lines and labels.
   - Mark nav data dirty.

4. Add a combined menu editor surface.
   - Screen picker.
   - Active layout/variant picker.
   - Layout edit tab backed by `glayout`.
   - Nav edit tab backed by `gmenu`.
   - Widget debug/properties tab.

5. Wire gubsy against the library.
   - Keep gubsy menu builders and policies in gubsy.
   - Use `gmenu` for runtime menu behavior.
   - Use `glayout` data for slot placement.
   - Use `gmenu` nav overrides for focus graph editing.

## Important Constraint

Current C++ screen builders are fine. We do not need a full persistent widget
document to get gubsy-style layout editing. The layout editor edits slots, and
widgets bind to those slots.

A persistent menu document may still be useful later for fully data-authored
menus, but it is not required for the first real editor pass.
