#include "gmenu/imgui.hpp"

#include <imgui.h>
#include <string>
#include <vector>

namespace gmenu::imgui {

namespace {

const char* direction_label(NavDirection direction) {
    switch (direction) {
    case NavDirection::Up:
        return "Up";
    case NavDirection::Down:
        return "Down";
    case NavDirection::Left:
        return "Left";
    case NavDirection::Right:
        return "Right";
    }
    return "Down";
}

bool selectable_widget_combo(const char* label, WidgetId& value, std::span<const DrawItem> items) {
    std::string preview = value == invalid_widget ? "None" : ("#" + std::to_string(value));
    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        if (ImGui::Selectable("None", value == invalid_widget)) {
            value = invalid_widget;
            changed = true;
        }
        for (const DrawItem& item : items) {
            if (item.type == WidgetType::Label) {
                continue;
            }
            std::string name = "#" + std::to_string(item.id) + " " + item.label;
            if (ImGui::Selectable(name.c_str(), value == item.id)) {
                value = item.id;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool direction_combo(NavDirection& direction) {
    bool changed = false;
    if (ImGui::BeginCombo("Direction", direction_label(direction))) {
        for (NavDirection candidate :
             {NavDirection::Up, NavDirection::Down, NavDirection::Left, NavDirection::Right}) {
            if (ImGui::Selectable(direction_label(candidate), direction == candidate)) {
                direction = candidate;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

} // namespace

bool render_nav_editor(Menu& menu, NavEditorState& editor, ScreenId screen,
                       std::span<const DrawItem> items) {
    bool changed = false;

    ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("gmenu: Navigation")) {
        ImGui::End();
        return false;
    }

    if (menu.nav_dirty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f), "Dirty");
    } else {
        ImGui::TextUnformatted("Clean");
    }
    std::vector<NavValidationIssue> issues = menu.validate_nav_overrides(screen, items);
    if (!issues.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "%zu issue(s)", issues.size());
    }

    selectable_widget_combo("Source", editor.source, items);
    direction_combo(editor.direction);
    selectable_widget_combo("Target", editor.target, items);

    const bool can_set = screen != invalid_screen && editor.source != invalid_widget &&
                         editor.target != invalid_widget;
    if (!can_set) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Set link")) {
        menu.set_nav_link(screen, editor.source, editor.direction, editor.target);
        changed = true;
    }
    if (!can_set) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (editor.source == invalid_widget) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Clear direction")) {
        menu.clear_nav_link(screen, editor.source, editor.direction);
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear source")) {
        menu.clear_nav_links(screen, editor.source);
        changed = true;
    }
    if (editor.source == invalid_widget) {
        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Effective Links");
    if (ImGui::BeginTable("effective_nav", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Widget");
        ImGui::TableSetupColumn("Up");
        ImGui::TableSetupColumn("Down");
        ImGui::TableSetupColumn("Left");
        ImGui::TableSetupColumn("Right");
        ImGui::TableSetupColumn("Label");
        ImGui::TableHeadersRow();
        for (const DrawItem& item : items) {
            if (item.type == WidgetType::Label) {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("#%u", item.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("#%u", item.nav_up);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("#%u", item.nav_down);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("#%u", item.nav_left);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("#%u", item.nav_right);
            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(item.label.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Stored Overrides");
    const std::span<const NavOverride> overrides = menu.nav_overrides();
    if (overrides.empty()) {
        ImGui::TextUnformatted("No stored overrides.");
    }
    for (const NavOverride& record : overrides) {
        ImGui::BulletText("screen %u widget %u: U %u D %u L %u R %u", record.scope.screen,
                          record.widget, record.links.up, record.links.down, record.links.left,
                          record.links.right);
    }

    if (!issues.empty() && ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const NavValidationIssue& issue : issues) {
            ImGui::BulletText("screen %u widget %u target %u: %s", issue.screen, issue.widget,
                              issue.target, issue.message.c_str());
        }
    }

    ImGui::End();
    return changed;
}

} // namespace gmenu::imgui
