#include "gmenu/menu.hpp"

namespace gmenu {

WidgetId Menu::resolve_nav(const Screen& screen, WidgetId from, WidgetId explicit_target,
                           int direction) const {
    (void)from;
    (void)direction;
    if (explicit_target == invalid_widget) {
        return invalid_widget;
    }
    const Widget* target = find_widget(screen, explicit_target);
    if (!target || !is_selectable(*target)) {
        return invalid_widget;
    }
    return explicit_target;
}

WidgetId Menu::resolve_return_nav(const Screen& screen, WidgetId from,
                                  NavDirection direction) const {
    for (auto it = nav_returns.rbegin(); it != nav_returns.rend(); ++it) {
        if (it->from != from || it->direction != direction || it->target == invalid_widget) {
            continue;
        }
        const Widget* target = find_widget(screen, it->target);
        if (target && is_selectable(*target)) {
            return it->target;
        }
    }
    return invalid_widget;
}

void Menu::remember_return_nav(WidgetId from, WidgetId to, NavDirection return_direction) {
    if (from == invalid_widget || to == invalid_widget || from == to) {
        return;
    }

    for (auto it = nav_returns.begin(); it != nav_returns.end();) {
        if (it->from == to && it->direction == return_direction) {
            it = nav_returns.erase(it);
        } else {
            ++it;
        }
    }

    nav_returns.push_back(NavReturn{to, return_direction, from});
    constexpr std::size_t max_returns = 8;
    while (nav_returns.size() > max_returns) {
        nav_returns.erase(nav_returns.begin());
    }
}

void Menu::move_focus_with_return(const Screen& screen, WidgetId target,
                                  NavDirection return_direction) {
    const Widget* target_widget = find_widget(screen, target);
    if (!target_widget || !is_selectable(*target_widget)) {
        return;
    }
    const WidgetId previous = focused;
    focused = target;
    remember_return_nav(previous, target, return_direction);
}

} // namespace gmenu
