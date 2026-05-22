#pragma once

#include "gmenu/types.hpp"

#include <string_view>

namespace gmenu {

Widget label(WidgetId id, std::string_view slot, std::string_view text);
Widget button(WidgetId id, std::string_view slot, std::string_view text, Action on_select);
Widget card(WidgetId id, std::string_view slot, std::string_view title, std::string_view body,
            Action on_select);
Widget toggle(WidgetId id, std::string_view slot, std::string_view text, bool& value);
Widget slider_1d(WidgetId id, std::string_view slot, std::string_view text, float& value, float min,
                 float max, float step);
Widget text_input(WidgetId id, std::string_view slot, std::string_view text, std::string& value,
                  int max_len);

} // namespace gmenu
