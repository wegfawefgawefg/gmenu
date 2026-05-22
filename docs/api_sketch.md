# API Sketch

## Setup

```cpp
gmenu::Menu menu;
menu.set_layouts(&layouts);
menu.set_user_data(&game_state);

menu.register_screen(MainMenu, build_main_menu);
menu.register_screen(Settings, build_settings);
menu.set_root(MainMenu);
```

## Update

```cpp
gmenu::Input input = gmenu::input_from_ginput(frame, menu_actions);
menu.update(input, dt, width, height, glayout::FormFactor::Desktop);
```

## Render

```cpp
for (const gmenu::DrawItem& item : menu.draw_items()) {
    if (item.style == MainButtonStyle) {
        draw_big_button(item);
    } else {
        draw_default_widget(item);
    }
}
```

The renderer owns assets, animation state, fonts, sounds, and transitions.

The SDL3 demo renderer in `examples/sdl_demo` is an example of this loop. It is
not required by consumers of `gmenu::gmenu`.

## Screen Builder

```cpp
void build_main_menu(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    (void)ctx;
    out.id = MainMenu;
    out.layout_id = 100;
    out.default_focus = PlayButton;

    out.widgets.push_back(gmenu::label(1, "title", "Title"));
    out.widgets.push_back(
        gmenu::button(PlayButton, "play", "Play", gmenu::Action::none()));
    out.widgets.push_back(
        gmenu::button(SettingsButton, "settings", "Settings", gmenu::Action::push(Settings)));
}
```

## Commands

```cpp
gmenu::CommandId quit = menu.register_command([](gmenu::BuildContext& ctx, int) {
    auto* game = static_cast<Game*>(ctx.user);
    game->quit = true;
});
```

Use commands for game-specific work. Use built-in actions for menu flow.

## Value Widgets

```cpp
out.widgets.push_back(gmenu::toggle(20, "fullscreen", "Fullscreen", settings.fullscreen));
out.widgets.push_back(
    gmenu::slider_1d(21, "volume", "Volume", settings.volume, 0.0f, 1.0f, 0.1f));
out.widgets.push_back(gmenu::option_cycle(22, "quality", "Quality", settings.quality,
                                          {"low", "medium", "high"}));
out.widgets.push_back(gmenu::text_input(23, "name", "Profile", profile.name, 32));
```

`DrawItem::value` contains the current display value for these widgets.

## Text Input

```cpp
gmenu::Input input;
input.text = sdl_text_event_text;
input.backspace = backspace_pressed;
menu.update(input, dt, width, height);
```

Text input remains backend-neutral. `gmenu` stores edit focus and modifies the
bound string. The renderer decides how to draw carets and selection effects.
