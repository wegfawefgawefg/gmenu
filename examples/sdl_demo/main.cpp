#include "glayout/editor.hpp"
#include "gmenu/gmenu.hpp"

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
#include "imgui_debug.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>
#endif

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
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
    std::string resolution_width = "1280";
    std::string resolution_height = "720";
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
    bool save_requested = false;
    bool load_requested = false;
};

gmenu::CommandId g_quit_command = gmenu::invalid_command;
gmenu::CommandId g_select_profile_command = gmenu::invalid_command;
gmenu::CommandId g_save_profile_command = gmenu::invalid_command;
gmenu::CommandId g_edit_bind_command = gmenu::invalid_command;
gmenu::CommandId g_add_bind_command = gmenu::invalid_command;
gmenu::CommandId g_apply_resolution_command = gmenu::invalid_command;

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

void command_apply_resolution(gmenu::BuildContext& ctx, int) {
    auto* state = static_cast<DemoState*>(ctx.user);
    state->save_status = "resolution " + state->resolution_width + "x" + state->resolution_height;
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
    name.nav_down = 32;
    name.on_commit = gmenu::Action::command_id(g_save_profile_command);
    out.widgets.push_back(std::move(name));

    gmenu::Widget save =
        gmenu::button(32, "row1", "Save Name", gmenu::Action::command_id(g_save_profile_command));
    save.style = kButtonStyle;
    save.nav_up = 31;
    save.nav_down = 33;
    out.widgets.push_back(std::move(save));

    gmenu::Widget pick =
        gmenu::button(33, "row2", "Pick Profile", gmenu::Action::push(kProfileList));
    pick.style = kButtonStyle;
    pick.nav_up = 32;
    pick.nav_down = 34;
    out.widgets.push_back(std::move(pick));

    gmenu::Widget back = gmenu::button(34, "back", "Back", gmenu::Action::pop());
    back.style = kButtonStyle;
    back.nav_up = 33;
    out.widgets.push_back(std::move(back));
}

