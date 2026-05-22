#include "gmenu/menu.hpp"

namespace gmenu {

namespace {

std::uint64_t nav_key(ScreenId screen, WidgetId widget) {
    return (static_cast<std::uint64_t>(screen) << 32U) | static_cast<std::uint64_t>(widget);
}

WidgetId link_for_direction(const NavLinks& links, NavDirection direction) {
    switch (direction) {
    case NavDirection::Up:
        return links.up;
    case NavDirection::Down:
        return links.down;
    case NavDirection::Left:
        return links.left;
    case NavDirection::Right:
        return links.right;
    }
    return invalid_widget;
}

void set_link_for_direction(NavLinks& links, NavDirection direction, WidgetId target) {
    switch (direction) {
    case NavDirection::Up:
        links.up = target;
        break;
    case NavDirection::Down:
        links.down = target;
        break;
    case NavDirection::Left:
        links.left = target;
        break;
    case NavDirection::Right:
        links.right = target;
        break;
    }
}

bool links_empty(const NavLinks& links) {
    return links.up == invalid_widget && links.down == invalid_widget &&
           links.left == invalid_widget && links.right == invalid_widget;
}

} // namespace

void Menu::set_nav_link(ScreenId screen, WidgetId widget, NavDirection direction, WidgetId target) {
    if (screen == invalid_screen || widget == invalid_widget) {
        return;
    }
    if (target == invalid_widget) {
        clear_nav_link(screen, widget, direction);
        return;
    }
    NavLinks& links = nav_overrides[nav_key(screen, widget)];
    set_link_for_direction(links, direction, target);
}

void Menu::clear_nav_link(ScreenId screen, WidgetId widget, NavDirection direction) {
    auto it = nav_overrides.find(nav_key(screen, widget));
    if (it == nav_overrides.end()) {
        return;
    }
    set_link_for_direction(it->second, direction, invalid_widget);
    if (links_empty(it->second)) {
        nav_overrides.erase(it);
    }
}

void Menu::clear_nav_links(ScreenId screen, WidgetId widget) {
    nav_overrides.erase(nav_key(screen, widget));
}

NavLinks Menu::nav_links(ScreenId screen, WidgetId widget) const {
    auto it = nav_overrides.find(nav_key(screen, widget));
    if (it == nav_overrides.end()) {
        return {};
    }
    return it->second;
}

void Menu::apply_nav_overrides(Screen& screen) const {
    for (Widget& widget : screen.widgets) {
        const NavLinks links = nav_links(screen.id, widget.id);
        if (link_for_direction(links, NavDirection::Up) != invalid_widget) {
            widget.nav_up = links.up;
        }
        if (link_for_direction(links, NavDirection::Down) != invalid_widget) {
            widget.nav_down = links.down;
        }
        if (link_for_direction(links, NavDirection::Left) != invalid_widget) {
            widget.nav_left = links.left;
        }
        if (link_for_direction(links, NavDirection::Right) != invalid_widget) {
            widget.nav_right = links.right;
        }
    }
}

} // namespace gmenu
