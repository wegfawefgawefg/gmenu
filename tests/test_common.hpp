#pragma once

#include "gmenu/gmenu.hpp"

#include <string>
#include <vector>

namespace gmenu_test {

struct AppState {
    bool command_ran = false;
    bool fullscreen = false;
    float volume = 0.5f;
    int quality = 0;
    std::string name = "Player";
};

inline std::vector<glayout::Layout> make_layouts() {
    glayout::Layout layout;
    layout.id = 100;
    layout.label = "test";
    layout.width = 800;
    layout.height = 600;
    layout.objects.push_back(glayout::Object{1, "play", glayout::Rect{0.1f, 0.1f, 0.8f, 0.1f}});
    layout.objects.push_back(
        glayout::Object{2, "settings", glayout::Rect{0.1f, 0.25f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{3, "name", glayout::Rect{0.1f, 0.4f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{4, "back", glayout::Rect{0.1f, 0.55f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{5, "quality", glayout::Rect{0.1f, 0.7f, 0.8f, 0.1f}});
    layout.objects.push_back(glayout::Object{6, "prev", glayout::Rect{0.1f, 0.82f, 0.1f, 0.08f}});
    layout.objects.push_back(glayout::Object{7, "next", glayout::Rect{0.8f, 0.82f, 0.1f, 0.08f}});
    layout.objects.push_back(glayout::Object{8, "page", glayout::Rect{0.35f, 0.82f, 0.3f, 0.08f}});
    return {layout};
}

} // namespace gmenu_test
