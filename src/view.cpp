#include "gmenu/menu.hpp"

#include <cstdio>

namespace gmenu {

namespace {

bool point_in_rect(float x, float y, glayout::Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

std::string float_text(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(value));
    return buffer;
}

std::string widget_value_text(const Widget& widget) {
    if (!widget.value.empty()) {
        return widget.value;
    }
    if (widget.type == WidgetType::Toggle && widget.bool_value) {
        return *widget.bool_value ? "on" : "off";
    }
    if (widget.type == WidgetType::Slider1D && widget.float_value) {
        return float_text(*widget.float_value);
    }
    if (widget.type == WidgetType::OptionCycle && widget.option_index) {
        const int index = *widget.option_index;
        if (index >= 0 && index < static_cast<int>(widget.options.size())) {
            return widget.options[static_cast<std::size_t>(index)];
        }
        return {};
    }
    if (widget.type == WidgetType::TextInput && widget.text_value) {
        return *widget.text_value;
    }
    return {};
}

} // namespace

void Menu::rebuild_draw_items(const Screen& screen, int width, int height,
                              glayout::FormFactor form_factor) {
    items.clear();
    const glayout::Layout* layout = nullptr;
    if (layout_store) {
        layout = layout_store->find_best(screen.layout_id, width, height, form_factor);
    } else if (layouts) {
        layout = glayout::find_best_layout(*layouts, screen.layout_id, width, height, form_factor);
    }

    for (const Widget& widget : screen.widgets) {
        DrawItem item;
        item.id = widget.id;
        item.type = widget.type;
        item.style = widget.style;
        item.label = widget.label;
        item.secondary = widget.secondary;
        item.value = widget_value_text(widget);
        item.nav_up = widget.nav_up;
        item.nav_down = widget.nav_down;
        item.nav_left = widget.nav_left;
        item.nav_right = widget.nav_right;
        item.state.focused = widget.id == focused;
        item.state.hovered = widget.id == hovered;
        item.state.pressed = widget.id == pressed;
        item.state.disabled = widget.disabled;
        item.state.editing = widget.id == editing;

        auto focus_it = focus_times.find(widget.id);
        auto hover_it = hover_times.find(widget.id);
        auto press_it = press_times.find(widget.id);
        item.state.focus_time = focus_it == focus_times.end() ? 0.0f : focus_it->second;
        item.state.hover_time = hover_it == hover_times.end() ? 0.0f : hover_it->second;
        item.state.press_time = press_it == press_times.end() ? 0.0f : press_it->second;

        if (layout) {
            if (const glayout::Object* object = glayout::find_object(*layout, widget.slot)) {
                item.rect = glayout::map_rect(glayout::Rect{0.0f, 0.0f, static_cast<float>(width),
                                                            static_cast<float>(height)},
                                              object->rect);
            }
        }
        items.push_back(item);
    }
}

WidgetId Menu::hovered_widget(const Screen& screen, const Input& input) const {
    if (!input.mouse_valid) {
        return invalid_widget;
    }
    for (const DrawItem& item : items) {
        if (item.id == invalid_widget || !point_in_rect(input.mouse_x, input.mouse_y, item.rect)) {
            continue;
        }
        const Widget* widget = find_widget(screen, item.id);
        if (widget && is_selectable(*widget)) {
            return widget->id;
        }
    }
    return invalid_widget;
}

} // namespace gmenu
