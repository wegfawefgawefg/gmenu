#include "gmenu/gmenu.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr gmenu::ScreenId kMain = 1;
constexpr gmenu::ScreenId kSettings = 2;
constexpr gmenu::ScreenId kProfiles = 3;

struct DemoState {
    bool fullscreen = false;
    float volume = 0.5f;
    std::string profile_name = "Default";
    bool quit_requested = false;
};

gmenu::CommandId g_quit_command = gmenu::invalid_command;

void command_quit(gmenu::BuildContext& ctx, int payload) {
    (void)payload;
    auto* state = static_cast<DemoState*>(ctx.user);
    state->quit_requested = true;
}

void build_main(gmenu::BuildContext&, gmenu::Screen& out) {
    out.id = kMain;
    out.layout_id = 100;
    out.default_focus = 10;
    out.widgets.push_back(gmenu::label(1, "title", "gmenu demo"));
    out.widgets.push_back(gmenu::button(10, "play", "Play", gmenu::Action::none()));
    out.widgets.push_back(
        gmenu::button(11, "settings", "Settings", gmenu::Action::push(kSettings)));
    out.widgets.push_back(
        gmenu::button(12, "profiles", "Profiles", gmenu::Action::push(kProfiles)));
    out.widgets.push_back(
        gmenu::button(13, "quit", "Quit", gmenu::Action::command_id(g_quit_command)));
}

void build_settings(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<DemoState*>(ctx.user);
    out.id = kSettings;
    out.layout_id = 100;
    out.default_focus = 20;
    out.widgets.push_back(gmenu::label(2, "title", "settings"));
    out.widgets.push_back(gmenu::toggle(20, "play", "Fullscreen", state->fullscreen));
    out.widgets.push_back(
        gmenu::slider_1d(21, "settings", "Volume", state->volume, 0.0f, 1.0f, 0.1f));
    out.widgets.push_back(gmenu::button(22, "back", "Back", gmenu::Action::pop()));
}

void build_profiles(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<DemoState*>(ctx.user);
    out.id = kProfiles;
    out.layout_id = 100;
    out.default_focus = 30;
    out.widgets.push_back(gmenu::label(3, "title", "profiles"));
    out.widgets.push_back(gmenu::text_input(30, "play", "Profile name", state->profile_name, 32));
    out.widgets.push_back(gmenu::button(31, "back", "Back", gmenu::Action::pop()));
}

std::vector<glayout::Layout> make_layouts() {
    glayout::Layout layout;
    layout.id = 100;
    layout.label = "demo";
    layout.width = 1280;
    layout.height = 720;
    layout.objects.push_back(glayout::Object{1, "title", glayout::Rect{0.1f, 0.08f, 0.8f, 0.12f}});
    layout.objects.push_back(glayout::Object{2, "play", glayout::Rect{0.22f, 0.25f, 0.56f, 0.10f}});
    layout.objects.push_back(
        glayout::Object{3, "settings", glayout::Rect{0.22f, 0.39f, 0.56f, 0.10f}});
    layout.objects.push_back(
        glayout::Object{4, "profiles", glayout::Rect{0.22f, 0.53f, 0.56f, 0.10f}});
    layout.objects.push_back(glayout::Object{5, "quit", glayout::Rect{0.22f, 0.67f, 0.56f, 0.10f}});
    layout.objects.push_back(glayout::Object{6, "back", glayout::Rect{0.22f, 0.67f, 0.56f, 0.10f}});
    return {layout};
}

void print_menu(const gmenu::Menu& menu, const DemoState& state) {
    std::cout << "screen=" << menu.current_screen() << " focus=" << menu.focus()
              << " fullscreen=" << (state.fullscreen ? "yes" : "no") << " volume=" << state.volume
              << " profile=" << state.profile_name << "\n";
    for (const gmenu::DrawItem& item : menu.draw_items()) {
        std::cout << (item.state.focused ? "> " : "  ") << item.label;
        if (!item.secondary.empty()) {
            std::cout << " - " << item.secondary;
        }
        std::cout << " rect=(" << item.rect.x << "," << item.rect.y << "," << item.rect.w << ","
                  << item.rect.h << ")\n";
    }
}

} // namespace

int main() {
    DemoState state;
    std::vector<glayout::Layout> layouts = make_layouts();

    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    g_quit_command = menu.register_command(command_quit);
    menu.register_screen(kMain, build_main);
    menu.register_screen(kSettings, build_settings);
    menu.register_screen(kProfiles, build_profiles);
    menu.set_root(kMain);

    menu.update(gmenu::Input{}, 0.016f, 1280, 720);
    print_menu(menu, state);
    return state.quit_requested ? 1 : 0;
}
