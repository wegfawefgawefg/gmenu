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
    add_title(ListScreenDef{def->id,
                            def->layout_id,
                            def->title_id,
                            def->title_slot,
                            def->title,
                            def->title_style,
                            def->default_focus,
                            {}},
              out);

    const int per_page =
        def->items_per_page > 0 ? def->items_per_page : static_cast<int>(def->item_slots.size());
    const int total_items = static_cast<int>(def->items.size());
    const int total_pages = per_page > 0 ? std::max(1, (total_items + per_page - 1) / per_page) : 1;
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

void register_basic_screen(Menu& menu, const BasicScreenDef& def) {
    menu.register_screen(def.id, build_basic_screen, &def);
}

void register_list_screen(Menu& menu, const ListScreenDef& def) {
    menu.register_screen(def.id, build_list_screen, &def);
}

void register_paged_list_screen(Menu& menu, const PagedListScreenDef& def) {
    menu.register_screen(def.id, build_paged_list_screen, &def);
}

} // namespace gmenu
