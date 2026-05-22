#pragma once

#include "gmenu/menu.hpp"

namespace gmenu::imgui {

struct NavEditorState {
    WidgetId source = invalid_widget;
    WidgetId target = invalid_widget;
    NavDirection direction = NavDirection::Down;
    int selected_override = -1;
};

bool render_nav_editor(Menu& menu, NavEditorState& editor, ScreenId screen,
                       std::span<const DrawItem> items);

} // namespace gmenu::imgui
