#include "gmenu/rows.hpp"

#include <cstddef>
#include <utility>

namespace gmenu {

void append_composed_row(Screen& screen, ComposedRowDef row) {
    if (row.widgets.empty()) {
        return;
    }

    for (std::size_t i = 0; i < row.widgets.size(); ++i) {
        Widget& widget = row.widgets[i];
        if (row.nav_up != invalid_widget && widget.nav_up == invalid_widget) {
            widget.nav_up = row.nav_up;
        }
        if (row.nav_down != invalid_widget && widget.nav_down == invalid_widget) {
            widget.nav_down = row.nav_down;
        }
        if (row.link_horizontal) {
            if (i > 0 && widget.nav_left == invalid_widget) {
                widget.nav_left = row.widgets[i - 1].id;
            }
            if (i + 1 < row.widgets.size() && widget.nav_right == invalid_widget) {
                widget.nav_right = row.widgets[i + 1].id;
            }
        }
    }

    for (Widget& widget : row.widgets) {
        screen.widgets.push_back(std::move(widget));
    }
}

} // namespace gmenu
