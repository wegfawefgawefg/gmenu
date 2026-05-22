#include "gmenu/input_screens.hpp"

#include "gmenu/widgets.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace gmenu {

namespace {

void add_title(const BindActionListScreenDef& def, Screen& out) {
    if (def.title.empty() || def.title_id == invalid_widget) {
        return;
    }
    Widget title = label(def.title_id, def.title_slot, def.title);
    title.style = def.title_style;
    out.widgets.push_back(std::move(title));
}

std::string bind_count_text(const ginput::InputProfile& profile, ginput::ActionId action) {
    const std::size_t count = ginput::button_binds_for_action(profile, action).size();
    if (count == 1) {
        return "1 button bind";
    }
    return std::to_string(count) + " button binds";
}

} // namespace

void build_bind_action_list_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const BindActionListScreenDef*>(ctx.screen_data);
    if (!def || !def->schema || !def->profile) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(*def, out);

    const std::vector<ginput::SchemaEntry>& actions = def->schema->actions();
    const int per_page =
        def->items_per_page > 0 ? def->items_per_page : static_cast<int>(def->item_slots.size());
    const int total_items = static_cast<int>(actions.size());
    const int total_pages = per_page > 0 ? std::max(1, (total_items + per_page - 1) / per_page) : 1;
    int page = def->page ? *def->page : 0;
    page = std::clamp(page, 0, total_pages - 1);
    if (def->page) {
        *def->page = page;
    }

    const int first = page * per_page;
    const int visible_count = std::min(per_page, static_cast<int>(def->item_slots.size()));
    for (int i = 0; i < visible_count; ++i) {
        const int action_index = first + i;
        if (action_index >= total_items) {
            break;
        }
        const ginput::SchemaEntry& action = actions[static_cast<std::size_t>(action_index)];
        Widget widget = card(static_cast<WidgetId>(1000 + action.id),
                             def->item_slots[static_cast<std::size_t>(i)], action.label,
                             bind_count_text(*def->profile, action.id),
                             Action::command_id(def->edit_command, action.id));
        widget.style = def->item_style;
        out.widgets.push_back(std::move(widget));
    }

    if (def->page_label_id != invalid_widget) {
        std::string text = "Page " + std::to_string(page + 1) + " / " + std::to_string(total_pages);
        Widget page_label = label(def->page_label_id, def->page_label_slot, text);
        page_label.style = def->nav_style;
        out.widgets.push_back(std::move(page_label));
    }

    if (def->page && def->prev_id != invalid_widget) {
        Widget prev = button(def->prev_id, def->prev_slot, "<",
                             Action::delta_int(*def->page, -1, 0, total_pages - 1));
        prev.style = def->nav_style;
        prev.disabled = page <= 0;
        out.widgets.push_back(std::move(prev));
    }

    if (def->page && def->next_id != invalid_widget) {
        Widget next = button(def->next_id, def->next_slot, ">",
                             Action::delta_int(*def->page, 1, 0, total_pages - 1));
        next.style = def->nav_style;
        next.disabled = page + 1 >= total_pages;
        out.widgets.push_back(std::move(next));
    }

    if (def->back_id != invalid_widget) {
        Widget back = button(def->back_id, def->back_slot, def->back_label, Action::pop());
        back.style = def->nav_style;
        out.widgets.push_back(std::move(back));
    }
}

void register_bind_action_list_screen(Menu& menu, const BindActionListScreenDef& def) {
    menu.register_screen(def.id, build_bind_action_list_screen, &def);
}

} // namespace gmenu