void build_settings_demo(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<DemoState*>(ctx.user);
    out.id = kSettings;
    out.layout_id = 100;
    out.default_focus = 20;

    gmenu::Widget title = gmenu::label(2, "title", "settings");
    title.style = kTitleStyle;
    out.widgets.push_back(std::move(title));

    gmenu::Widget fullscreen = gmenu::toggle(20, "row0", "Fullscreen", state->fullscreen);
    fullscreen.style = kValueStyle;
    fullscreen.nav_down = 21;
    out.widgets.push_back(std::move(fullscreen));

    gmenu::Widget volume = gmenu::slider_1d(21, "row1", "Volume", state->volume, 0.0f, 1.0f, 0.1f);
    volume.style = kValueStyle;
    volume.nav_up = 20;
    volume.nav_down = 22;
    out.widgets.push_back(std::move(volume));

    gmenu::Widget quality =
        gmenu::option_cycle(22, "row2", "Quality", state->quality, {"low", "medium", "high"});
    quality.style = kValueStyle;
    quality.nav_up = 21;
    quality.nav_down = 23;
    out.widgets.push_back(std::move(quality));

    gmenu::Widget profile_name =
        gmenu::text_input(23, "row3", "Profile Name", state->profile_name, 32);
    profile_name.style = kValueStyle;
    profile_name.nav_up = 22;
    profile_name.nav_down = 26;
    profile_name.on_commit = gmenu::Action::command_id(g_save_profile_command);
    out.widgets.push_back(std::move(profile_name));

    gmenu::ComposedRowDef resolution;
    resolution.nav_up = 23;
    resolution.nav_down = 25;
    gmenu::Widget width =
        gmenu::text_input(26, "resolution_w", "Width", state->resolution_width, 5);
    width.style = kValueStyle;
    width.on_commit = gmenu::Action::command_id(g_apply_resolution_command);
    resolution.widgets.push_back(std::move(width));
    gmenu::Widget height =
        gmenu::text_input(27, "resolution_h", "Height", state->resolution_height, 5);
    height.style = kValueStyle;
    height.on_commit = gmenu::Action::command_id(g_apply_resolution_command);
    resolution.widgets.push_back(std::move(height));
    gmenu::Widget apply = gmenu::button(28, "resolution_apply", "Apply",
                                        gmenu::Action::command_id(g_apply_resolution_command));
    apply.style = kButtonStyle;
    resolution.widgets.push_back(std::move(apply));
    gmenu::append_composed_row(out, std::move(resolution));

    gmenu::Widget back = gmenu::button(25, "back", "Back", gmenu::Action::pop());
    back.style = kButtonStyle;
    back.nav_up = 26;
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
    layout.objects.push_back(
        glayout::Object{13, "resolution_w", glayout::Rect{0.24f, 0.70f, 0.16f, 0.07f}});
    layout.objects.push_back(
        glayout::Object{14, "resolution_h", glayout::Rect{0.42f, 0.70f, 0.16f, 0.07f}});
    layout.objects.push_back(
        glayout::Object{15, "resolution_apply", glayout::Rect{0.60f, 0.70f, 0.16f, 0.07f}});
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

const char* nav_source_text(gmenu::NavSource source) {
    switch (source) {
    case gmenu::NavSource::None:
        return "none";
    case gmenu::NavSource::Explicit:
        return "explicit";
    case gmenu::NavSource::Override:
        return "override";
    }
    return "none";
}

gmenu::NavSource nav_source_for_direction(const gmenu::DrawItem& item,
                                          gmenu::NavDirection direction) {
    switch (direction) {
    case gmenu::NavDirection::Up:
        return item.nav_up_source;
    case gmenu::NavDirection::Down:
        return item.nav_down_source;
    case gmenu::NavDirection::Left:
        return item.nav_left_source;
    case gmenu::NavDirection::Right:
        return item.nav_right_source;
    }
    return gmenu::NavSource::None;
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
                   const gmenu::DrawItem& source, gmenu::WidgetId target_id, const char* direction,
                   gmenu::NavSource nav_source) {
    const gmenu::DrawItem* target = find_draw_item(items, target_id);
    if (!target || nav_source == gmenu::NavSource::None) {
        return;
    }

    SDL_FPoint from = edge_point(source.rect, direction);
    SDL_FPoint to = edge_point(target->rect, direction);
    if (nav_source == gmenu::NavSource::Override) {
        set_color(renderer, 255, 210, 95);
    } else {
        set_color(renderer, 125, 180, 225);
    }
    SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);

    const float mid_x = (from.x + to.x) * 0.5f;
    const float mid_y = (from.y + to.y) * 0.5f;
    std::string label = std::string(direction) + " " + nav_source_text(nav_source);
    draw_text(renderer, mid_x + 4.0f, mid_y + 4.0f, label);
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

        draw_nav_link(renderer, items, item, item.nav_up, "up",
                      nav_source_for_direction(item, gmenu::NavDirection::Up));
        draw_nav_link(renderer, items, item, item.nav_down, "down",
                      nav_source_for_direction(item, gmenu::NavDirection::Down));
        draw_nav_link(renderer, items, item, item.nav_left, "left",
                      nav_source_for_direction(item, gmenu::NavDirection::Left));
        draw_nav_link(renderer, items, item, item.nav_right, "right",
                      nav_source_for_direction(item, gmenu::NavDirection::Right));
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
        SDL_FRect track =
            item.controls.has_slider_track
                ? to_sdl(item.controls.slider_track)
                : SDL_FRect{rect.x + 14.0f, rect.y + rect.h - 16.0f, rect.w - 28.0f, 5.0f};
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
    if (item.type == gmenu::WidgetType::OptionCycle) {
        if (item.controls.has_option_value) {
            SDL_FRect value = to_sdl(item.controls.option_value);
            set_color(renderer, 24, 37, 53);
            SDL_RenderFillRect(renderer, &value);
        }
        if (item.controls.has_option_left) {
            SDL_FRect left = to_sdl(item.controls.option_left);
            set_color(renderer, 20, 30, 40);
            SDL_RenderFillRect(renderer, &left);
            set_color(renderer, 248, 248, 238);
            draw_text(renderer, left.x + 12.0f, left.y + 5.0f, "<");
        }
        if (item.controls.has_option_right) {
            SDL_FRect right = to_sdl(item.controls.option_right);
            set_color(renderer, 20, 30, 40);
            SDL_RenderFillRect(renderer, &right);
            set_color(renderer, 248, 248, 238);
            draw_text(renderer, right.x + 12.0f, right.y + 5.0f, ">");
        }
    }
    if (!item.value.empty()) {
        draw_text(renderer, rect.x + rect.w - 160.0f, rect.y + 14.0f, item.value);
    }
    if (item.state.editing) {
        draw_text(renderer, rect.x + rect.w - 24.0f, rect.y + 14.0f, "_");
    }
}

