#pragma once

#include "gmenu/types.hpp"

#include <vector>

namespace gmenu {

struct ComposedRowDef {
    std::vector<Widget> widgets;
    WidgetId nav_up = invalid_widget;
    WidgetId nav_down = invalid_widget;
    bool link_horizontal = true;
};

void append_composed_row(Screen& screen, ComposedRowDef row);

} // namespace gmenu
