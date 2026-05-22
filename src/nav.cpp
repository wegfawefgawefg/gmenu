#include "gmenu/menu.hpp"

#include <algorithm>
#include <utility>

namespace gmenu {

namespace {

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

WidgetId widget_link_for_direction(const Widget& widget, NavDirection direction) {
    switch (direction) {
    case NavDirection::Up:
        return widget.nav_up;
    case NavDirection::Down:
        return widget.nav_down;
    case NavDirection::Left:
        return widget.nav_left;
    case NavDirection::Right:
        return widget.nav_right;
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

bool same_scope(const NavScope& a, const NavScope& b) {
    return a.screen == b.screen && a.layout_id == b.layout_id && a.width == b.width &&
           a.height == b.height && a.form_factor == b.form_factor &&
           a.match_resolution == b.match_resolution && a.match_form_factor == b.match_form_factor;
}

bool scope_matches(const NavScope& record, const NavScope& query) {
    if (record.screen != query.screen) {
        return false;
    }
    if (record.layout_id != 0 && record.layout_id != query.layout_id) {
        return false;
    }
    if (record.match_resolution && (record.width != query.width || record.height != query.height)) {
        return false;
    }
    if (record.match_form_factor && record.form_factor != query.form_factor) {
        return false;
    }
    return true;
}

int scope_score(const NavScope& scope) {
    int score = 1;
    if (scope.layout_id != 0) {
        score += 8;
    }
    if (scope.match_resolution) {
        score += 4;
    }
    if (scope.match_form_factor) {
        score += 2;
    }
    return score;
}

NavScope current_scope(const Screen& screen, int width, int height,
                       glayout::FormFactor form_factor) {
    NavScope scope;
    scope.screen = screen.id;
    scope.layout_id = screen.layout_id;
    scope.width = width;
    scope.height = height;
    scope.form_factor = form_factor;
    scope.match_resolution = true;
    scope.match_form_factor = true;
    return scope;
}

void merge_direction(NavLinks& out, int& out_score, const NavOverride& record,
                     NavDirection direction) {
    const WidgetId target = link_for_direction(record.links, direction);
    if (target == invalid_widget) {
        return;
    }
    const int score = scope_score(record.scope);
    if (score >= out_score) {
        set_link_for_direction(out, direction, target);
        out_score = score;
    }
}

bool item_exists(std::span<const DrawItem> items, WidgetId id) {
    for (const DrawItem& item : items) {
        if (item.id == id && item.type != WidgetType::Label) {
            return true;
        }
    }
    return false;
}

void validate_direction(std::vector<NavValidationIssue>& issues, std::span<const DrawItem> items,
                        const NavOverride& record, NavDirection direction) {
    const WidgetId target = link_for_direction(record.links, direction);
    if (target == invalid_widget || item_exists(items, target)) {
        return;
    }
    NavValidationIssue issue;
    issue.screen = record.scope.screen;
    issue.widget = record.widget;
    issue.target = target;
    issue.direction = direction;
    issue.message = "nav target is missing from current screen";
    issues.push_back(std::move(issue));
}

} // namespace

void Menu::set_nav_link(ScreenId screen, WidgetId widget, NavDirection direction, WidgetId target) {
    NavScope scope;
    scope.screen = screen;
    set_nav_link(scope, widget, direction, target);
}

void Menu::set_nav_link(NavScope scope, WidgetId widget, NavDirection direction, WidgetId target) {
    if (scope.screen == invalid_screen || widget == invalid_widget) {
        return;
    }
    if (target == invalid_widget) {
        clear_nav_link(scope, widget, direction);
        return;
    }

    for (NavOverride& record : nav_override_records) {
        if (record.widget == widget && same_scope(record.scope, scope)) {
            if (link_for_direction(record.links, direction) == target) {
                return;
            }
            set_link_for_direction(record.links, direction, target);
            nav_dirty_flag = true;
            return;
        }
    }

    NavOverride record;
    record.scope = scope;
    record.widget = widget;
    set_link_for_direction(record.links, direction, target);
    nav_override_records.push_back(record);
    nav_dirty_flag = true;
}

void Menu::clear_nav_link(ScreenId screen, WidgetId widget, NavDirection direction) {
    NavScope scope;
    scope.screen = screen;
    clear_nav_link(scope, widget, direction);
}

void Menu::clear_nav_link(NavScope scope, WidgetId widget, NavDirection direction) {
    auto it = std::remove_if(
        nav_override_records.begin(), nav_override_records.end(), [&](NavOverride& record) {
            if (record.widget != widget || !same_scope(record.scope, scope)) {
                return false;
            }
            if (link_for_direction(record.links, direction) == invalid_widget) {
                return false;
            }
            set_link_for_direction(record.links, direction, invalid_widget);
            nav_dirty_flag = true;
            return links_empty(record.links);
        });
    nav_override_records.erase(it, nav_override_records.end());
}

void Menu::clear_nav_links(ScreenId screen, WidgetId widget) {
    NavScope scope;
    scope.screen = screen;
    clear_nav_links(scope, widget);
}

void Menu::clear_nav_links(NavScope scope, WidgetId widget) {
    auto old_size = nav_override_records.size();
    nav_override_records.erase(
        std::remove_if(nav_override_records.begin(), nav_override_records.end(),
                       [&](const NavOverride& record) {
                           return record.widget == widget && same_scope(record.scope, scope);
                       }),
        nav_override_records.end());
    if (nav_override_records.size() != old_size) {
        nav_dirty_flag = true;
    }
}

NavLinks Menu::nav_links(ScreenId screen, WidgetId widget) const {
    NavScope scope;
    scope.screen = screen;
    return nav_links(scope, widget);
}

NavLinks Menu::nav_links(NavScope scope, WidgetId widget) const {
    NavLinks out;
    int up_score = -1;
    int down_score = -1;
    int left_score = -1;
    int right_score = -1;

    for (const NavOverride& record : nav_override_records) {
        if (record.widget != widget || !scope_matches(record.scope, scope)) {
            continue;
        }
        merge_direction(out, up_score, record, NavDirection::Up);
        merge_direction(out, down_score, record, NavDirection::Down);
        merge_direction(out, left_score, record, NavDirection::Left);
        merge_direction(out, right_score, record, NavDirection::Right);
    }
    return out;
}

std::span<const NavOverride> Menu::nav_overrides() const {
    return nav_override_records;
}

bool Menu::nav_dirty() const {
    return nav_dirty_flag;
}

void Menu::mark_nav_saved() {
    nav_dirty_flag = false;
}

std::vector<NavValidationIssue>
Menu::validate_nav_overrides(ScreenId screen, std::span<const DrawItem> draw_item_list) const {
    std::vector<NavValidationIssue> issues;
    for (const NavOverride& record : nav_override_records) {
        if (record.scope.screen != screen) {
            continue;
        }
        if (!item_exists(draw_item_list, record.widget)) {
            NavValidationIssue issue;
            issue.screen = record.scope.screen;
            issue.widget = record.widget;
            issue.message = "nav source is missing from current screen";
            issues.push_back(std::move(issue));
            continue;
        }
        validate_direction(issues, draw_item_list, record, NavDirection::Up);
        validate_direction(issues, draw_item_list, record, NavDirection::Down);
        validate_direction(issues, draw_item_list, record, NavDirection::Left);
        validate_direction(issues, draw_item_list, record, NavDirection::Right);
    }
    return issues;
}

void Menu::apply_nav_overrides(Screen& screen, int width, int height,
                               glayout::FormFactor form_factor) const {
    const NavScope scope = current_scope(screen, width, height, form_factor);
    for (Widget& widget : screen.widgets) {
        const NavLinks links = nav_links(scope, widget.id);
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

WidgetId Menu::effective_nav(const Screen& screen, const Widget& widget,
                             NavDirection direction) const {
    if (!is_selectable(widget)) {
        return invalid_widget;
    }
    const WidgetId explicit_target = widget_link_for_direction(widget, direction);
    const int step = (direction == NavDirection::Up || direction == NavDirection::Left) ? -1 : 1;
    return resolve_nav(screen, widget.id, explicit_target, step);
}

NavSource Menu::nav_source(const Screen& screen, const Widget& widget, NavDirection direction,
                           int width, int height, glayout::FormFactor form_factor) const {
    const NavScope scope = current_scope(screen, width, height, form_factor);
    const NavLinks links = nav_links(scope, widget.id);
    if (link_for_direction(links, direction) != invalid_widget) {
        return NavSource::Override;
    }
    if (widget_link_for_direction(widget, direction) != invalid_widget) {
        return NavSource::Explicit;
    }
    if (effective_nav(screen, widget, direction) != invalid_widget) {
        return NavSource::Fallback;
    }
    return NavSource::None;
}

} // namespace gmenu