void apply_key(SDL_Keycode key, SDL_Keymod mod, bool down, HeldInput& held, gmenu::Input& input,
               glayout::EditorInput& editor_input, DemoToggles& toggles, NavEditState& nav_editor) {
    const bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
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
    case SDLK_N:
        if (down && ctrl) {
            toggles.nav_overlay = !toggles.nav_overlay;
            if (toggles.nav_overlay) {
                toggles.layout_edit = false;
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
    case SDLK_S:
        if (down && toggles.nav_overlay) {
            nav_editor.save_requested = true;
        }
        break;
    case SDLK_L:
        if (down && ctrl) {
            toggles.layout_edit = !toggles.layout_edit;
            if (toggles.layout_edit) {
                toggles.nav_overlay = false;
            }
            held = {};
        } else if (down && toggles.nav_overlay) {
            nav_editor.load_requested = true;
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

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
#endif

    DemoState state;
    init_input_demo(state);
    const std::filesystem::path layout_path = "gmenu_layouts_demo.lisp";
    const std::filesystem::path nav_path = "gmenu_nav_demo.lisp";
    glayout::LayoutStore layout_store;
    glayout::ParseResult loaded_layouts = layout_store.load_file(layout_path);
    if (loaded_layouts.ok && !layout_store.layouts.empty()) {
        state.save_status = "loaded demo layouts";
    } else {
        layout_store.layouts = make_layouts();
    }
    gmenu::Menu menu;
    menu.set_layout_store(&layout_store);
    menu.set_user_data(&state);
    if (menu.load_nav_file(nav_path)) {
        state.save_status = "loaded nav overrides";
    }
    g_quit_command = menu.register_command(command_quit);
    g_select_profile_command = menu.register_command(command_select_profile);
    g_save_profile_command = menu.register_command(command_save_profile);
    g_edit_bind_command = menu.register_command(command_edit_bind);
    g_add_bind_command = menu.register_command(command_add_bind);
    g_apply_resolution_command = menu.register_command(command_apply_resolution);

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
    menu.register_screen(kSettings, build_settings_demo);
    menu.register_screen(kProfiles, build_profile_edit);
    gmenu::register_profile_list_screen(menu, profiles_def);
    gmenu::register_bind_action_list_screen(menu, binds_def);
    gmenu::register_bind_action_edit_screen(menu, bind_edit_def);
    menu.set_root(kMain);

    HeldInput held;
    glayout::EditorState layout_editor;
    NavEditState nav_editor;
    DemoToggles toggles;
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
    DemoDebugUi debug_ui;
#endif
    bool running = true;
    Uint64 last_ticks = SDL_GetTicks();

    while (running && !state.quit) {
        gmenu::Input input;
        glayout::EditorInput editor_input;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
            ImGui_ImplSDL3_ProcessEvent(&event);
#endif
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                const bool down = event.type == SDL_EVENT_KEY_DOWN;
                const bool debug_key = event.key.key == SDLK_F9 || event.key.key == SDLK_F10;
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
                if (down && event.key.key == SDLK_F10) {
                    debug_ui.enabled = !debug_ui.enabled;
                }
                if (down && event.key.key == SDLK_F9 && debug_ui.enabled) {
                    debug_ui.bar_visible = !debug_ui.bar_visible;
                }
                const bool imgui_wants_keyboard = ImGui::GetIO().WantCaptureKeyboard;
                if (!debug_key && imgui_wants_keyboard) {
                    continue;
                }
#else
                (void)debug_key;
#endif
                apply_key(event.key.key, event.key.mod, down, held, input, editor_input, toggles,
                          nav_editor);
                editor_input.ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
                editor_input.shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
                if (ImGui::GetIO().WantCaptureKeyboard) {
                    continue;
                }
#endif
                input.text += event.text.text;
            }
        }

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const bool imgui_wants_mouse = ImGui::GetIO().WantCaptureMouse;
#else
        const bool imgui_wants_mouse = false;
#endif

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
        input.mouse_down =
            !toggles.layout_edit && !toggles.nav_overlay && !imgui_wants_mouse && left_mouse_down;
        editor_input.mouse_x = mouse_x;
        editor_input.mouse_y = mouse_y;
        editor_input.left_down = toggles.layout_edit && !imgui_wants_mouse && left_mouse_down;

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        Uint64 now = SDL_GetTicks();
        float dt = static_cast<float>(now - last_ticks) / 1000.0f;
        last_ticks = now;
        if (toggles.layout_edit && !layout_store.layouts.empty()) {
            int layout_index = 0;
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
            layout_index = std::clamp(debug_ui.selected_layout, 0,
                                      static_cast<int>(layout_store.layouts.size()) - 1);
#endif
            glayout::Layout& active_layout =
                layout_store.layouts[static_cast<std::size_t>(layout_index)];
            glayout::editor_begin_frame(layout_editor, active_layout, editor_input,
                                        glayout::Viewport{
                                            0.0f,
                                            0.0f,
                                            static_cast<float>(width),
                                            static_cast<float>(height),
                                        });
            if (layout_editor.save_requested) {
                if (layout_store.save_file(layout_path)) {
                    glayout::editor_mark_saved(layout_editor);
                    state.save_status = "saved demo layouts";
                } else {
                    layout_editor.save_requested = false;
                    state.save_status = "failed to save demo layouts";
                }
            }
        }
        menu.update(input, dt, width, height);
        const bool nav_clicked = toggles.nav_overlay && !imgui_wants_mouse && left_mouse_down &&
                                 !nav_editor.mouse_was_down;
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
        if (toggles.nav_overlay && nav_editor.save_requested) {
            if (menu.save_nav_file(nav_path)) {
                menu.mark_nav_saved();
                state.save_status = "saved nav overrides";
            } else {
                state.save_status = "failed to save nav overrides";
            }
            nav_editor.save_requested = false;
        }
        if (toggles.nav_overlay && nav_editor.load_requested) {
            if (menu.load_nav_file(nav_path)) {
                state.save_status = "loaded nav overrides";
                menu.update(gmenu::Input{}, 0.0f, width, height);
            } else {
                state.save_status = "failed to load nav overrides";
            }
            nav_editor.load_requested = false;
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
            int layout_index = 0;
#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
            layout_index = std::clamp(debug_ui.selected_layout, 0,
                                      static_cast<int>(layout_store.layouts.size()) - 1);
#endif
            draw_layout_overlay(renderer, layout_editor,
                                layout_store.layouts[static_cast<std::size_t>(layout_index)], width,
                                height);
        }
        if (toggles.nav_overlay) {
            draw_nav_overlay(renderer, menu.draw_items(), nav_editor);
        }
        set_color(renderer, 155, 170, 185);
        if (toggles.layout_edit) {
            draw_text(renderer, 18.0f, static_cast<float>(height) - 44.0f,
                      "Ctrl+L layout edit ON: drag rectangles, Z/Y undo/redo, C/V "
                      "copy/paste, delete remove");
        } else if (toggles.nav_overlay) {
            std::string nav_text = "Ctrl+N nav edit ON: click source";
            if (nav_editor.source != gmenu::invalid_widget) {
                nav_text += " #" + std::to_string(nav_editor.source) + ", arrow chooses direction";
            }
            if (nav_editor.has_direction) {
                nav_text += ", click target for ";
                nav_text += direction_text(nav_editor.direction);
            }
            if (menu.nav_dirty()) {
                nav_text += " [dirty]";
            }
            draw_text(renderer, 18.0f, static_cast<float>(height) - 44.0f, nav_text);
        }
        draw_text(renderer, 18.0f, static_cast<float>(height) - 28.0f,
                  "Ctrl+L layout edit, Ctrl+N nav edit, F10 ImGui debug, arrows navigate, enter "
                  "select/edit, esc back, q/e page; nav S save/L load; status: " +
                      state.save_status);

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
        bool imgui_changed = render_demo_debug_ui(debug_ui, menu, layout_store, layout_editor);
        if (debug_ui.nav_editor.save_requested) {
            if (menu.save_nav_file(nav_path)) {
                menu.mark_nav_saved();
                state.save_status = "saved nav overrides";
            } else {
                state.save_status = "failed to save nav overrides";
            }
            debug_ui.nav_editor.save_requested = false;
        }
        if (debug_ui.nav_editor.load_requested) {
            if (menu.load_nav_file(nav_path)) {
                state.save_status = "loaded nav overrides";
                imgui_changed = true;
            } else {
                state.save_status = "failed to load nav overrides";
            }
            debug_ui.nav_editor.load_requested = false;
        }
        if (imgui_changed) {
            menu.update(gmenu::Input{}, 0.0f, width, height);
        }
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
#endif
        SDL_RenderPresent(renderer);
    }

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
#endif
    SDL_StopTextInput(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
