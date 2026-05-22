#pragma once

#include "ginput/profile.hpp"
#include "ginput/schema.hpp"
#include "gmenu/menu.hpp"

#include <string>
#include <vector>

namespace gmenu {

struct BindActionListScreenDef {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId title_id = invalid_widget;
    std::string title_slot = "title";
    std::string title = "Binds";
    StyleId title_style = default_style;
    WidgetId default_focus = invalid_widget;
    const ginput::Schema* schema = nullptr;
    const ginput::InputProfile* profile = nullptr;
    int* page = nullptr;
    int items_per_page = 0;
    std::vector<std::string> item_slots;
    CommandId edit_command = invalid_command;
    WidgetId page_label_id = invalid_widget;
    std::string page_label_slot = "page";
    WidgetId prev_id = invalid_widget;
    std::string prev_slot = "prev";
    WidgetId next_id = invalid_widget;
    std::string next_slot = "next";
    WidgetId back_id = invalid_widget;
    std::string back_slot = "back";
    std::string back_label = "Back";
    StyleId nav_style = default_style;
    StyleId item_style = default_style;
};

struct BindActionEditScreenDef {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId title_id = invalid_widget;
    std::string title_slot = "title";
    std::string title_prefix = "Edit ";
    StyleId title_style = default_style;
    WidgetId default_focus = invalid_widget;
    const ginput::Schema* schema = nullptr;
    ginput::InputProfile* profile = nullptr;
    ginput::ActionId* action = nullptr;
    int* page = nullptr;
    int items_per_page = 0;
    std::vector<std::string> item_slots;
    CommandId add_command = invalid_command;
    WidgetId add_id = invalid_widget;
    std::string add_slot = "add";
    std::string add_label = "Add bind";
    WidgetId page_label_id = invalid_widget;
    std::string page_label_slot = "page";
    WidgetId prev_id = invalid_widget;
    std::string prev_slot = "prev";
    WidgetId next_id = invalid_widget;
    std::string next_slot = "next";
    WidgetId back_id = invalid_widget;
    std::string back_slot = "back";
    std::string back_label = "Back";
    StyleId nav_style = default_style;
    StyleId item_style = default_style;
};

void build_bind_action_list_screen(BuildContext& ctx, Screen& out);
void build_bind_action_edit_screen(BuildContext& ctx, Screen& out);
void register_bind_action_list_screen(Menu& menu, const BindActionListScreenDef& def);
void register_bind_action_edit_screen(Menu& menu, const BindActionEditScreenDef& def);

} // namespace gmenu
