#include "glayout/editor.hpp"
#include "gmenu/gmenu.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr gmenu::ScreenId kMain = 1;
constexpr gmenu::ScreenId kSettings = 2;
constexpr gmenu::ScreenId kProfiles = 3;
constexpr gmenu::ScreenId kBinds = 4;
constexpr gmenu::ScreenId kBindEdit = 5;
constexpr gmenu::ScreenId kProfileList = 6;

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

constexpr gmenu::StyleId kTitleStyle = 10;
constexpr gmenu::StyleId kButtonStyle = 20;
constexpr gmenu::StyleId kValueStyle = 30;

struct DemoState {
    bool fullscreen = false;
    float volume = 0.5f;
    int quality = 1;
    std::string profile_name = "Default";
    std::string save_status = "not saved";
    int selected_profile_id = 1;
    int profile_page = 0;
    int bind_action_page = 0;
    int bind_edit_page = 0;
    int selected_action = 1;
    int next_fake_key = 100;
    std::vector<gmenu::ProfileEntry> profiles{
        {1, "Default", "keyboard"},
        {2, "Guest", "gamepad"},
        {3, "Speedrun", "custom"},
        {4, "Couch", "shared"},
    };
    ginput::Schema input_schema;
    ginput::InputProfile input_profile;
    bool quit = false;
};

