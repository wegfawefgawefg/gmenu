#pragma once

#include "gmenu/menu.hpp"

namespace gmenu::imgui {

struct NavEditorState {
    WidgetId source = invalid_widget;
    WidgetId target = invalid_widget;
    ScreenId selected_screen = invalid_screen;
    NavDirection direction = NavDirection::Down;
    int selected_link = -1;
    bool save_requested = false;
    bool load_requested = false;
};

bool render_nav_editor(Menu& menu, NavEditorState& editor, ScreenId screen,
                       std::span<const DrawItem> items);
bool render_nav_editor_panel(Menu& menu, NavEditorState& editor, ScreenId screen,
                             std::span<const DrawItem> items);

} // namespace gmenu::imgui
