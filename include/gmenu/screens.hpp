#pragma once

#include "gmenu/menu.hpp"

#include <string>
#include <utility>
#include <vector>

namespace gmenu {

struct BasicScreenDef {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId title_id = invalid_widget;
    std::string title_slot = "title";
    std::string title;
    StyleId title_style = default_style;
    WidgetId default_focus = invalid_widget;
    std::vector<Widget> widgets;
};

struct ListItem {
    WidgetId id = invalid_widget;
    std::string slot;
    std::string label;
    std::string secondary;
    Action action;
    StyleId style = default_style;
    bool disabled = false;
};

struct ListScreenDef {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId title_id = invalid_widget;
    std::string title_slot = "title";
    std::string title;
    StyleId title_style = default_style;
    WidgetId default_focus = invalid_widget;
    std::vector<ListItem> items;
};

void build_basic_screen(BuildContext& ctx, Screen& out);
void build_list_screen(BuildContext& ctx, Screen& out);

void register_basic_screen(Menu& menu, const BasicScreenDef& def);
void register_list_screen(Menu& menu, const ListScreenDef& def);

} // namespace gmenu
