#include "gmenu/menu.hpp"

#include <algorithm>

namespace gmenu {

void Menu::execute(const Action& action) {
    switch (action.type) {
    case ActionType::None:
        break;
    case ActionType::Push:
        push(action.screen);
        break;
    case ActionType::Pop:
        pop();
        break;
    case ActionType::Replace:
        replace(action.screen);
        break;
    case ActionType::SetRoot:
        set_root(action.screen);
        break;
    case ActionType::Command:
        invoke_command(action.command, action.payload);
        break;
    case ActionType::SetInt:
        if (action.int_value) {
            *action.int_value = action.payload;
        }
        break;
    case ActionType::DeltaInt:
        if (action.int_value) {
            int min_value = action.min;
            int max_value = action.max;
            if (min_value > max_value) {
                std::swap(min_value, max_value);
            }
            *action.int_value =
                std::clamp(*action.int_value + action.payload, min_value, max_value);
        }
        break;
    case ActionType::RemoveButtonBind:
        if (action.input_profile) {
            ginput::remove_button_bind(
                *action.input_profile,
                ginput::ButtonBind{action.encoded_control, action.input_action});
        }
        break;
    }
}

void Menu::invoke_command(CommandId id, int payload) {
    if (id == invalid_command) {
        return;
    }
    std::size_t index = static_cast<std::size_t>(id - 1);
    if (index >= commands.size() || !commands[index]) {
        return;
    }
    const ScreenDef* def = find_screen(current_screen());
    BuildContext ctx{*this, current_screen(), user, def ? def->data : nullptr};
    commands[index](ctx, payload);
}

} // namespace gmenu
