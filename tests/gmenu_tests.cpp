#include "gmenu/gmenu.hpp"
#include "test_common.hpp"

#include <cassert>
#include <filesystem>
#include <utility>
#include <vector>

namespace {

constexpr gmenu::ScreenId kMain = 1;
constexpr gmenu::ScreenId kSettings = 2;
constexpr gmenu::WidgetId kPlay = 10;
constexpr gmenu::WidgetId kSettingsButton = 11;
constexpr gmenu::WidgetId kBack = 12;

void command_mark(gmenu::BuildContext& ctx, int payload) {
    (void)payload;
    auto* state = static_cast<gmenu_test::AppState*>(ctx.user);
    state->command_ran = true;
}

gmenu::CommandId g_mark_command = gmenu::invalid_command;
gmenu::CommandId g_commit_command = gmenu::invalid_command;
int g_commit_count = 0;

void command_commit(gmenu::BuildContext&, int) {
    ++g_commit_count;
}

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
    auto* state = static_cast<gmenu_test::AppState*>(ctx.user);
    out.id = kSettings;
    out.layout_id = 100;
    out.default_focus = kBack;
    out.widgets.push_back(gmenu::toggle(20, "play", "Fullscreen", state->fullscreen));
    out.widgets.push_back(
        gmenu::slider_1d(21, "settings", "Volume", state->volume, 0.0f, 1.0f, 0.25f));
    out.widgets.push_back(
        gmenu::option_cycle(23, "quality", "Quality", state->quality, {"low", "medium", "high"}));
    gmenu::Widget name = gmenu::text_input(22, "name", "Name", state->name, 32);
    name.on_commit = gmenu::Action::command_id(g_commit_command);
    out.widgets.push_back(std::move(name));
    out.widgets.push_back(gmenu::button(kBack, "back", "Back", gmenu::Action::pop()));
    out.widgets[0].nav_down = 21;
    out.widgets[1].nav_up = 20;
    out.widgets[1].nav_down = 23;
    out.widgets[2].nav_up = 21;
    out.widgets[2].nav_down = 22;
    out.widgets[3].nav_up = 23;
    out.widgets[3].nav_down = kBack;
    out.widgets[4].nav_up = 22;
}

void test_stack_and_commands() {
    gmenu_test::AppState state;
    glayout::LayoutStore layout_store;
    layout_store.layouts = gmenu_test::make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layout_store(&layout_store);
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

    gmenu::Input right;
    right.right = true;
    menu.update(right, 0.016f, 800, 600);
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
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    g_commit_count = 0;
    g_commit_command = menu.register_command(command_commit);
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

    gmenu::Input slider_mouse;
    slider_mouse.mouse_valid = true;
    slider_mouse.mouse_x = 720.0f;
    slider_mouse.mouse_y = 180.0f;
    slider_mouse.mouse_down = true;
    menu.update(slider_mouse, 0.016f, 800, 600);
    slider_mouse.mouse_down = false;
    menu.update(slider_mouse, 0.016f, 800, 600);
    assert(state.volume > 0.95f);

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

    menu.update(select, 0.016f, 800, 600);
    saw_editing = false;
    for (const gmenu::DrawItem& item : menu.draw_items()) {
        if (item.id == 22) {
            saw_editing = item.state.editing;
        }
    }
    assert(!saw_editing);
    assert(g_commit_count == 1);
}

struct FeedbackCounts {
    int focus_moved = 0;
    int activated = 0;
    int rejected = 0;
    int adjusted_left = 0;
    int adjusted_right = 0;
    int text_started = 0;
    int text_ended = 0;
    gmenu::WidgetId last_from = gmenu::invalid_widget;
    gmenu::WidgetId last_to = gmenu::invalid_widget;
    gmenu::WidgetId last_widget = gmenu::invalid_widget;
};

void feedback_focus_moved(void* user, gmenu::WidgetId from, gmenu::WidgetId to) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->focus_moved;
    counts->last_from = from;
    counts->last_to = to;
}

void feedback_widget_counter(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->activated;
    counts->last_widget = widget;
}

void feedback_rejected(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->rejected;
    counts->last_widget = widget;
}

void feedback_left(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->adjusted_left;
    counts->last_widget = widget;
}

void feedback_right(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->adjusted_right;
    counts->last_widget = widget;
}

void feedback_text_started(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->text_started;
    counts->last_widget = widget;
}

void feedback_text_ended(void* user, gmenu::WidgetId widget) {
    auto* counts = static_cast<FeedbackCounts*>(user);
    ++counts->text_ended;
    counts->last_widget = widget;
}