struct HeldInput {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

struct DemoToggles {
    bool layout_edit = false;
    bool nav_overlay = false;
};

struct NavEditState {
    gmenu::WidgetId source = gmenu::invalid_widget;
    gmenu::NavDirection direction = gmenu::NavDirection::Down;
    bool has_direction = false;
    bool mouse_was_down = false;
    bool clear_source = false;
    bool clear_link = false;
};

gmenu::CommandId g_quit_command = gmenu::invalid_command;
gmenu::CommandId g_select_profile_command = gmenu::invalid_command;
gmenu::CommandId g_save_profile_command = gmenu::invalid_command;
gmenu::CommandId g_edit_bind_command = gmenu::invalid_command;
gmenu::CommandId g_add_bind_command = gmenu::invalid_command;

void command_quit(gmenu::BuildContext& ctx, int) {
    auto* state = static_cast<DemoState*>(ctx.user);
    state->quit = true;
}

void command_select_profile(gmenu::BuildContext& ctx, int payload) {
    auto* state = static_cast<DemoState*>(ctx.user);
    state->selected_profile_id = payload;
    for (const gmenu::ProfileEntry& profile : state->profiles) {
        if (profile.id == payload) {
            state->profile_name = profile.name;
            state->save_status = "selected " + profile.name;
            ctx.menu.pop();
            return;
        }
    }
}

void command_save_profile(gmenu::BuildContext& ctx, int) {
    auto* state = static_cast<DemoState*>(ctx.user);
    std::string name = state->profile_name.empty() ? "Unnamed" : state->profile_name;
    for (gmenu::ProfileEntry& profile : state->profiles) {
        if (profile.id == state->selected_profile_id) {
            profile.name = name;
            profile.secondary = "saved in memory";
            state->save_status = "saved " + name;
            return;
        }
    }
}

void command_edit_bind(gmenu::BuildContext& ctx, int payload) {
    auto* state = static_cast<DemoState*>(ctx.user);
    state->selected_action = payload;
    state->bind_edit_page = 0;
    ctx.menu.push(kBindEdit);
}

void command_add_bind(gmenu::BuildContext& ctx, int payload) {
    auto* state = static_cast<DemoState*>(ctx.user);
    ginput::DeviceButton button;
    button.kind = ginput::DeviceKind::Keyboard;
    button.code = state->next_fake_key;
    ++state->next_fake_key;
    ginput::add_button_bind(state->input_profile,
                            ginput::ButtonBind{ginput::encode_button(button), payload});
    state->save_status = "demo added fake key " + std::to_string(button.code);
}

void init_input_demo(DemoState& state) {
    state.input_schema.add_action(1, "Move Left", "movement");
    state.input_schema.add_action(2, "Move Right", "movement");
    state.input_schema.add_action(3, "Jump", "movement");
    state.input_schema.add_action(4, "Pause", "system");
    state.input_profile.id = 1;
    state.input_profile.name = "keyboard";

    ginput::DeviceButton left;
    left.kind = ginput::DeviceKind::Keyboard;
    left.code = 'A';
    ginput::add_button_bind(state.input_profile,
                            ginput::ButtonBind{ginput::encode_button(left), 1});

    ginput::DeviceButton right;
    right.kind = ginput::DeviceKind::Keyboard;
    right.code = 'D';
    ginput::add_button_bind(state.input_profile,
                            ginput::ButtonBind{ginput::encode_button(right), 2});

    ginput::DeviceButton jump;
    jump.kind = ginput::DeviceKind::Keyboard;
    jump.code = ' ';
    ginput::add_button_bind(state.input_profile,
                            ginput::ButtonBind{ginput::encode_button(jump), 3});
}

void build_profile_edit(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<DemoState*>(ctx.user);
    out.id = kProfiles;
    out.layout_id = 100;
    out.default_focus = 31;

    gmenu::Widget title = gmenu::label(3, "title", "profile save");
    title.style = kTitleStyle;
    out.widgets.push_back(std::move(title));

    gmenu::Widget status = gmenu::label(30, "status", state->save_status);
    status.style = kValueStyle;
    out.widgets.push_back(std::move(status));

    gmenu::Widget name = gmenu::text_input(31, "row0", "Profile Name", state->profile_name, 32);
    name.style = kValueStyle;
    out.widgets.push_back(std::move(name));

    gmenu::Widget save =
        gmenu::button(32, "row1", "Save Name", gmenu::Action::command_id(g_save_profile_command));
    save.style = kButtonStyle;
    out.widgets.push_back(std::move(save));

    gmenu::Widget pick =
        gmenu::button(33, "row2", "Pick Profile", gmenu::Action::push(kProfileList));
    pick.style = kButtonStyle;
    out.widgets.push_back(std::move(pick));

    gmenu::Widget back = gmenu::button(34, "back", "Back", gmenu::Action::pop());
    back.style = kButtonStyle;
    out.widgets.push_back(std::move(back));
}

std::vector<glayout::Layout> make_layouts() {
    glayout::Layout layout;
    layout.id = 100;
    layout.label = "desktop";
    layout.width = 1280;
    layout.height = 720;
    layout.objects.push_back(
        glayout::Object{1, "title", glayout::Rect{0.18f, 0.06f, 0.64f, 0.10f}});
    layout.objects.push_back(
        glayout::Object{2, "status", glayout::Rect{0.24f, 0.17f, 0.52f, 0.06f}});
    layout.objects.push_back(glayout::Object{3, "row0", glayout::Rect{0.24f, 0.25f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{4, "row1", glayout::Rect{0.24f, 0.34f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{5, "row2", glayout::Rect{0.24f, 0.43f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{6, "row3", glayout::Rect{0.24f, 0.52f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{7, "row4", glayout::Rect{0.24f, 0.61f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{8, "row5", glayout::Rect{0.24f, 0.70f, 0.52f, 0.07f}});
    layout.objects.push_back(glayout::Object{9, "page", glayout::Rect{0.24f, 0.80f, 0.20f, 0.06f}});
    layout.objects.push_back(
        glayout::Object{10, "prev", glayout::Rect{0.46f, 0.80f, 0.08f, 0.06f}});
    layout.objects.push_back(
        glayout::Object{11, "next", glayout::Rect{0.56f, 0.80f, 0.08f, 0.06f}});
    layout.objects.push_back(
        glayout::Object{12, "back", glayout::Rect{0.66f, 0.80f, 0.10f, 0.06f}});
    return {layout};
}

SDL_FRect to_sdl(glayout::Rect rect) {
    return SDL_FRect{rect.x, rect.y, rect.w, rect.h};
}

void set_color(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void draw_text(SDL_Renderer* renderer, float x, float y, const std::string& text) {
    SDL_RenderDebugText(renderer, x, y, text.c_str());
}

void draw_grid(SDL_Renderer* renderer, int width, int height, float step) {
    if (step <= 0.0f) {
        return;
    }

    int index = 1;
    for (float x = step; x < 1.0f; x += step, ++index) {
        Uint8 shade = index % 5 == 0 ? 64 : 36;
        set_color(renderer, shade, static_cast<Uint8>(shade + 7), static_cast<Uint8>(shade + 12));
        const float screen_x = x * static_cast<float>(width);
        SDL_RenderLine(renderer, screen_x, 0.0f, screen_x, static_cast<float>(height));
    }

    index = 1;
    for (float y = step; y < 1.0f; y += step, ++index) {
        Uint8 shade = index % 5 == 0 ? 64 : 36;
        set_color(renderer, shade, static_cast<Uint8>(shade + 7), static_cast<Uint8>(shade + 12));
        const float screen_y = y * static_cast<float>(height);
        SDL_RenderLine(renderer, 0.0f, screen_y, static_cast<float>(width), screen_y);
    }
}

void draw_layout_overlay(SDL_Renderer* renderer, const glayout::EditorState& editor,
                         const glayout::Layout& layout, int width, int height) {
    glayout::Viewport viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
    std::vector<glayout::OverlayObject> objects =
        glayout::editor_collect_overlay_objects(editor, layout, viewport);
    for (const glayout::OverlayObject& object : objects) {
        SDL_FRect rect = to_sdl(object.rect);
        if (object.selected) {
            set_color(renderer, 255, 95, 82);
        } else {
            set_color(renderer, 125, 180, 225);
        }
        SDL_RenderRect(renderer, &rect);
    }

    std::vector<glayout::OverlayHandle> handles =
        glayout::editor_collect_overlay_handles(editor, layout, viewport);
    set_color(renderer, 255, 95, 82);
    for (const glayout::OverlayHandle& handle : handles) {
        SDL_FRect rect = to_sdl(handle.rect);
        SDL_RenderFillRect(renderer, &rect);
    }
}

const gmenu::DrawItem* find_draw_item(const std::span<const gmenu::DrawItem>& items,
                                      gmenu::WidgetId id) {
    for (const gmenu::DrawItem& item : items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

const gmenu::DrawItem* find_draw_item_at(const std::span<const gmenu::DrawItem>& items, float x,
                                         float y) {
    for (const gmenu::DrawItem& item : items) {
        if (item.type == gmenu::WidgetType::Label) {
            continue;
        }
        const bool inside = x >= item.rect.x && x <= item.rect.x + item.rect.w &&
                            y >= item.rect.y && y <= item.rect.y + item.rect.h;
        if (inside) {
            return &item;
        }
    }
    return nullptr;
}

const char* direction_text(gmenu::NavDirection direction) {
    switch (direction) {
    case gmenu::NavDirection::Up:
        return "up";
    case gmenu::NavDirection::Down:
        return "down";
    case gmenu::NavDirection::Left:
        return "left";
    case gmenu::NavDirection::Right:
        return "right";
    }
    return "down";
}

SDL_FPoint edge_point(glayout::Rect rect, const char* direction) {
    if (std::string(direction) == "up") {
        return SDL_FPoint{rect.x + rect.w * 0.5f, rect.y};
    }
    if (std::string(direction) == "down") {
        return SDL_FPoint{rect.x + rect.w * 0.5f, rect.y + rect.h};
    }
    if (std::string(direction) == "left") {
        return SDL_FPoint{rect.x, rect.y + rect.h * 0.5f};
    }
    return SDL_FPoint{rect.x + rect.w, rect.y + rect.h * 0.5f};
}

void draw_nav_link(SDL_Renderer* renderer, const std::span<const gmenu::DrawItem>& items,
                   const gmenu::DrawItem& source, gmenu::WidgetId target_id,
                   const char* direction) {
    const gmenu::DrawItem* target = find_draw_item(items, target_id);
    if (!target) {
        return;
    }

    SDL_FPoint from = edge_point(source.rect, direction);
    SDL_FPoint to = edge_point(target->rect, direction);
    set_color(renderer, 255, 210, 95);
    SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);

    const float mid_x = (from.x + to.x) * 0.5f;
    const float mid_y = (from.y + to.y) * 0.5f;
    draw_text(renderer, mid_x + 4.0f, mid_y + 4.0f, direction);
}

void draw_nav_overlay(SDL_Renderer* renderer, const std::span<const gmenu::DrawItem>& items,
                      const NavEditState& nav_editor) {
    for (const gmenu::DrawItem& item : items) {
        if (item.type == gmenu::WidgetType::Label) {
            continue;
        }

        if (item.id == nav_editor.source) {
            SDL_FRect rect = to_sdl(item.rect);
            set_color(renderer, 255, 95, 82);
            SDL_RenderRect(renderer, &rect);
        }

        set_color(renderer, 255, 210, 95);
        draw_text(renderer, item.rect.x + 4.0f, item.rect.y + item.rect.h - 16.0f,
                  "#" + std::to_string(item.id));

        draw_nav_link(renderer, items, item, item.nav_up, "up");
        draw_nav_link(renderer, items, item, item.nav_down, "down");
        draw_nav_link(renderer, items, item, item.nav_left, "left");
        draw_nav_link(renderer, items, item, item.nav_right, "right");
    }
}

void draw_item(SDL_Renderer* renderer, const gmenu::DrawItem& item) {
    SDL_FRect rect = to_sdl(item.rect);

    if (item.type == gmenu::WidgetType::Label) {
        set_color(renderer, 244, 232, 190);
        draw_text(renderer, rect.x + 12.0f, rect.y + 16.0f, item.label);
        return;
    }

    if (item.state.focused) {
        set_color(renderer, 235, 184, 63);
    } else if (item.state.hovered) {
        set_color(renderer, 74, 111, 143);
    } else {
        set_color(renderer, 43, 70, 98);
    }
    SDL_RenderFillRect(renderer, &rect);

    set_color(renderer, item.state.focused ? 255 : 170, item.state.focused ? 244 : 190,
              item.state.focused ? 190 : 210);
    SDL_RenderRect(renderer, &rect);

    set_color(renderer, 248, 248, 238);
    draw_text(renderer, rect.x + 14.0f, rect.y + 14.0f, item.label);
    if (!item.secondary.empty()) {
        set_color(renderer, 190, 205, 215);
        draw_text(renderer, rect.x + 14.0f, rect.y + 34.0f, item.secondary);
    }
    if (item.type == gmenu::WidgetType::Slider1D) {
        SDL_FRect track{rect.x + 14.0f, rect.y + rect.h - 16.0f, rect.w - 28.0f, 5.0f};
        set_color(renderer, 20, 30, 40);
        SDL_RenderFillRect(renderer, &track);
        SDL_FRect fill = track;
        fill.w *= 0.5f;
        if (!item.value.empty()) {
            fill.w = track.w * std::stof(item.value);
        }
        set_color(renderer, 235, 184, 63);
        SDL_RenderFillRect(renderer, &fill);
    }
    if (!item.value.empty()) {
        draw_text(renderer, rect.x + rect.w - 160.0f, rect.y + 14.0f, item.value);
    }
    if (item.state.editing) {
        draw_text(renderer, rect.x + rect.w - 24.0f, rect.y + 14.0f, "_");
    }
}

void apply_key(SDL_Keycode key, bool down, HeldInput& held, gmenu::Input& input,
               glayout::EditorInput& editor_input, DemoToggles& toggles, NavEditState& nav_editor) {
    switch (key) {
    case SDLK_F1:
        if (down) {
            toggles.layout_edit = !toggles.layout_edit;
            if (toggles.layout_edit) {
                toggles.nav_overlay = false;
            }
            held = {};
        }
        break;
    case SDLK_F2:
        if (down) {
            toggles.nav_overlay = !toggles.nav_overlay;
            if (toggles.nav_overlay) {
                toggles.layout_edit = false;
            }
            held = {};
        }
        break;
    case SDLK_UP:
    case SDLK_W:
        if (toggles.nav_overlay && down) {
            nav_editor.direction = gmenu::NavDirection::Up;
            nav_editor.has_direction = true;
        } else if (!toggles.nav_overlay) {
            held.up = down;
        }
        break;
    case SDLK_DOWN:
        if (toggles.nav_overlay && down) {
            nav_editor.direction = gmenu::NavDirection::Down;
            nav_editor.has_direction = true;
        } else if (!toggles.nav_overlay) {
            held.down = down;
        }
        break;
    case SDLK_LEFT:
    case SDLK_A:
        if (toggles.nav_overlay && down) {
            nav_editor.direction = gmenu::NavDirection::Left;
            nav_editor.has_direction = true;
        } else if (!toggles.nav_overlay) {
            held.left = down;
        }
        break;
    case SDLK_RIGHT:
    case SDLK_D:
        if (toggles.nav_overlay && down) {
            nav_editor.direction = gmenu::NavDirection::Right;
            nav_editor.has_direction = true;
        } else if (!toggles.nav_overlay) {
            held.right = down;
        }
        break;
    case SDLK_RETURN:
    case SDLK_SPACE:
        if (down) {
            input.select = true;
        }
        break;
    case SDLK_ESCAPE:
        if (down) {
            input.back = true;
        }
        break;
    case SDLK_BACKSPACE:
        if (down) {
            input.backspace = true;
            editor_input.key_delete = toggles.layout_edit;
            if (toggles.nav_overlay) {
                nav_editor.clear_link = true;
            }
        }
        break;
    case SDLK_DELETE:
        if (down) {
            editor_input.key_delete = toggles.layout_edit;
            if (toggles.nav_overlay) {
                nav_editor.clear_source = true;
            }
        }
        break;
    case SDLK_Q:
        if (down) {
            input.page_prev = true;
        }
        break;
    case SDLK_E:
        if (down) {
            input.page_next = true;
        }
        break;
    case SDLK_Z:
        if (down) {
            editor_input.key_undo = toggles.layout_edit;
        }
        break;
    case SDLK_Y:
        if (down) {
            editor_input.key_redo = toggles.layout_edit;
        }
        break;
    case SDLK_C:
        if (down) {
            editor_input.key_copy = toggles.layout_edit;
        }
        break;
    case SDLK_V:
        if (down) {
            editor_input.key_paste = toggles.layout_edit;
        }
        break;
    default:
        break;
    }
}

} // namespace

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("gmenu SDL demo", kWindowWidth, kWindowHeight,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_StartTextInput(window);

    DemoState state;
    init_input_demo(state);
    glayout::LayoutStore layout_store;
    layout_store.layouts = make_layouts();
    gmenu::Menu menu;
    menu.set_layout_store(&layout_store);
    menu.set_user_data(&state);
    g_quit_command = menu.register_command(command_quit);
    g_select_profile_command = menu.register_command(command_select_profile);
    g_save_profile_command = menu.register_command(command_save_profile);
    g_edit_bind_command = menu.register_command(command_edit_bind);
    g_add_bind_command = menu.register_command(command_add_bind);

    gmenu::ListScreenDef main_def;
    main_def.id = kMain;
    main_def.layout_id = 100;
    main_def.title_id = 1;
    main_def.title = "gmenu";
    main_def.title_style = kTitleStyle;
    main_def.default_focus = 10;
    main_def.items.push_back(
        gmenu::ListItem{10, "row0", "Play", "", gmenu::Action::none(), kButtonStyle});
    main_def.items.push_back(
        gmenu::ListItem{11, "row1", "Settings", "", gmenu::Action::push(kSettings), kButtonStyle});
    main_def.items.push_back(
        gmenu::ListItem{12, "row2", "Profiles", "", gmenu::Action::push(kProfiles), kButtonStyle});
    main_def.items.push_back(
        gmenu::ListItem{13, "row3", "Input Binds", "", gmenu::Action::push(kBinds), kButtonStyle});
    main_def.items.push_back(gmenu::ListItem{
        14, "row4", "Quit", "", gmenu::Action::command_id(g_quit_command), kButtonStyle});

    gmenu::SettingsScreenDef settings_def;
    settings_def.id = kSettings;
    settings_def.layout_id = 100;
    settings_def.title_id = 2;
    settings_def.title = "settings";
    settings_def.title_style = kTitleStyle;
    settings_def.default_focus = 20;
    settings_def.item_style = kValueStyle;
    settings_def.nav_style = kButtonStyle;
    settings_def.back_id = 25;
    settings_def.back_slot = "back";
    gmenu::SettingItem fullscreen;
    fullscreen.id = 20;
    fullscreen.slot = "row0";
    fullscreen.label = "Fullscreen";
    fullscreen.type = gmenu::SettingType::Toggle;
    fullscreen.bool_value = &state.fullscreen;
    settings_def.items.push_back(fullscreen);
    gmenu::SettingItem volume;
    volume.id = 21;
    volume.slot = "row1";
    volume.label = "Volume";
    volume.type = gmenu::SettingType::Slider1D;
    volume.float_value = &state.volume;
    volume.min = 0.0f;
    volume.max = 1.0f;
    volume.step = 0.1f;
    settings_def.items.push_back(volume);
    gmenu::SettingItem quality;
    quality.id = 22;
    quality.slot = "row2";
    quality.label = "Quality";
    quality.type = gmenu::SettingType::OptionCycle;
    quality.option_index = &state.quality;
    quality.options = {"low", "medium", "high"};
    settings_def.items.push_back(quality);
    gmenu::SettingItem profile_name;
    profile_name.id = 23;
    profile_name.slot = "row3";
    profile_name.label = "Profile Name";
    profile_name.type = gmenu::SettingType::TextInput;
    profile_name.text_value = &state.profile_name;
    profile_name.text_max_len = 32;
    settings_def.items.push_back(profile_name);

    gmenu::ProfileListScreenDef profiles_def;
    profiles_def.id = kProfileList;
    profiles_def.layout_id = 100;
    profiles_def.title_id = 40;
    profiles_def.title = "profiles";
    profiles_def.title_style = kTitleStyle;
    profiles_def.default_focus = 3001;
    profiles_def.profiles = &state.profiles;
    profiles_def.selected_profile_id = &state.selected_profile_id;
    profiles_def.select_command = g_select_profile_command;
    profiles_def.page = &state.profile_page;
    profiles_def.items_per_page = 3;
    profiles_def.item_slots = {"row0", "row1", "row2"};
    profiles_def.page_label_id = 41;
    profiles_def.prev_id = 42;
    profiles_def.next_id = 43;
    profiles_def.back_id = 44;
    profiles_def.back_slot = "back";
    profiles_def.nav_style = kButtonStyle;
    profiles_def.item_style = kValueStyle;

    gmenu::BindActionListScreenDef binds_def;
    binds_def.id = kBinds;
    binds_def.layout_id = 100;
    binds_def.title_id = 50;
    binds_def.title = "input binds";
    binds_def.title_style = kTitleStyle;
    binds_def.default_focus = 1001;
    binds_def.schema = &state.input_schema;
    binds_def.profile = &state.input_profile;
    binds_def.page = &state.bind_action_page;
    binds_def.items_per_page = 4;
    binds_def.item_slots = {"row0", "row1", "row2", "row3"};
    binds_def.edit_command = g_edit_bind_command;
    binds_def.page_label_id = 51;
    binds_def.prev_id = 52;
    binds_def.next_id = 53;
    binds_def.back_id = 54;
    binds_def.back_slot = "back";
    binds_def.nav_style = kButtonStyle;
    binds_def.item_style = kValueStyle;

    gmenu::BindActionEditScreenDef bind_edit_def;
    bind_edit_def.id = kBindEdit;
    bind_edit_def.layout_id = 100;
    bind_edit_def.title_id = 60;
    bind_edit_def.title_style = kTitleStyle;
    bind_edit_def.default_focus = 61;
    bind_edit_def.schema = &state.input_schema;
    bind_edit_def.profile = &state.input_profile;
    bind_edit_def.action = &state.selected_action;
    bind_edit_def.page = &state.bind_edit_page;
    bind_edit_def.items_per_page = 3;
    bind_edit_def.item_slots = {"row0", "row1", "row2"};
    bind_edit_def.add_command = g_add_bind_command;
    bind_edit_def.add_id = 61;
    bind_edit_def.add_slot = "row3";
    bind_edit_def.add_label = "Add Demo Key";
    bind_edit_def.page_label_id = 62;
    bind_edit_def.prev_id = 63;
    bind_edit_def.next_id = 64;
    bind_edit_def.back_id = 65;
    bind_edit_def.back_slot = "back";
    bind_edit_def.nav_style = kButtonStyle;
    bind_edit_def.item_style = kValueStyle;

    gmenu::register_list_screen(menu, main_def);
    gmenu::register_settings_screen(menu, settings_def);
    menu.register_screen(kProfiles, build_profile_edit);
    gmenu::register_profile_list_screen(menu, profiles_def);
    gmenu::register_bind_action_list_screen(menu, binds_def);
    gmenu::register_bind_action_edit_screen(menu, bind_edit_def);
    menu.set_root(kMain);

    HeldInput held;
    glayout::EditorState layout_editor;
    NavEditState nav_editor;
    DemoToggles toggles;
    bool running = true;
    Uint64 last_ticks = SDL_GetTicks();

    while (running && !state.quit) {
        gmenu::Input input;
        glayout::EditorInput editor_input;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                const bool down = event.type == SDL_EVENT_KEY_DOWN;
                apply_key(event.key.key, down, held, input, editor_input, toggles, nav_editor);
                editor_input.ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
                editor_input.shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                input.text += event.text.text;
            }
        }

        input.up = held.up;
        input.down = held.down;
        input.left = held.left;
        input.right = held.right;
        if (toggles.layout_edit || toggles.nav_overlay) {
            input.up = false;
            input.down = false;
            input.left = false;
            input.right = false;
            input.select = false;
            input.back = false;
            input.page_prev = false;
            input.page_next = false;
            input.backspace = false;
            input.text.clear();
        }
        input.mouse_valid = true;
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
        SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        input.mouse_x = mouse_x;
        input.mouse_y = mouse_y;
        const bool left_mouse_down = (buttons & SDL_BUTTON_LMASK) != 0;
        input.mouse_down = !toggles.layout_edit && !toggles.nav_overlay && left_mouse_down;
        editor_input.mouse_x = mouse_x;
        editor_input.mouse_y = mouse_y;
        editor_input.left_down = toggles.layout_edit && left_mouse_down;

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        Uint64 now = SDL_GetTicks();
        float dt = static_cast<float>(now - last_ticks) / 1000.0f;
        last_ticks = now;
        if (toggles.layout_edit && !layout_store.layouts.empty()) {
            glayout::editor_begin_frame(layout_editor, layout_store.layouts.front(), editor_input,
                                        glayout::Viewport{
                                            0.0f,
                                            0.0f,
                                            static_cast<float>(width),
                                            static_cast<float>(height),
                                        });
            if (layout_editor.save_requested) {
                layout_editor.save_requested = false;
                state.save_status = "layout save requested (demo memory only)";
            }
        }
        menu.update(input, dt, width, height);
        const bool nav_clicked =
            toggles.nav_overlay && left_mouse_down && !nav_editor.mouse_was_down;
        nav_editor.mouse_was_down = left_mouse_down;
        if (nav_clicked) {
            const gmenu::DrawItem* hit = find_draw_item_at(menu.draw_items(), mouse_x, mouse_y);
            if (hit && nav_editor.source != gmenu::invalid_widget && nav_editor.has_direction) {
                menu.set_nav_link(menu.current_screen(), nav_editor.source, nav_editor.direction,
                                  hit->id);
                state.save_status = "nav " + std::to_string(nav_editor.source) + " " +
                                    direction_text(nav_editor.direction) + " -> " +
                                    std::to_string(hit->id);
                nav_editor.source = hit->id;
                nav_editor.has_direction = false;
                menu.update(gmenu::Input{}, 0.0f, width, height);
            } else if (hit) {
                nav_editor.source = hit->id;
                nav_editor.has_direction = false;
            }
        }
        if (toggles.nav_overlay && nav_editor.source != gmenu::invalid_widget &&
            nav_editor.clear_link && nav_editor.has_direction) {
            menu.clear_nav_link(menu.current_screen(), nav_editor.source, nav_editor.direction);
            state.save_status = "cleared nav " + std::to_string(nav_editor.source) + " " +
                                direction_text(nav_editor.direction);
            nav_editor.clear_link = false;
            nav_editor.has_direction = false;
            menu.update(gmenu::Input{}, 0.0f, width, height);
        } else if (toggles.nav_overlay && nav_editor.source != gmenu::invalid_widget &&
                   (nav_editor.clear_link || nav_editor.clear_source)) {
            menu.clear_nav_links(menu.current_screen(), nav_editor.source);
            state.save_status = "cleared nav links for " + std::to_string(nav_editor.source);
            nav_editor.clear_link = false;
            nav_editor.clear_source = false;
            nav_editor.has_direction = false;
            menu.update(gmenu::Input{}, 0.0f, width, height);
        } else {
            nav_editor.clear_link = false;
            nav_editor.clear_source = false;
        }

        set_color(renderer, 16, 22, 29);
        SDL_RenderClear(renderer);
        if (toggles.layout_edit) {
            draw_grid(renderer, width, height, layout_editor.grid_step);
        }
        for (const gmenu::DrawItem& item : menu.draw_items()) {
            draw_item(renderer, item);
        }
        if (toggles.layout_edit && !layout_store.layouts.empty()) {
            draw_layout_overlay(renderer, layout_editor, layout_store.layouts.front(), width,
                                height);
        }
        if (toggles.nav_overlay) {
            draw_nav_overlay(renderer, menu.draw_items(), nav_editor);
        }
        set_color(renderer, 155, 170, 185);
        if (toggles.layout_edit) {
            draw_text(renderer, 18.0f, static_cast<float>(height) - 44.0f,
                      "F1 layout edit ON: drag rectangles, Z/Y undo/redo, C/V "
                      "copy/paste, delete remove");
        } else if (toggles.nav_overlay) {
            std::string nav_text = "F2 nav edit ON: click source";
            if (nav_editor.source != gmenu::invalid_widget) {
                nav_text += " #" + std::to_string(nav_editor.source) + ", arrow chooses direction";
            }
            if (nav_editor.has_direction) {
                nav_text += ", click target for ";
                nav_text += direction_text(nav_editor.direction);
            }
            draw_text(renderer, 18.0f, static_cast<float>(height) - 44.0f, nav_text);
        }
        draw_text(renderer, 18.0f, static_cast<float>(height) - 28.0f,
                  "F1 layout edit, F2 nav edit, arrows navigate, enter select/edit, esc back, "
                  "q/e page; binds add fake keys only; status: " +
                      state.save_status);
        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
