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

struct PagedListScreenDef {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId title_id = invalid_widget;
    std::string title_slot = "title";
    std::string title;
    StyleId title_style = default_style;
    WidgetId default_focus = invalid_widget;
    int* page = nullptr;
    int items_per_page = 0;
    std::vector<std::string> item_slots;
    std::vector<ListItem> items;
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

void build_basic_screen(BuildContext& ctx, Screen& out);
void build_list_screen(BuildContext& ctx, Screen& out);
void build_paged_list_screen(BuildContext& ctx, Screen& out);

void register_basic_screen(Menu& menu, const BasicScreenDef& def);
void register_list_screen(Menu& menu, const ListScreenDef& def);
void register_paged_list_screen(Menu& menu, const PagedListScreenDef& def);

} // namespace gmenu
