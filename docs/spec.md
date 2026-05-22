# gmenu Specification

## Goal

`gmenu` is an importable game menu library. It should make common game menus easy
to build without pulling in all of Gubsy.

The library should be direct, explicit, and easy to debug. It should avoid deep
OOP hierarchies, hidden renderer dependencies, and broad generic UI-framework
behavior.

## Dependencies

`gmenu` assumes:

- `glayout` for authored layout rectangles
- `ginput` for input actions, button edges, repeat helpers, and frame state

`gmenu` may use `gconfig` later for reusable settings/profile/bind screens. The
base menu runtime should not require config storage.

## Responsibilities

`gmenu` owns:

- screen registration
- root, push, pop, replace, and clear transitions
- widget definitions and widget builders
- focus state
- directional navigation
- mouse hover and rectangular click hit testing
- command callbacks
- text edit state
- simple value widgets such as toggles and sliders
- conversion from menu state to renderable `DrawItem` values

## Non-Responsibilities

`gmenu` does not own:

- SDL windows or events
- rendering
- texture loading
- font loading
- audio playback
- animation graphs
- particle effects
- game save policy
- asset databases
- mod/server browser semantics

The host renders `DrawItem` however it wants.

## Rendering Model

`gmenu` does not draw. Every frame, it exposes `DrawItem` values:

- widget id
- widget type
- rect from `glayout`
- style id
- label/value text
- focused, hovered, pressed, disabled, and editing state
- time-in-state fields for renderer-owned animation

The renderer owns visual interpretation. A focused button can be a plain color,
sprite swap, fade, slide, neon flicker, or anything else.

## Styling

Core style data should stay minimal. `StyleId` is the stable hook between menu
logic and host rendering.

Do not put renderer-specific assets, animation timers, easing curves, or texture
handles into core widget style. If a renderer wants that, it should map style ids
to its own theme data.

## Layout

Widgets reference layout slots by label. `gmenu` asks `glayout` for the best
layout for the current size and form factor, then maps each widget slot to a
screen rect.

Initial hit testing is rectangular. Custom/polygon hit testing is out of scope
for the first version.

## Input

`gmenu::Input` is backend-neutral. Hosts may fill it manually, or use
`input_from_ginput` with a `ginput::FrameState`.

Standard menu actions are:

- up
- down
- left
- right
- select
- back
- page previous
- page next

## Screen Flow

The menu has a stack because Gubsy already uses one and drill-down menus need it.

Required operations:

- `set_root(screen)`: clear and open a base screen
- `push(screen)`: open a child/details/modal screen
- `pop()`: close current screen
- `replace(screen)`: swap current screen
- `clear()`: close all screens

Only the top screen is active.

## Gubsy Notes

Gubsy's current menu system is a useful feature checklist, not an implementation
to copy directly. The extraction should avoid:

- `EngineState` coupling in core
- SDL2 renderer coupling in core
- audio calls inside menu logic
- hardcoded Gubsy screen ids
- static vectors inside screen builders
- layout editor checks inside menu update

## Future Reusable Screens

After the core is stable, `gmenu` should grow common game screens:

- main menu
- settings hub
- user profile picker/editor
- input profile picker/editor
- bind editor
- save slot picker

These screens can assume `gconfig` if needed.
