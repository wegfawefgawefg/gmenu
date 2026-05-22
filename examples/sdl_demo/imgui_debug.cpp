#include "imgui_debug.hpp"

#if defined(GMENU_SDL_DEMO_WITH_IMGUI)

#include "glayout/imgui.hpp"

#include <algorithm>
#include <cstddef>
#include <imgui.h>
#include <string>

namespace {

const char* widget_type_text(gmenu::WidgetType type) {
    switch (type) {
    case gmenu::WidgetType::Label:
        return "label";
    case gmenu::WidgetType::Button:
        return "button";
    case gmenu::WidgetType::Card:
        return "card";
    case gmenu::WidgetType::Toggle:
        return "toggle";
    case gmenu::WidgetType::Slider1D:
        return "slider";
    case gmenu::WidgetType::OptionCycle:
        return "option";
    case gmenu::WidgetType::TextInput:
        return "text";
    }
    return "unknown";
}

const char* nav_source_text(gmenu::NavSource source) {
    switch (source) {
    case gmenu::NavSource::None:
        return "none";
    case gmenu::NavSource::Explicit:
        return "explicit";
    case gmenu::NavSource::Override:
        return "override";
    }
    return "none";
}

void render_debug_bar(DemoDebugUi& debug) {
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(620.0f, 78.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("gmenu debug", &debug.bar_visible,
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Ctrl+L layout overlay, Ctrl+N nav overlay, F10 debug, F9 bar");
    ImGui::Checkbox("Menu metadata", &debug.show_menu_metadata);
    ImGui::SameLine();
    ImGui::Checkbox("Layout metadata", &debug.show_layout_metadata);
    ImGui::SameLine();
    ImGui::Checkbox("Layout editor", &debug.show_layout_editor);
    ImGui::SameLine();
    ImGui::Checkbox("Nav editor", &debug.show_nav_editor);
    ImGui::SameLine();
    ImGui::Checkbox("Combined editor", &debug.show_combined_editor);

    ImGui::End();
}

void render_menu_metadata_panel(gmenu::Menu& menu) {
    ImGui::Text("Current screen: %u", menu.current_screen());
    ImGui::Text("Focus: %u", menu.focus());
    ImGui::Text("Draw items: %zu", menu.draw_items().size());
    ImGui::Text("Nav overrides: %zu", menu.nav_overrides().size());
    ImGui::SameLine();
    ImGui::TextUnformatted(menu.nav_dirty() ? "dirty" : "clean");

    if (ImGui::BeginTable("draw_items", 10,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 280.0f))) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Label");
        ImGui::TableSetupColumn("Rect");
        ImGui::TableSetupColumn("Up");
        ImGui::TableSetupColumn("Down");
        ImGui::TableSetupColumn("Left");
        ImGui::TableSetupColumn("Right");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        for (const gmenu::DrawItem& item : menu.draw_items()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", item.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(widget_type_text(item.type));
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(item.label.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f %.0f %.0f %.0f", item.rect.x, item.rect.y, item.rect.w, item.rect.h);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u/%s", item.nav_up, nav_source_text(item.nav_up_source));
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u/%s", item.nav_down, nav_source_text(item.nav_down_source));
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%u/%s", item.nav_left, nav_source_text(item.nav_left_source));
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%u/%s", item.nav_right, nav_source_text(item.nav_right_source));
            ImGui::TableSetColumnIndex(8);
            std::string state;
            if (item.state.focused)
                state += "focused ";
            if (item.state.hovered)
                state += "hovered ";
            if (item.state.pressed)
                state += "pressed ";
            if (item.state.editing)
                state += "editing ";
            ImGui::TextUnformatted(state.c_str());
            ImGui::TableSetColumnIndex(9);
            ImGui::TextUnformatted(item.value.c_str());
        }

        ImGui::EndTable();
    }
}

void render_menu_metadata(gmenu::Menu& menu, bool& open) {
    ImGui::SetNextWindowSize(ImVec2(760.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("gmenu: Menu Metadata", &open)) {
        ImGui::End();
        return;
    }

    render_menu_metadata_panel(menu);
    ImGui::End();
}

bool render_combined_editor(DemoDebugUi& debug, gmenu::Menu& menu,
                            glayout::LayoutStore& layout_store,
                            glayout::EditorState& layout_editor) {
    bool changed = false;
    ImGui::SetNextWindowSize(ImVec2(860.0f, 580.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("gmenu: Combined Editor", &debug.show_combined_editor)) {
        ImGui::End();
        return false;
    }

    if (ImGui::BeginTabBar("combined_editor_tabs")) {
        if (ImGui::BeginTabItem("Menu")) {
            render_menu_metadata_panel(menu);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Layout")) {
            ImGui::TextUnformatted("Layout rectangles are edited by glayout.");
            ImGui::Checkbox("Show glayout editor window", &debug.show_layout_editor);
            ImGui::SameLine();
            ImGui::Checkbox("Show layout metadata", &debug.show_layout_metadata);
            if (!layout_store.layouts.empty()) {
                debug.selected_layout = std::clamp(
                    debug.selected_layout, 0, static_cast<int>(layout_store.layouts.size()) - 1);
                const glayout::Layout& layout =
                    layout_store.layouts[static_cast<std::size_t>(debug.selected_layout)];
                ImGui::Text("Selected layout: #%d %s", layout.id, layout.label.c_str());
                ImGui::Text("Objects: %zu", layout.objects.size());
                if (layout_editor.dirty) {
                    ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f), "Layout dirty");
                }
            } else {
                ImGui::TextUnformatted("No layouts loaded.");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Navigation")) {
            changed = gmenu::imgui::render_nav_editor_panel(
                          menu, debug.nav_editor, menu.current_screen(), menu.draw_items()) ||
                      changed;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    return changed;
}

} // namespace

bool render_demo_debug_ui(DemoDebugUi& debug, gmenu::Menu& menu, glayout::LayoutStore& layout_store,
                          glayout::EditorState& layout_editor) {
    if (!debug.enabled) {
        return false;
    }

    bool changed = false;
    if (debug.bar_visible) {
        render_debug_bar(debug);
    }
    if (debug.show_menu_metadata) {
        render_menu_metadata(menu, debug.show_menu_metadata);
    }
    if (debug.show_layout_metadata) {
        glayout::imgui::render_layout_browser(layout_store.layouts);
    }
    if (debug.show_layout_editor) {
        changed = glayout::imgui::render_integrated_editor(layout_editor, layout_store,
                                                           debug.selected_layout) ||
                  changed;
    }
    if (debug.show_nav_editor) {
        changed = gmenu::imgui::render_nav_editor(menu, debug.nav_editor, menu.current_screen(),
                                                  menu.draw_items()) ||
                  changed;
    }
    if (debug.show_combined_editor) {
        changed = render_combined_editor(debug, menu, layout_store, layout_editor) || changed;
    }

    return changed;
}

#endif
