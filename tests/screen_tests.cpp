#include "gmenu/gmenu.hpp"
#include "test_common.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace {

int g_last_edit_action = -1;
int g_last_add_bind_action = -1;

void command_edit_action(gmenu::BuildContext&, int payload) {
    g_last_edit_action = payload;
}

void command_add_bind(gmenu::BuildContext&, int payload) {
    g_last_add_bind_action = payload;
}

void test_canned_screens() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();

    gmenu::ListScreenDef main_def;
    main_def.id = 50;
    main_def.layout_id = 100;
    main_def.title_id = 500;
    main_def.title = "Main";
    main_def.default_focus = 501;
    main_def.items.push_back(
        gmenu::ListItem{501, "play", "Profiles", "Pick a profile", gmenu::Action::push(51)});

    gmenu::BasicScreenDef profile_def;
    profile_def.id = 51;
    profile_def.layout_id = 100;
    profile_def.title_id = 510;
    profile_def.title = "Profiles";
    profile_def.default_focus = 511;
    profile_def.widgets.push_back(gmenu::text_input(511, "name", "Name", state.name, 32));
    profile_def.widgets.push_back(gmenu::button(512, "back", "Back", gmenu::Action::pop()));

    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    gmenu::register_list_screen(menu, main_def);
    gmenu::register_basic_screen(menu, profile_def);
    assert(menu.registered_screens().size() == 2);
    assert(menu.registered_screens()[0].id == 50);
    assert(menu.registered_screens()[1].id == 51);

    assert(menu.set_root(50));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items().size() == 2);
    assert(menu.draw_items()[0].label == "Main");
    assert(menu.draw_items()[1].secondary == "Pick a profile");

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(menu.current_screen() == 51);
    assert(menu.draw_items()[0].label == "Profiles");
}

void test_paged_list_screen() {
    int page = 0;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();

    gmenu::PagedListScreenDef def;
    def.id = 60;
    def.layout_id = 100;
    def.title_id = 600;
    def.title = "Saves";
    def.default_focus = 601;
    def.page = &page;
    def.items_per_page = 2;
    def.item_slots = {"play", "settings"};
    def.page_label_id = 610;
    def.prev_id = 611;
    def.next_id = 612;
    def.items.push_back(gmenu::ListItem{601, "", "Save 1", "forest", gmenu::Action::none()});
    def.items.push_back(gmenu::ListItem{602, "", "Save 2", "cave", gmenu::Action::none()});
    def.items.push_back(gmenu::ListItem{603, "", "Save 3", "castle", gmenu::Action::none()});

    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    gmenu::register_paged_list_screen(menu, def);
    assert(menu.set_root(60));

    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items().size() == 6);
    assert(menu.draw_items()[1].label == "Save 1");
    assert(menu.draw_items()[2].label == "Save 2");
    assert(menu.draw_items()[3].label == "Page 1 / 2");

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    menu.update(down, 0.4f, 800, 600);

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);

    assert(page == 1);
    assert(menu.draw_items()[1].label == "Save 3");
    assert(menu.draw_items()[2].label == "Page 2 / 2");
}

void test_bind_action_list_screen() {
    int page = 0;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    ginput::Schema schema;
    schema.add_action(0, "Jump", "Gameplay");
    schema.add_action(1, "Use", "Gameplay");
    schema.add_action(2, "Menu Up", "Menu");

    ginput::InputProfile profile;
    profile.id = 1;
    profile.name = "Keyboard";
    ginput::add_button_bind(profile, ginput::ButtonBind{100, 0});
    ginput::add_button_bind(profile, ginput::ButtonBind{101, 0});
    ginput::add_button_bind(profile, ginput::ButtonBind{102, 1});

    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    gmenu::CommandId edit_command = menu.register_command(command_edit_action);

    gmenu::BindActionListScreenDef def;
    def.id = 70;
    def.layout_id = 100;
    def.title_id = 700;
    def.title = "Binds";
    def.default_focus = 1000;
    def.schema = &schema;
    def.profile = &profile;
    def.page = &page;
    def.items_per_page = 2;
    def.item_slots = {"play", "settings"};
    def.edit_command = edit_command;
    def.page_label_id = 710;
    def.prev_id = 711;
    def.next_id = 712;

    gmenu::register_bind_action_list_screen(menu, def);
    assert(menu.set_root(70));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items()[1].label == "Jump");
    assert(menu.draw_items()[1].secondary == "2 button binds");
    assert(menu.draw_items()[2].label == "Use");
    assert(menu.draw_items()[2].secondary == "1 button bind");

    g_last_edit_action = -1;
    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(g_last_edit_action == 0);
}

