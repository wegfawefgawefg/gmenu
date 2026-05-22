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

`gmenu` does not depend on `gconfig`. Reusable settings/profile/bind screens
work on caller-owned data. Persisting that data belongs to the host or `gconfig`.

## Responsibilities

`gmenu` owns:

- screen registration
- root, push, pop, replace, and clear transitions
- widget definitions and widget builders
- data-backed basic and list screen builders
- focus state
- directional navigation
- mouse hover and rectangular click hit testing
- command callbacks
- text edit state
- simple value widgets such as toggles and sliders
- option-cycle values
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

The repo may contain renderer demos. Those demos must not add renderer
dependencies to `gmenu::gmenu`.

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

Held directional inputs are preserved as held inputs so `gmenu` repeat behavior
can work. Select/back/page inputs are edge-triggered by the adapter.

Standard menu actions are:

- up
- down
- left
- right
- select
- back
- page previous
- page next

Text input is explicit data on `gmenu::Input`. Backends should append printable
text to `Input::text` and set `Input::backspace` for deletion.

## Widgets

Initial widget set:

- `Label`: non-selectable text
- `Button`: selectable action
- `Card`: selectable button with secondary text
- `Toggle`: selectable bool value
- `Slider1D`: selectable float value adjusted by left/right/select
- `OptionCycle`: selectable integer index adjusted by left/right/select
- `TextInput`: selectable string value that enters editing on select

Widgets may provide explicit navigation ids. If no explicit target exists,
navigation falls back to selectable widget order.

## Reusable Screen Builders

`gmenu` includes low-opinion screen builders for common pages:

- `BasicScreenDef`: title plus explicit widgets
- `ListScreenDef`: title plus button/card list items
- `PagedListScreenDef`: title plus list items split across pages with optional
  page label and prev/next/back widgets
- `SettingsScreenDef`: typed settings rows for toggles, sliders, option cycles,
  and text input
- `ProfileListScreenDef`: paged profile picker with selected-profile marker or
  selection command
- `BindActionListScreenDef`: `ginput` action list with bind counts and an edit
  command payload
- `BindActionEditScreenDef`: current button binds for one action, direct remove
  actions, and an add/capture command payload

These cover main menus, profile pickers, save-slot pickers, simple settings
pages, and many Gubsy/Splonks-style drill-down pages. More specialized screens
can still use normal `ScreenBuildFn` functions.

Data-backed screen definitions are stored by pointer. They must outlive the
`Menu` that registers them. This keeps ownership explicit and avoids hidden
allocation or virtual object lifetimes.

Paged-list page state is owned by the caller through an `int*`. Prev/next
buttons use normal `Action::delta_int` actions.

Settings values and profile lists are caller-owned. `gmenu` mutates simple bound
values directly, but it does not persist them. Save/load policy belongs to
`gconfig` or the host game.

The bind screens intentionally do not capture devices. They get the user to the
action that should be edited, remove existing button binds, and dispatch a host
command with the `ginput::ActionId` as payload when a new bind should be
captured. The host can open a capture screen appropriate to its backend.

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

## Remaining Host Policy

The current reusable screens cover main/list pages, paged save/profile/mod-style
lists, typed settings pages, profile pickers, and input-bind overview/edit
pages. The host still owns policy-heavy behavior:

- saving settings/profile changes to disk
- creating/deleting profiles
- creating/deleting save slots
- capturing physical input devices for new binds
- drawing, sounds, transitions, and animation

Those behaviors can be layered on top with commands and caller-owned data.
