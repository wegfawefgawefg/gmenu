#include "gmenu/gmenu.hpp"

#include <cassert>
#include <string>
#include <vector>

namespace {

constexpr gmenu::ScreenId kMain = 1;
constexpr gmenu::ScreenId kSettings = 2;
constexpr gmenu::WidgetId kPlay = 10;
constexpr gmenu::WidgetId kSettingsButton = 11;
constexpr gmenu::WidgetId kBack = 12;

struct AppState {
    bool command_ran = false;
    bool fullscreen = false;
    float volume = 0.5f;
    int quality = 0;
    std::string name = "Player";
};

void command_mark(gmenu::BuildContext& ctx, int payload) {
    (void)payload;
    auto* state = static_cast<AppState*>(ctx.user);
    state->command_ran = true;
}

gmenu::CommandId g_mark_command = gmenu::invalid_command;

void build_main(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    out.id = kMain;
    out.layout_id = 100;
    out.default_focus = kPlay;
    out.widgets.push_back(
        gmenu::button(kPlay, "play", "Play", gmenu::Action::command_id(g_mark_command)));
    out.widgets.push_back(
        gmenu::button(kSettingsButton, "settings", "Settings", gmenu::Action::push(kSettings)));
    out.widgets[0].nav_down = kSettingsButton;
    out.widgets[1].nav_up = kPlay;
    (void)ctx;
}

void build_settings(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<AppState*>(ctx.user);
    out.id = kSettings;
    out.layout_id = 100;
    out.default_focus = kBack;
    out.widgets.push_back(gmenu::toggle(20, "play", "Fullscreen", state->fullscreen));
    out.widgets.push_back(
        gmenu::slider_1d(21, "settings", "Volume", state->volume, 0.0f, 1.0f, 0.25f));
    out.widgets.push_back(
        gmenu::option_cycle(23, "quality", "Quality", state->quality, {"low", "medium", "high"}));
    out.widgets.push_back(gmenu::text_input(22, "name", "Name", state->name, 32));
    out.widgets.push_back(gmenu::button(kBack, "back", "Back", gmenu::Action::pop()));
}

std::vector<glayout::Layout> make_layouts() {
    glayout::Layout layout;
    layout.id = 100;
    layout.label = "test";
    layout.width = 800;
    layout.height = 600;
    layout.objects.push_back(glayout::Object{1, "play", glayout::Rect{0.1f, 0.1f, 0.8f, 0.1f}});
    layout.objects.push_back(
        glayout::Object{2, "settings", glayout::Rect{0.1f, 0.25f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{3, "name", glayout::Rect{0.1f, 0.4f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{4, "back", glayout::Rect{0.1f, 0.55f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{5, "quality", glayout::Rect{0.1f, 0.7f, 0.8f, 0.1f}});
    return {layout};
}

void test_stack_and_commands() {
    AppState state;
    std::vector<glayout::Layout> layouts = make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    g_mark_command = menu.register_command(command_mark);
    menu.register_screen(kMain, build_main);
    menu.register_screen(kSettings, build_settings);

    assert(menu.set_root(kMain));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.current_screen() == kMain);
    assert(menu.focus() == kPlay);
    assert(menu.draw_items().size() == 2);
    assert(menu.draw_items()[0].rect.w == 640.0f);

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(state.command_ran);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    assert(menu.focus() == kSettingsButton);
    menu.update(select, 0.016f, 800, 600);
    assert(menu.current_screen() == kSettings);
    assert(menu.stack().size() == 2);
    assert(menu.draw_items().size() == 5);
    assert(menu.draw_items()[0].label == "Fullscreen");

    gmenu::Input back;
    back.back = true;
    menu.update(back, 0.016f, 800, 600);
    assert(menu.current_screen() == kMain);
    assert(menu.draw_items().size() == 2);
    assert(menu.draw_items()[0].label == "Play");
}

void test_value_widgets_and_text() {
    AppState state;
    std::vector<glayout::Layout> layouts = make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    menu.register_screen(kSettings, build_settings);

    assert(menu.set_root(kSettings));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.focus() == kBack);

    gmenu::Input mouse_down;
    mouse_down.mouse_valid = true;
    mouse_down.mouse_x = 90.0f;
    mouse_down.mouse_y = 80.0f;
    mouse_down.mouse_down = true;
    menu.update(mouse_down, 0.016f, 800, 600);

    gmenu::Input mouse_up = mouse_down;
    mouse_up.mouse_down = false;
    menu.update(mouse_up, 0.016f, 800, 600);
    assert(state.fullscreen);

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(!state.fullscreen);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    assert(menu.focus() == 21);

    gmenu::Input right;
    right.right = true;
    menu.update(right, 0.016f, 800, 600);
    assert(state.volume == 0.75f);

    menu.update(down, 0.4f, 800, 600);
    assert(menu.focus() == 23);
    menu.update(right, 0.016f, 800, 600);
    assert(state.quality == 1);

    menu.update(down, 0.4f, 800, 600);
    assert(menu.focus() == 22);
    menu.update(select, 0.016f, 800, 600);
    gmenu::Input text;
    text.text = " One";
    menu.update(text, 0.016f, 800, 600);
    assert(state.name == "Player One");
    text = gmenu::Input{};
    text.backspace = true;
    menu.update(text, 0.016f, 800, 600);
    assert(state.name == "Player On");

    bool saw_editing = false;
    bool saw_value = false;
    for (const gmenu::DrawItem& item : menu.draw_items()) {
        if (item.id == 22) {
            saw_editing = item.state.editing;
            saw_value = item.value == "Player On";
        }
    }
    assert(saw_editing);
    assert(saw_value);
}

void test_ginput_mapping() {
    ginput::FrameState frame;
    frame.resize_actions(8);
    frame.begin_frame();
    frame.set_down(1, true);

    gmenu::InputActionMap map;
    map.select = 1;
    gmenu::Input input = gmenu::input_from_ginput(frame, map);
    assert(input.select);
    assert(!input.back);
}

} // namespace

int main() {
    test_stack_and_commands();
    test_value_widgets_and_text();
    test_ginput_mapping();
    return 0;
}
