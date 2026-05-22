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

bool screen_combo(const char* label, ScreenId& value, std::span<const ScreenDef> screens) {
    std::string preview = value == invalid_screen ? "None" : ("#" + std::to_string(value));
    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        if (ImGui::Selectable("None", value == invalid_screen)) {
            value = invalid_screen;
            changed = true;
        }
        for (const ScreenDef& screen : screens) {
            std::string name = "#" + std::to_string(screen.id);
            if (ImGui::Selectable(name.c_str(), value == screen.id)) {
                value = screen.id;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

const DrawItem* find_item(std::span<const DrawItem> items, WidgetId id) {
    for (const DrawItem& item : items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

const char* widget_type_label(WidgetType type) {
    switch (type) {
    case WidgetType::Label:
        return "label";
    case WidgetType::Button:
        return "button";
    case WidgetType::Card:
        return "card";
    case WidgetType::Toggle:
        return "toggle";
    case WidgetType::Slider1D:
        return "slider";
    case WidgetType::OptionCycle:
        return "option";
    case WidgetType::TextInput:
        return "text";
    }
    return "unknown";
}

void render_link_target(const char* label, WidgetId target, NavSource source) {
    ImGui::Text("%s: #%u", label, target);
    ImGui::SameLine();
    const char* source_text = "none";
    if (source == NavSource::Explicit) {
        source_text = "explicit";
    } else if (source == NavSource::Override) {
        source_text = "override";
    }
    ImGui::TextDisabled("(%s)", source_text);
}

void render_selected_widget(NavEditorState& editor, std::span<const DrawItem> items) {
    const DrawItem* item = find_item(items, editor.source);
    if (!item) {
        ImGui::TextUnformatted("No source widget selected.");
        return;
    }

    ImGui::Text("Widget #%u", item->id);
    ImGui::Text("Type: %s", widget_type_label(item->type));
    ImGui::Text("Label: %s", item->label.c_str());
    ImGui::Text("Rect: %.0f %.0f %.0f %.0f", item->rect.x, item->rect.y, item->rect.w,
                item->rect.h);
    if (!item->value.empty()) {
        ImGui::Text("Value: %s", item->value.c_str());
    }
    render_link_target("Up", item->nav_up, item->nav_up_source);
    render_link_target("Down", item->nav_down, item->nav_down_source);
    render_link_target("Left", item->nav_left, item->nav_left_source);
    render_link_target("Right", item->nav_right, item->nav_right_source);
}

} // namespace

bool render_nav_editor(Menu& menu, NavEditorState& editor, ScreenId screen,
                       std::span<const DrawItem> items) {
    ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("gmenu: Navigation")) {
        ImGui::End();
        return false;
    }

    const bool changed = render_nav_editor_panel(menu, editor, screen, items);

    ImGui::End();
    return changed;
}

bool render_nav_editor_panel(Menu& menu, NavEditorState& editor, ScreenId screen,
                             std::span<const DrawItem> items) {
    bool changed = false;

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

    if (editor.selected_screen == invalid_screen) {
        editor.selected_screen = screen;
    }
    ImGui::SeparatorText("Screen");
    screen_combo("Registered screen", editor.selected_screen, menu.registered_screens());
    const bool can_open_screen =
        editor.selected_screen != invalid_screen && editor.selected_screen != menu.current_screen();
    if (!can_open_screen) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Set root")) {
        if (menu.set_root(editor.selected_screen)) {
            editor.source = invalid_widget;
            editor.target = invalid_widget;
            screen = editor.selected_screen;
            changed = true;
        }
    }
    if (!can_open_screen) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    ImGui::Text("Current: #%u", menu.current_screen());

    selectable_widget_combo("Source", editor.source, items);
    direction_combo(editor.direction);
    selectable_widget_combo("Target", editor.target, items);

    ImGui::SeparatorText("Selected Widget");
    render_selected_widget(editor, items);

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
            ImGui::PushID(static_cast<int>(item.id));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            std::string widget_label = "#" + std::to_string(item.id);
            if (ImGui::Selectable(widget_label.c_str(), editor.source == item.id,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                editor.source = item.id;
            }
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
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Stored Overrides");
    if (ImGui::Button("Save overrides")) {
        editor.save_requested = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load overrides")) {
        editor.load_requested = true;
    }
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
        if (ImGui::BeginTable("nav_validation", 5,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Screen");
            ImGui::TableSetupColumn("Widget");
            ImGui::TableSetupColumn("Direction");
            ImGui::TableSetupColumn("Target");
            ImGui::TableSetupColumn("Message");
            ImGui::TableHeadersRow();
            for (const NavValidationIssue& issue : issues) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", issue.screen);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", issue.widget);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(direction_label(issue.direction));
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%u", issue.target);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(issue.message.c_str());
            }
            ImGui::EndTable();
        }
    } else if (issues.empty()) {
        ImGui::SeparatorText("Validation");
        ImGui::TextUnformatted("No nav validation issues for the current screen.");
    }

    return changed;
}

} // namespace gmenu::imgui
