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

Or register simple data-backed screens:

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

The screen definition must outlive the menu.

Debug tools can inspect registered screens without taking ownership:

```cpp
for (const gmenu::ScreenDef& screen : menu.registered_screens()) {
    show_screen(screen.id);
}
```

Paged lists use caller-owned page state:

```cpp
int page = 0;
gmenu::PagedListScreenDef profiles;
profiles.id = Profiles;
profiles.layout_id = 100;
profiles.title_id = 1;
profiles.title = "Profiles";
profiles.page = &page;
profiles.items_per_page = 4;
profiles.item_slots = {"row0", "row1", "row2", "row3"};
profiles.prev_id = 20;
profiles.next_id = 21;
profiles.page_label_id = 22;
profiles.items.push_back({30, "", "Default", "last used", gmenu::Action::none()});

gmenu::register_paged_list_screen(menu, profiles);
```

## Bind Action Screen

```cpp
int binds_page = 0;
gmenu::CommandId edit_bind = menu.register_command(open_bind_capture);

gmenu::BindActionListScreenDef binds;
binds.id = Binds;
binds.layout_id = 100;
binds.title_id = 1;
binds.title = "Binds";
binds.schema = &schema;
binds.profile = &input_profile;
binds.page = &binds_page;
binds.items_per_page = 6;
binds.item_slots = {"row0", "row1", "row2", "row3", "row4", "row5"};
binds.edit_command = edit_bind;

gmenu::register_bind_action_list_screen(menu, binds);
```

Selecting an action runs `edit_bind` with the `ginput::ActionId` as payload.
Device capture remains host-owned.

The edit screen lists existing button binds and can remove them:

```cpp
ginput::ActionId edited_action = Jump;
int edit_page = 0;
gmenu::CommandId add_bind = menu.register_command(open_capture_screen);

gmenu::BindActionEditScreenDef edit;
edit.id = BindEdit;
edit.layout_id = 100;
edit.title_id = 1;
edit.schema = &schema;
edit.profile = &input_profile;
edit.action = &edited_action;
edit.page = &edit_page;
edit.items_per_page = 4;
edit.item_slots = {"row0", "row1", "row2", "row3"};
edit.add_id = 20;
edit.add_slot = "add";
edit.add_command = add_bind;

gmenu::register_bind_action_edit_screen(menu, edit);
```

Selecting an existing bind removes it from the profile. Selecting add runs
`add_bind` with the edited action id as payload.

## Settings Screen

```cpp
gmenu::SettingsScreenDef settings;
settings.id = Settings;
settings.layout_id = 100;
settings.title_id = 1;
settings.title = "Settings";
settings.default_focus = 10;

gmenu::SettingItem fullscreen;
fullscreen.id = 10;
fullscreen.slot = "row0";
fullscreen.label = "Fullscreen";
fullscreen.type = gmenu::SettingType::Toggle;
fullscreen.bool_value = &config.fullscreen;
settings.items.push_back(fullscreen);

gmenu::SettingItem volume;
volume.id = 11;
volume.slot = "row1";
volume.label = "Volume";
volume.type = gmenu::SettingType::Slider1D;
volume.float_value = &config.volume;
volume.min = 0.0f;
volume.max = 1.0f;
volume.step = 0.05f;
settings.items.push_back(volume);

gmenu::register_settings_screen(menu, settings);
```

## Profile List

```cpp
std::vector<gmenu::ProfileEntry> profiles = {
    {1, "Default", "keyboard"},
    {2, "Guest", "gamepad"},
};
int selected_profile = -1;
int page = 0;

gmenu::ProfileListScreenDef picker;
picker.id = Profiles;
picker.layout_id = 100;
picker.title_id = 1;
picker.profiles = &profiles;
picker.selected_profile_id = &selected_profile;
picker.page = &page;
picker.items_per_page = 4;
picker.item_slots = {"row0", "row1", "row2", "row3"};

gmenu::register_profile_list_screen(menu, picker);
```

Selecting a profile sets `selected_profile_id`, unless `select_command` is set.
When `select_command` is set, selecting a profile dispatches the profile id as
the command payload instead.

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

`DrawItem::controls` contains common interaction sub-rects such as slider tracks
and option-cycle left/right/value regions. Renderers can draw from the same
rects that `gmenu` uses for hit testing.

## Composed Rows

Use normal widgets for rows with multiple controls:

```cpp
gmenu::ComposedRowDef row;
row.nav_up = Volume;
row.nav_down = Back;
row.widgets.push_back(gmenu::text_input(Width, "resolution_w", "Width", width_text, 5));
row.widgets.push_back(gmenu::text_input(Height, "resolution_h", "Height", height_text, 5));
row.widgets.push_back(gmenu::button(Apply, "resolution_apply", "Apply", apply_action));
gmenu::append_composed_row(out, std::move(row));
```

This avoids compound widgets with multiple hidden edit states. Each child is a
normal widget with its own id, slot, focus, nav, and action behavior.

## Text Input

```cpp
gmenu::Input input;
input.text = sdl_text_event_text;
input.backspace = backspace_pressed;
menu.update(input, dt, width, height);
```

Text input remains backend-neutral. `gmenu` stores edit focus and modifies the
bound string. The renderer decides how to draw carets and selection effects.

## ImGui Nav Editor

The optional `gmenu::imgui` target provides a nav editor/debug panel. It can
inspect registered screens, jump the menu root to a selected screen, edit nav
links for the current screen, and request nav save/load through
`NavEditorState`. The host still owns file paths and persistence policy.
