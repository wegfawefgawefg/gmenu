#include "gmenu/screens.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace gmenu {

namespace {

void add_title(const BasicScreenDef& def, Screen& out) {
    if (def.title.empty() || def.title_id == invalid_widget) {
        return;
    }
    Widget title = label(def.title_id, def.title_slot, def.title);
    title.style = def.title_style;
    out.widgets.push_back(std::move(title));
}

void add_title(const ListScreenDef& def, Screen& out) {
    if (def.title.empty() || def.title_id == invalid_widget) {
        return;
    }
    Widget title = label(def.title_id, def.title_slot, def.title);
    title.style = def.title_style;
    out.widgets.push_back(std::move(title));
}

void add_title(ScreenId id, int layout_id, WidgetId title_id, const std::string& title_slot,
               const std::string& title_text, StyleId title_style, WidgetId default_focus,
               Screen& out) {
    if (title_text.empty() || title_id == invalid_widget) {
        return;
    }
    add_title(
        ListScreenDef{
            id, layout_id, title_id, title_slot, title_text, title_style, default_focus, {}},
        out);
}

int page_count(int item_count, int per_page) {
    if (per_page <= 0) {
        return 1;
    }
    return std::max(1, (item_count + per_page - 1) / per_page);
}

} // namespace

void build_basic_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const BasicScreenDef*>(ctx.screen_data);
    if (!def) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(*def, out);
    for (const Widget& widget : def->widgets) {
        out.widgets.push_back(widget);
    }
}

void build_list_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const ListScreenDef*>(ctx.screen_data);
    if (!def) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(*def, out);
    for (const ListItem& item : def->items) {
        Widget widget = item.secondary.empty()
                            ? button(item.id, item.slot, item.label, item.action)
                            : card(item.id, item.slot, item.label, item.secondary, item.action);
        widget.style = item.style;
        widget.disabled = item.disabled;
        out.widgets.push_back(std::move(widget));
    }
}

void build_paged_list_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const PagedListScreenDef*>(ctx.screen_data);
    if (!def) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(def->id, def->layout_id, def->title_id, def->title_slot, def->title, def->title_style,
              def->default_focus, out);

    const int per_page =
        def->items_per_page > 0 ? def->items_per_page : static_cast<int>(def->item_slots.size());
    const int total_items = static_cast<int>(def->items.size());
    const int total_pages = page_count(total_items, per_page);
    int page = def->page ? *def->page : 0;
    page = std::clamp(page, 0, total_pages - 1);
    if (def->page) {
        *def->page = page;
    }

    const int first = page * per_page;
    const int visible_count = std::min(per_page, static_cast<int>(def->item_slots.size()));
    for (int i = 0; i < visible_count; ++i) {
        const int item_index = first + i;
        if (item_index >= total_items) {
            break;
        }
        ListItem item = def->items[static_cast<std::size_t>(item_index)];
        item.slot = def->item_slots[static_cast<std::size_t>(i)];
        if (item.style == default_style) {
            item.style = def->item_style;
        }
        Widget widget = item.secondary.empty()
                            ? button(item.id, item.slot, item.label, item.action)
                            : card(item.id, item.slot, item.label, item.secondary, item.action);
        widget.style = item.style;
        widget.disabled = item.disabled;
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
        out.widgets.push_back(prev);
    }

    if (def->page && def->next_id != invalid_widget) {
        Widget next = button(def->next_id, def->next_slot, ">",
                             Action::delta_int(*def->page, 1, 0, total_pages - 1));
        next.style = def->nav_style;
        next.disabled = page + 1 >= total_pages;
        out.widgets.push_back(next);
    }

    if (def->back_id != invalid_widget) {
        Widget back = button(def->back_id, def->back_slot, def->back_label, Action::pop());
        back.style = def->nav_style;
        out.widgets.push_back(std::move(back));
    }
}

