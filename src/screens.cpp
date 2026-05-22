#include "gmenu/screens.hpp"

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

void register_basic_screen(Menu& menu, const BasicScreenDef& def) {
    menu.register_screen(def.id, build_basic_screen, &def);
}

void register_list_screen(Menu& menu, const ListScreenDef& def) {
    menu.register_screen(def.id, build_list_screen, &def);
}

} // namespace gmenu
