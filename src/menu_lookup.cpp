#include "gmenu/menu.hpp"

namespace gmenu {

namespace {

bool contains(glayout::Rect rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

} // namespace

WidgetId Menu::first_selectable(const Screen& screen) const {
    for (const Widget& widget : screen.widgets) {
        if (is_selectable(widget)) {
            return widget.id;
        }
    }
    return invalid_widget;
}

const Widget* Menu::find_widget(const Screen& screen, WidgetId id) const {
    for (const Widget& widget : screen.widgets) {
        if (widget.id == id) {
            return &widget;
        }
    }
    return nullptr;
}

Widget* Menu::find_widget(Screen& screen, WidgetId id) const {
    for (Widget& widget : screen.widgets) {
        if (widget.id == id) {
            return &widget;
        }
    }
    return nullptr;
}

bool Menu::is_selectable(const Widget& widget) const {
    if (widget.disabled) {
        return false;
    }
    return widget.type != WidgetType::Label;
}

const DrawItem* Menu::find_draw_item(WidgetId id) const {
    for (const DrawItem& item : items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

ControlPart Menu::hit_control_part(const DrawItem& item, float x, float y) const {
    if (item.controls.has_slider_track && contains(item.controls.slider_track, x, y)) {
        return ControlPart::SliderTrack;
    }
    if (item.controls.has_option_left && contains(item.controls.option_left, x, y)) {
        return ControlPart::OptionLeft;
    }
    if (item.controls.has_option_right && contains(item.controls.option_right, x, y)) {
        return ControlPart::OptionRight;
    }
    if (item.controls.has_option_value && contains(item.controls.option_value, x, y)) {
        return ControlPart::OptionValue;
    }
    if (contains(item.rect, x, y)) {
        return ControlPart::Body;
    }
    return ControlPart::None;
}

} // namespace gmenu