void test_bind_action_edit_screen() {
    int page = 0;
    ginput::ActionId action = 0;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    ginput::Schema schema;
    schema.add_action(0, "Jump", "Gameplay");

    ginput::InputProfile profile;
    profile.id = 1;
    profile.name = "Keyboard";
    const ginput::EncodedControl jump_a =
        ginput::encode_button(ginput::DeviceButton{ginput::DeviceKind::Keyboard, 0, 30});
    const ginput::EncodedControl jump_b =
        ginput::encode_button(ginput::DeviceButton{ginput::DeviceKind::Keyboard, 0, 31});
    ginput::add_button_bind(profile, ginput::ButtonBind{jump_a, action});
    ginput::add_button_bind(profile, ginput::ButtonBind{jump_b, action});

    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    gmenu::CommandId add_command = menu.register_command(command_add_bind);

    gmenu::BindActionEditScreenDef def;
    def.id = 80;
    def.layout_id = 100;
    def.title_id = 800;
    def.default_focus = static_cast<gmenu::WidgetId>(2000 + jump_a);
    def.schema = &schema;
    def.profile = &profile;
    def.action = &action;
    def.page = &page;
    def.items_per_page = 2;
    def.item_slots = {"play", "settings"};
    def.add_command = add_command;
    def.add_id = 810;
    def.add_slot = "quality";
    def.page_label_id = 811;

    gmenu::register_bind_action_edit_screen(menu, def);
    assert(menu.set_root(80));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items()[0].label == "Edit Jump");
    assert(ginput::button_binds_for_action(profile, action).size() == 2);

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(ginput::button_binds_for_action(profile, action).size() == 1);

    g_last_add_bind_action = -1;
    gmenu::Input mouse;
    mouse.mouse_valid = true;
    mouse.mouse_x = 100.0f;
    mouse.mouse_y = 430.0f;
    mouse.mouse_down = true;
    menu.update(mouse, 0.016f, 800, 600);
    mouse.mouse_down = false;
    menu.update(mouse, 0.016f, 800, 600);
    assert(g_last_add_bind_action == 0);
}

void test_settings_screen() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();

    gmenu::SettingsScreenDef def;
    def.id = 90;
    def.layout_id = 100;
    def.title_id = 900;
    def.title = "Video";
    def.default_focus = 901;

    gmenu::SettingItem fullscreen;
    fullscreen.id = 901;
    fullscreen.slot = "play";
    fullscreen.label = "Fullscreen";
    fullscreen.type = gmenu::SettingType::Toggle;
    fullscreen.bool_value = &state.fullscreen;
    def.items.push_back(fullscreen);

    gmenu::SettingItem volume;
    volume.id = 902;
    volume.slot = "settings";
    volume.label = "Volume";
    volume.type = gmenu::SettingType::Slider1D;
    volume.float_value = &state.volume;
    volume.min = 0.0f;
    volume.max = 1.0f;
    volume.step = 0.25f;
    def.items.push_back(volume);

    gmenu::SettingItem quality;
    quality.id = 903;
    quality.slot = "quality";
    quality.label = "Quality";
    quality.type = gmenu::SettingType::OptionCycle;
    quality.option_index = &state.quality;
    quality.options = {"low", "medium", "high"};
    def.items.push_back(quality);

    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    gmenu::register_settings_screen(menu, def);
    assert(menu.set_root(90));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items()[0].label == "Video");
    assert(menu.draw_items()[1].value == "off");

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(state.fullscreen);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    gmenu::Input right;
    right.right = true;
    menu.update(right, 0.016f, 800, 600);
    assert(state.volume == 0.75f);
}