void test_feedback_hooks() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    FeedbackCounts counts;
    gmenu::FeedbackHooks hooks;
    hooks.user = &counts;
    hooks.focus_moved = feedback_focus_moved;
    hooks.activated = feedback_widget_counter;
    hooks.rejected = feedback_rejected;
    hooks.adjusted_left = feedback_left;
    hooks.adjusted_right = feedback_right;
    hooks.text_edit_started = feedback_text_started;
    hooks.text_edit_ended = feedback_text_ended;

    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    menu.set_feedback_hooks(&hooks);
    g_commit_count = 0;
    g_commit_command = menu.register_command(command_commit);
    menu.register_screen(kSettings, build_settings);

    assert(menu.set_root(kSettings));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(counts.focus_moved == 1);
    assert(counts.last_from == gmenu::invalid_widget);
    assert(counts.last_to == kBack);

    gmenu::Input hover_first;
    hover_first.mouse_valid = true;
    hover_first.mouse_x = 90.0f;
    hover_first.mouse_y = 80.0f;
    menu.update(hover_first, 0.016f, 800, 600);
    assert(counts.focus_moved == 2);
    assert(counts.last_from == kBack);
    assert(counts.last_to == 20);

    gmenu::Input right;
    right.right = true;
    menu.update(right, 0.016f, 800, 600);
    assert(counts.rejected == 1);
    assert(counts.last_widget == 20);

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(counts.activated == 1);
    assert(counts.last_widget == 20);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.4f, 800, 600);
    assert(menu.focus() == 21);
    menu.update(right, 0.016f, 800, 600);
    assert(counts.adjusted_right == 1);
    assert(counts.last_widget == 21);

    menu.update(down, 0.4f, 800, 600);
    menu.update(down, 0.4f, 800, 600);
    assert(menu.focus() == 22);
    menu.update(select, 0.016f, 800, 600);
    assert(counts.text_started == 1);
    assert(counts.last_widget == 22);

    gmenu::Input text;
    text.text = "!";
    menu.update(text, 0.016f, 800, 600);
    menu.update(select, 0.016f, 800, 600);
    assert(counts.text_ended == 1);
    assert(counts.last_widget == 22);
    assert(g_commit_count == 1);
}

void test_mouse_focus_lock() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    menu.register_screen(kMain, build_main);

    assert(menu.set_root(kMain));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.focus() == kPlay);

    gmenu::Input mouse_over_settings;
    mouse_over_settings.mouse_valid = true;
    mouse_over_settings.mouse_x = 90.0f;
    mouse_over_settings.mouse_y = 170.0f;
    menu.update(mouse_over_settings, 0.016f, 800, 600);
    assert(menu.focus() == kSettingsButton);

    gmenu::Input keyboard_up = mouse_over_settings;
    keyboard_up.up = true;
    menu.update(keyboard_up, 0.016f, 800, 600);
    assert(menu.focus() == kPlay);

    menu.update(mouse_over_settings, 0.016f, 800, 600);
    assert(menu.focus() == kPlay);

    gmenu::Input moved_mouse = mouse_over_settings;
    moved_mouse.mouse_x += 1.0f;
    menu.update(moved_mouse, 0.016f, 800, 600);
    assert(menu.focus() == kSettingsButton);
}

void test_nav_overrides() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    menu.register_screen(kMain, build_main);

    menu.set_nav_link(kMain, kPlay, gmenu::NavDirection::Down, kPlay);
    assert(menu.nav_links(kMain, kPlay).down == kPlay);
    assert(menu.set_root(kMain));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);

    bool saw_override = false;
    bool saw_no_fallback = false;
    for (const gmenu::DrawItem& item : menu.draw_items()) {
        if (item.id == kPlay) {
            saw_override = item.nav_down == kPlay;
            saw_no_fallback = item.nav_up == gmenu::invalid_widget &&
                              item.nav_up_source == gmenu::NavSource::None;
        }
    }
    assert(saw_override);
    assert(saw_no_fallback);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    assert(menu.focus() == kPlay);

    menu.clear_nav_link(kMain, kPlay, gmenu::NavDirection::Down);
    assert(menu.nav_links(kMain, kPlay).down == gmenu::invalid_widget);

    menu.set_nav_link(kMain, kPlay, gmenu::NavDirection::Down, 9999);
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    std::vector<gmenu::NavValidationIssue> issues =
        menu.validate_nav_overrides(kMain, menu.draw_items());
    assert(!issues.empty());
}

void test_nav_persistence() {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / "gmenu_nav_persistence_test.lisp";

    gmenu::Menu menu;
    menu.set_nav_link(kMain, kPlay, gmenu::NavDirection::Down, kSettingsButton);
    assert(menu.nav_dirty());
    assert(menu.save_nav_file(path));
    menu.mark_nav_saved();
    assert(!menu.nav_dirty());

    gmenu::Menu loaded;
    assert(loaded.load_nav_file(path));
    assert(loaded.nav_links(kMain, kPlay).down == kSettingsButton);
    assert(!loaded.nav_dirty());

    std::filesystem::remove(path);
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
    test_feedback_hooks();
    test_mouse_focus_lock();
    test_nav_overrides();
    test_nav_persistence();
    test_ginput_mapping();
    return 0;
}
