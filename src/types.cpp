#include "gmenu/types.hpp"

namespace gmenu {

int version_major() {
    return 0;
}

Action Action::none() {
    return {};
}

Action Action::push(ScreenId id) {
    Action action;
    action.type = ActionType::Push;
    action.screen = id;
    return action;
}

Action Action::pop() {
    Action action;
    action.type = ActionType::Pop;
    return action;
}

Action Action::replace(ScreenId id) {
    Action action;
    action.type = ActionType::Replace;
    action.screen = id;
    return action;
}

Action Action::set_root(ScreenId id) {
    Action action;
    action.type = ActionType::SetRoot;
    action.screen = id;
    return action;
}

Action Action::command_id(CommandId id, int value) {
    Action action;
    action.type = ActionType::Command;
    action.command = id;
    action.payload = value;
    return action;
}

Action Action::set_int(int& value, int target) {
    Action action;
    action.type = ActionType::SetInt;
    action.int_value = &value;
    action.payload = target;
    return action;
}

Action Action::delta_int(int& value, int delta, int min_value, int max_value) {
    Action action;
    action.type = ActionType::DeltaInt;
    action.int_value = &value;
    action.payload = delta;
    action.min = min_value;
    action.max = max_value;
    return action;
}

} // namespace gmenu