void test_profile_list_screen() {
    int page = 0;
    int selected = -1;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();
    std::vector<gmenu::ProfileEntry> profiles{
        {10, "Default", "keyboard"},
        {11, "Guest", "gamepad"},
        {12, "Speedrun", "custom"},
    };

    gmenu::ProfileListScreenDef def;
    def.id = 91;
    def.layout_id = 100;
    def.title_id = 910;
    def.title = "Profiles";
    def.default_focus = 3010;
    def.profiles = &profiles;
    def.selected_profile_id = &selected;
    def.page = &page;
    def.items_per_page = 2;
    def.item_slots = {"play", "settings"};
    def.page_label_id = 911;
    def.prev_id = 912;
    def.next_id = 913;

    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    gmenu::register_profile_list_screen(menu, def);
    assert(menu.set_root(91));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.draw_items()[0].label == "Profiles");
    assert(menu.draw_items()[1].label == "Default");
    assert(menu.draw_items()[2].label == "Guest");

    gmenu::Input select;
    select.select = true;
    menu.update(select, 0.016f, 800, 600);
    assert(selected == 10);
    assert(menu.draw_items()[1].value == "selected");

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    menu.update(down, 0.4f, 800, 600);
    menu.update(select, 0.016f, 800, 600);
    assert(page == 1);
    assert(menu.draw_items()[1].label == "Speedrun");
}

void build_composed_row_screen(gmenu::BuildContext& ctx, gmenu::Screen& out) {
    auto* state = static_cast<gmenu_test::AppState*>(ctx.user);
    out.id = 92;
    out.layout_id = 100;
    out.default_focus = 920;
    gmenu::Widget above = gmenu::button(920, "play", "Above", gmenu::Action::none());
    above.nav_down = 921;
    out.widgets.push_back(std::move(above));

    gmenu::ComposedRowDef row;
    row.nav_up = 920;
    row.nav_down = 924;
    row.widgets.push_back(gmenu::text_input(921, "prev", "Width", state->name, 8));
    row.widgets.push_back(gmenu::text_input(922, "page", "Height", state->name, 8));
    row.widgets.push_back(gmenu::button(923, "next", "Apply", gmenu::Action::none()));
    gmenu::append_composed_row(out, std::move(row));

    out.widgets.push_back(gmenu::button(924, "back", "Below", gmenu::Action::none()));
}

void test_composed_row_helper() {
    gmenu_test::AppState state;
    std::vector<glayout::Layout> layouts = gmenu_test::make_layouts();

    gmenu::Menu menu;
    menu.set_user_data(&state);
    menu.set_layouts(&layouts);
    menu.register_screen(92, build_composed_row_screen);
    assert(menu.set_root(92));
    menu.update(gmenu::Input{}, 0.016f, 800, 600);
    assert(menu.focus() == 920);

    gmenu::Input down;
    down.down = true;
    menu.update(down, 0.016f, 800, 600);
    assert(menu.focus() == 921);

    gmenu::Input right;
    right.right = true;
    menu.update(right, 0.016f, 800, 600);
    assert(menu.focus() == 922);
    menu.update(right, 0.4f, 800, 600);
    assert(menu.focus() == 923);

    gmenu::Input left;
    left.left = true;
    menu.update(left, 0.016f, 800, 600);
    assert(menu.focus() == 922);

    menu.update(down, 0.4f, 800, 600);
    assert(menu.focus() == 924);
}

} // namespace

int main() {
    test_canned_screens();
    test_paged_list_screen();
    test_bind_action_list_screen();
    test_bind_action_edit_screen();
    test_settings_screen();
    test_profile_list_screen();
    test_composed_row_helper();
    return 0;
}
