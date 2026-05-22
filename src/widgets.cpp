#include "gmenu/widgets.hpp"

namespace gmenu {

Widget label(WidgetId id, std::string_view slot, std::string_view text) {
    Widget widget;
    widget.id = id;
    widget.type = WidgetType::Label;
    widget.slot = std::string(slot);
    widget.label = std::string(text);
    return widget;
}

Widget button(WidgetId id, std::string_view slot, std::string_view text, Action on_select) {
    Widget widget;
    widget.id = id;
    widget.type = WidgetType::Button;
    widget.slot = std::string(slot);
    widget.label = std::string(text);
    widget.on_select = on_select;
    return widget;
}

Widget card(WidgetId id, std::string_view slot, std::string_view title, std::string_view body,
            Action on_select) {
    Widget widget = button(id, slot, title, on_select);
    widget.type = WidgetType::Card;
    widget.secondary = std::string(body);
    return widget;
}

Widget toggle(WidgetId id, std::string_view slot, std::string_view text, bool& value) {
    Widget widget = button(id, slot, text, Action::none());
    widget.type = WidgetType::Toggle;
    widget.bool_value = &value;
    return widget;
}

Widget slider_1d(WidgetId id, std::string_view slot, std::string_view text, float& value, float min,
                 float max, float step) {
    Widget widget = button(id, slot, text, Action::none());
    widget.type = WidgetType::Slider1D;
    widget.float_value = &value;
    widget.min = min;
    widget.max = max;
    widget.step = step;
    return widget;
}

Widget option_cycle(WidgetId id, std::string_view slot, std::string_view text, int& option_index,
                    std::vector<std::string> options) {
    Widget widget = button(id, slot, text, Action::none());
    widget.type = WidgetType::OptionCycle;
    widget.option_index = &option_index;
    widget.option_count = static_cast<int>(options.size());
    widget.options = std::move(options);
    return widget;
}

Widget text_input(WidgetId id, std::string_view slot, std::string_view text, std::string& value,
                  int max_len) {
    Widget widget = button(id, slot, text, Action::none());
    widget.type = WidgetType::TextInput;
    widget.text_value = &value;
    widget.text_max_len = max_len;
    return widget;
}

} // namespace gmenu
