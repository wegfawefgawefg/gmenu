#include "gmenu/input.hpp"

namespace gmenu {

Input input_from_ginput(const ginput::FrameState& frame, const InputActionMap& map) {
    Input input;
    input.up = frame.pressed(map.up);
    input.down = frame.pressed(map.down);
    input.left = frame.pressed(map.left);
    input.right = frame.pressed(map.right);
    input.select = frame.pressed(map.select);
    input.back = frame.pressed(map.back);
    input.page_prev = frame.pressed(map.page_prev);
    input.page_next = frame.pressed(map.page_next);
    return input;
}

} // namespace gmenu
