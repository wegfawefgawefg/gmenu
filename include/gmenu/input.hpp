#pragma once

#include "ginput/runtime.hpp"
#include "gmenu/types.hpp"

namespace gmenu {

struct InputActionMap {
    ginput::ActionId up = -1;
    ginput::ActionId down = -1;
    ginput::ActionId left = -1;
    ginput::ActionId right = -1;
    ginput::ActionId select = -1;
    ginput::ActionId back = -1;
    ginput::ActionId page_prev = -1;
    ginput::ActionId page_next = -1;
};

Input input_from_ginput(const ginput::FrameState& frame, const InputActionMap& map);

} // namespace gmenu