void build_settings_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const SettingsScreenDef*>(ctx.screen_data);
    if (!def) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(def->id, def->layout_id, def->title_id, def->title_slot, def->title, def->title_style,
              def->default_focus, out);

    for (const SettingItem& item : def->items) {
        Widget widget;
        switch (item.type) {
        case SettingType::Toggle:
            if (!item.bool_value) {
                continue;
            }
            widget = toggle(item.id, item.slot, item.label, *item.bool_value);
            break;
        case SettingType::Slider1D:
            if (!item.float_value) {
                continue;
            }
            widget = slider_1d(item.id, item.slot, item.label, *item.float_value, item.min,
                               item.max, item.step);
            break;
        case SettingType::OptionCycle:
            if (!item.option_index) {
                continue;
            }
            widget = option_cycle(item.id, item.slot, item.label, *item.option_index, item.options);
            break;
        case SettingType::TextInput:
            if (!item.text_value) {
                continue;
            }
            widget =
                text_input(item.id, item.slot, item.label, *item.text_value, item.text_max_len);
            break;
        }
        widget.style = item.style == default_style ? def->item_style : item.style;
        widget.disabled = item.disabled;
        out.widgets.push_back(std::move(widget));
    }

    if (def->back_id != invalid_widget) {
        Widget back = button(def->back_id, def->back_slot, def->back_label, Action::pop());
        back.style = def->nav_style;
        out.widgets.push_back(std::move(back));
    }
}

void build_profile_list_screen(BuildContext& ctx, Screen& out) {
    auto* def = static_cast<const ProfileListScreenDef*>(ctx.screen_data);
    if (!def || !def->profiles) {
        return;
    }

    out.id = def->id;
    out.layout_id = def->layout_id;
    out.default_focus = def->default_focus;
    add_title(def->id, def->layout_id, def->title_id, def->title_slot, def->title, def->title_style,
              def->default_focus, out);

    const int per_page =
        def->items_per_page > 0 ? def->items_per_page : static_cast<int>(def->item_slots.size());
    const int total_items = static_cast<int>(def->profiles->size());
    const int total_pages = page_count(total_items, per_page);
    int page = def->page ? *def->page : 0;
    page = std::clamp(page, 0, total_pages - 1);
    if (def->page) {
        *def->page = page;
    }

    const int first = page * per_page;
    const int visible_count = std::min(per_page, static_cast<int>(def->item_slots.size()));
    for (int i = 0; i < visible_count; ++i) {
        const int profile_index = first + i;
        if (profile_index >= total_items) {
            break;
        }
        const ProfileEntry& profile = (*def->profiles)[static_cast<std::size_t>(profile_index)];
        Action action = def->select_command != invalid_command
                            ? Action::command_id(def->select_command, profile.id)
                            : Action::none();
        if (def->selected_profile_id && def->select_command == invalid_command) {
            action = Action::set_int(*def->selected_profile_id, profile.id);
        }
        Widget widget = card(static_cast<WidgetId>(3000 + profile.id),
                             def->item_slots[static_cast<std::size_t>(i)], profile.name,
                             profile.secondary, action);
        widget.style = def->item_style;
        widget.disabled = profile.disabled;
        if (def->selected_profile_id && *def->selected_profile_id == profile.id) {
            widget.value = "selected";
        }
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

void register_basic_screen(Menu& menu, const BasicScreenDef& def) {
    menu.register_screen(def.id, build_basic_screen, &def);
}

void register_list_screen(Menu& menu, const ListScreenDef& def) {
    menu.register_screen(def.id, build_list_screen, &def);
}

void register_paged_list_screen(Menu& menu, const PagedListScreenDef& def) {
    menu.register_screen(def.id, build_paged_list_screen, &def);
}

void register_settings_screen(Menu& menu, const SettingsScreenDef& def) {
    menu.register_screen(def.id, build_settings_screen, &def);
}

void register_profile_list_screen(Menu& menu, const ProfileListScreenDef& def) {
    menu.register_screen(def.id, build_profile_list_screen, &def);
}

} // namespace gmenu
