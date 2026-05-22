#pragma once

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)

#include "glayout/editor.hpp"
#include "gmenu/gmenu.hpp"
#include "gmenu/imgui.hpp"

struct DemoDebugUi {
    bool enabled = false;
    bool bar_visible = true;
    bool show_menu_metadata = false;
    bool show_layout_metadata = false;
    bool show_layout_editor = false;
    bool show_nav_editor = false;
    bool show_combined_editor = false;
    int selected_layout = 0;
    gmenu::imgui::NavEditorState nav_editor;
};

bool render_demo_debug_ui(DemoDebugUi& debug, gmenu::Menu& menu, glayout::LayoutStore& layout_store,
                          glayout::EditorState& layout_editor);

#endif
