#include "gmenu/menu.hpp"

namespace gmenu {

void Menu::set_layouts(const std::vector<glayout::Layout>* layout_list) {
    layouts = layout_list;
    layout_store = nullptr;
}

void Menu::set_layout_store(const glayout::LayoutStore* store) {
    layout_store = store;
    layouts = nullptr;
}

void Menu::set_user_data(void* ptr) {
    user = ptr;
}

void Menu::set_feedback_hooks(const FeedbackHooks* hooks) {
    feedback_hooks = hooks;
}

void Menu::register_screen(ScreenId id, ScreenBuildFn build, const void* data) {
    if (id == invalid_screen || !build) {
        return;
    }
    for (ScreenDef& def : screens) {
        if (def.id == id) {
            def.build = build;
            def.data = data;
            return;
        }
    }
    screens.push_back(ScreenDef{id, build, data});
}

CommandId Menu::register_command(CommandFn fn) {
    if (!fn) {
        return invalid_command;
    }
    commands.push_back(fn);
    return static_cast<CommandId>(commands.size());
}

bool Menu::set_root(ScreenId id) {
    if (!find_screen(id)) {
        return false;
    }
    clear();
    instances.push_back(ScreenInstance{id});
    public_stack.push_back(id);
    focused = invalid_widget;
    return true;
}

bool Menu::push(ScreenId id) {
    if (!find_screen(id)) {
        return false;
    }
    instances.push_back(ScreenInstance{id});
    public_stack.push_back(id);
    focused = invalid_widget;
    return true;
}

bool Menu::replace(ScreenId id) {
    if (!find_screen(id)) {
        return false;
    }
    if (instances.empty()) {
        return set_root(id);
    }
    instances.back().id = id;
    public_stack.back() = id;
    focused = invalid_widget;
    return true;
}

void Menu::pop() {
    if (instances.empty()) {
        return;
    }
    instances.pop_back();
    public_stack.pop_back();
    focused = invalid_widget;
}

void Menu::clear() {
    instances.clear();
    public_stack.clear();
    items.clear();
    nav_returns.clear();
    focused = invalid_widget;
    hovered = invalid_widget;
    pressed = invalid_widget;
    editing = invalid_widget;
    slider_dragging = invalid_widget;
    editing_start_value.clear();
    allow_mouse_focus = true;
    mouse_focus_locked = false;
    focus_times.clear();
    hover_times.clear();
    press_times.clear();
}

void Menu::update(const Input& input, float dt, int width, int height,
                  glayout::FormFactor form_factor) {
    feedback_events.clear();

    if (instances.empty()) {
        items.clear();
        flush_feedback();
        return;
    }

    Screen screen = build_current_screen(width, height, form_factor);
    rebuild_draw_items(screen, width, height, form_factor);
    update_focus(screen, input, dt);
    if (instances.empty()) {
        items.clear();
        flush_feedback();
        return;
    }
    screen = build_current_screen(width, height, form_factor);
    rebuild_draw_items(screen, width, height, form_factor);
    flush_feedback();
}

std::span<const DrawItem> Menu::draw_items() const {
    return items;
}

std::span<const ScreenId> Menu::stack() const {
    return public_stack;
}

std::span<const ScreenDef> Menu::registered_screens() const {
    return screens;
}

WidgetId Menu::focus() const {
    return focused;
}

ScreenId Menu::current_screen() const {
    if (instances.empty()) {
        return invalid_screen;
    }
    return instances.back().id;
}

void* Menu::user_data() const {
    return user;
}

const ScreenDef* Menu::find_screen(ScreenId id) const {
    for (const ScreenDef& def : screens) {
        if (def.id == id) {
            return &def;
        }
    }
    return nullptr;
}

Screen Menu::build_current_screen(int width, int height, glayout::FormFactor form_factor) {
    Screen screen;
    if (instances.empty()) {
        return screen;
    }
    const ScreenDef* def = find_screen(instances.back().id);
    if (!def || !def->build) {
        return screen;
    }
    screen.id = def->id;
    BuildContext ctx{*this, def->id, user, def->data};
    def->build(ctx, screen);
    apply_nav_overrides(screen, width, height, form_factor);
    return screen;
}

} // namespace gmenu
