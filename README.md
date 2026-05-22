<p align="center">
  <img src="assets/logo.svg" width="96" alt="gmenu logo">
</p>

# gmenu

`gmenu` is a small C++20 game menu library. It owns menu behavior: screens, focus,
navigation, widgets, commands, and renderable menu state.

It assumes `glayout` for authored layout rectangles and `ginput` for input state.
It does not own SDL, rendering, audio, asset loading, or animation systems.

## Scope

`gmenu` provides:

- screen root, push, pop, replace, and clear operations
- label, button, card, toggle, slider, option-cycle, and text-input widget data
- focus movement and mouse hit testing
- command callbacks
- data-backed basic, list, and paged-list screen builders
- `ginput` bind-action list screen builder
- typed settings screen builder
- paged profile list screen builder
- draw/view items with rects, widget state, labels, values, and style ids
- explicit nav override storage for editor-authored focus links
- optional `gmenu::imgui` helpers for nav editing/debugging
- a small `ginput::FrameState` to `gmenu::Input` adapter

Rendering is done by the host program. The host can draw plain rectangles,
texture-backed buttons, animated panels, particle effects, or any other style
from the same `DrawItem` data.

The repository includes an SDL3 demo renderer, but SDL3 is not part of the core
library target.

## Add To A Project

Place `gmenu`, `glayout`, `ginput`, and `gsexp` next to each other:

```text
external/
  gsexp/
  glayout/
  ginput/
  gmenu/
```

Then add `gmenu` from CMake:

```cmake
add_subdirectory(external/gmenu)
target_link_libraries(my_game PRIVATE gmenu::gmenu)
```

`gmenu` will use sibling `../glayout` and `../ginput` by default when their CMake
targets are not already available.

## Basic Use

```cpp
gmenu::Menu menu;
menu.set_layout_store(&layout_store);
menu.set_user_data(&game);

menu.register_screen(MainMenu, build_main_menu);
menu.set_root(MainMenu);

gmenu::Input input = gmenu::input_from_ginput(frame, menu_action_map);
menu.update(input, dt, width, height, glayout::FormFactor::Desktop);

for (const gmenu::DrawItem& item : menu.draw_items()) {
    draw_menu_item(item);
}
```

Screen builders produce widgets. They should not draw.

```cpp
void build_main_menu(gmenu::BuildContext&, gmenu::Screen& out) {
    out.id = MainMenu;
    out.layout_id = 100;
    out.default_focus = PlayButton;
    out.widgets.push_back(gmenu::button(PlayButton, "play", "Play", gmenu::Action::none()));
    out.widgets.push_back(
        gmenu::button(SettingsButton, "settings", "Settings", gmenu::Action::push(Settings)));
}
```

Simple screens can be registered as data instead of hand-written builders:

```cpp
gmenu::ListScreenDef main;
main.id = MainMenu;
main.layout_id = 100;
main.title_id = 1;
main.title = "Main";
main.default_focus = 10;
main.items.push_back({10, "play", "Play", "", gmenu::Action::none()});
main.items.push_back({11, "settings", "Settings", "", gmenu::Action::push(Settings)});

gmenu::register_list_screen(menu, main);
```

Screen definition objects passed to `register_basic_screen` and
`register_list_screen` must outlive the `Menu`.

`DrawItem::style` is only a stable id. The renderer owns textures, fonts,
animation state, sounds, and transitions.

Explicit navigation links can be authored separately from transient screen
builders:

```cpp
menu.set_nav_link(MainMenu, PlayButton, gmenu::NavDirection::Down, SettingsButton);
menu.clear_nav_link(MainMenu, PlayButton, gmenu::NavDirection::Down);
```

The links are applied after a screen is built. Runtime navigation only follows
authored links. If no link exists for a direction, that direction does nothing.
`DrawItem` reports both the effective nav target and whether it came from a
saved override or an explicit widget link.

Nav overrides can be saved as S-expressions:

```cpp
menu.save_nav_file("menu_nav.lisp");
menu.mark_nav_saved();
menu.load_nav_file("menu_nav.lisp");
```

Optional ImGui helpers are isolated in `gmenu::imgui`:

```cmake
set(GMENU_WITH_IMGUI ON CACHE BOOL "" FORCE)
set(GMENU_IMGUI_TARGET imgui CACHE STRING "" FORCE)
add_subdirectory(third_party/gmenu)
target_link_libraries(my_game PRIVATE gmenu::gmenu gmenu::imgui)
```

The SDL demo also enables ImGui debug windows when `GMENU_WITH_IMGUI=ON` and
`GMENU_IMGUI_SOURCE_DIR` points at a Dear ImGui checkout with the SDL3 backend
sources. The demo exposes menu metadata, layout metadata, the `glayout` editor,
and the `gmenu` nav editor. The core `gmenu::gmenu` target does not depend on
ImGui.

Text input is backend-neutral. Put SDL text events, key-repeat backspace events,
or another backend's text feed into `gmenu::Input::text` and `backspace`.

Paged lists cover common profile/save/mod/server/binds pages:

```cpp
int page = 0;
gmenu::PagedListScreenDef saves;
saves.id = Saves;
saves.layout_id = 100;
saves.title = "Saves";
saves.title_id = 1;
saves.page = &page;
saves.items_per_page = 4;
saves.item_slots = {"row0", "row1", "row2", "row3"};
saves.prev_id = 20;
saves.next_id = 21;
saves.page_label_id = 22;
saves.items.push_back({30, "", "Save 1", "forest", gmenu::Action::none()});

gmenu::register_paged_list_screen(menu, saves);
```

`gmenu` also includes `ginput` bind screens. The overview screen lists schema
actions and bind counts. The edit screen lists current button binds, can remove
them directly, and dispatches an add/capture command with the action id as
payload. Actual device capture is still host-owned.

Typed settings and profile screens are data-backed too. Settings items bind
directly to caller-owned bool, float, option-index, or string values. Profile
lists page over caller-owned profile entries and can either set a selected id or
dispatch a selection command.

## Demo

```sh
./scripts/build.sh
./scripts/run.sh
./scripts/run_sdl_demo.sh
./scripts/run_sdl_demo_imgui.sh
```

VS Code F5 runs the ImGui-enabled SDL3 demo when SDL3 and the local Dear ImGui
source checkout are available. The plain SDL launch configuration is still
available as `gmenu SDL demo`.

Demo controls:

- `Ctrl+L`: edit `glayout` slots. Drag rectangles; use `Z`/`Y` for undo/redo,
  `C`/`V` for copy/paste, and delete/backspace to remove a slot.
- `Ctrl+N`: edit `gmenu` navigation. Click a source widget, press an arrow for the
  direction, then click a target widget. Backspace clears the armed direction;
  delete clears links for the selected source. `S` saves nav overrides to
  `gmenu_nav_demo.lisp`; `L` reloads them.
- `F10`: toggle ImGui debug windows in ImGui-enabled demo builds. `F9` toggles
  the small debug bar.
