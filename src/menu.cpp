#include "gmenu/menu.hpp"

#include <algorithm>
#include <cmath>

namespace gmenu {

namespace {

bool point_in_rect(float x, float y, glayout::Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

float clamp_value(float value, float min, float max) {
    if (min > max) {
        std::swap(min, max);
    }
    return std::clamp(value, min, max);
}

} // namespace

int version_major() {
    return 0;
}

Action Action::none() {
    return {};
}

Action Action::push(ScreenId id) {
    Action action;
    action.type = ActionType::Push;
    action.screen = id;
    return action;
}

Action Action::pop() {
    Action action;
    action.type = ActionType::Pop;
    return action;
}

Action Action::replace(ScreenId id) {
    Action action;
    action.type = ActionType::Replace;
    action.screen = id;
    return action;
}

Action Action::set_root(ScreenId id) {
    Action action;
    action.type = ActionType::SetRoot;
    action.screen = id;
    return action;
}

Action Action::command_id(CommandId id, int value) {
    Action action;
    action.type = ActionType::Command;
    action.command = id;
    action.payload = value;
    return action;
}

void Menu::set_layouts(const std::vector<glayout::Layout>* layout_list) {
    layouts = layout_list;
}

void Menu::set_user_data(void* ptr) {
    user = ptr;
}

void Menu::register_screen(ScreenId id, ScreenBuildFn build) {
    if (id == invalid_screen || !build) {
        return;
    }
    for (ScreenDef& def : screens) {
        if (def.id == id) {
            def.build = build;
            return;
        }
    }
    screens.push_back(ScreenDef{id, build});
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
    focused = invalid_widget;
    hovered = invalid_widget;
    pressed = invalid_widget;
    focus_times.clear();
    hover_times.clear();
    press_times.clear();
}

void Menu::update(const Input& input, float dt, int width, int height,
                  glayout::FormFactor form_factor) {
    if (instances.empty()) {
        items.clear();
        return;
    }

    Screen screen = build_current_screen(width, height, form_factor);
    rebuild_draw_items(screen, width, height, form_factor);
    update_focus(screen, input, dt);
    rebuild_draw_items(screen, width, height, form_factor);
}

std::span<const DrawItem> Menu::draw_items() const {
    return items;
}

std::span<const ScreenId> Menu::stack() const {
    return public_stack;
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
    (void)width;
    (void)height;
    (void)form_factor;
    Screen screen;
    if (instances.empty()) {
        return screen;
    }
    const ScreenDef* def = find_screen(instances.back().id);
    if (!def || !def->build) {
        return screen;
    }
    screen.id = def->id;
    BuildContext ctx{*this, def->id, user};
    def->build(ctx, screen);
    return screen;
}

void Menu::rebuild_draw_items(const Screen& screen, int width, int height,
                              glayout::FormFactor form_factor) {
    items.clear();
    const glayout::Layout* layout = nullptr;
    if (layouts) {
        layout = glayout::find_best_layout(*layouts, screen.layout_id, width, height, form_factor);
    }

    for (const Widget& widget : screen.widgets) {
        DrawItem item;
        item.id = widget.id;
        item.type = widget.type;
        item.style = widget.style;
        item.label = widget.label;
        item.secondary = widget.secondary;
        item.value = widget.value;
        item.state.focused = widget.id == focused;
        item.state.hovered = widget.id == hovered;
        item.state.pressed = widget.id == pressed;
        item.state.disabled = widget.disabled;

        auto focus_it = focus_times.find(widget.id);
        auto hover_it = hover_times.find(widget.id);
        auto press_it = press_times.find(widget.id);
        item.state.focus_time = focus_it == focus_times.end() ? 0.0f : focus_it->second;
        item.state.hover_time = hover_it == hover_times.end() ? 0.0f : hover_it->second;
        item.state.press_time = press_it == press_times.end() ? 0.0f : press_it->second;

        if (layout) {
            if (const glayout::Object* object = glayout::find_object(*layout, widget.slot)) {
                item.rect = glayout::map_rect(glayout::Rect{0.0f, 0.0f, static_cast<float>(width),
                                                            static_cast<float>(height)},
                                              object->rect);
            }
        }
        items.push_back(item);
    }
}

void Menu::update_focus(const Screen& screen, const Input& input, float dt) {
    if (!find_widget(screen, focused) || !is_selectable(*find_widget(screen, focused))) {
        focused = screen.default_focus != invalid_widget ? screen.default_focus
                                                         : first_selectable(screen);
    }

    hovered = hovered_widget(screen, input);
    if (hovered != invalid_widget && !input.mouse_down) {
        focused = hovered;
    }

    ginput::RepeatResult up = ginput::update_repeat(input.up, repeat_up, dt);
    ginput::RepeatResult down = ginput::update_repeat(input.down, repeat_down, dt);
    ginput::RepeatResult left = ginput::update_repeat(input.left, repeat_left, dt);
    ginput::RepeatResult right = ginput::update_repeat(input.right, repeat_right, dt);

    if (up.trigger) {
        const Widget* widget = find_widget(screen, focused);
        if (widget) {
            focused = resolve_nav(screen, focused, widget->nav_up, -1);
        }
    } else if (down.trigger) {
        const Widget* widget = find_widget(screen, focused);
        if (widget) {
            focused = resolve_nav(screen, focused, widget->nav_down, 1);
        }
    } else if (left.trigger) {
        const Widget* widget = find_widget(screen, focused);
        if (widget && widget->on_left.type != ActionType::None) {
            execute(widget->on_left);
        } else if (widget) {
            focused = resolve_nav(screen, focused, widget->nav_left, -1);
        }
    } else if (right.trigger) {
        const Widget* widget = find_widget(screen, focused);
        if (widget && widget->on_right.type != ActionType::None) {
            execute(widget->on_right);
        } else if (widget) {
            focused = resolve_nav(screen, focused, widget->nav_right, 1);
        }
    }

    const bool mouse_clicked = input.mouse_valid && input.mouse_down && !prev_mouse_down;
    prev_mouse_down = input.mouse_down;
    if (mouse_clicked && hovered != invalid_widget) {
        focused = hovered;
        pressed = hovered;
    } else if (!input.mouse_down) {
        pressed = invalid_widget;
    }

    const Widget* active = find_widget(screen, focused);
    if (input.select && active && !active->disabled) {
        if (active->type == WidgetType::Toggle && active->bool_value) {
            *active->bool_value = !*active->bool_value;
        } else if (active->type == WidgetType::Slider1D && active->float_value) {
            *active->float_value =
                clamp_value(*active->float_value + active->step, active->min, active->max);
        }
        execute(active->on_select);
    } else if (input.back && active && active->on_back.type != ActionType::None) {
        execute(active->on_back);
    } else if (input.back && instances.size() > 1) {
        pop();
    }

    for (const Widget& widget : screen.widgets) {
        if (widget.id == focused) {
            focus_times[widget.id] += dt;
        } else {
            focus_times.erase(widget.id);
        }
        if (widget.id == hovered) {
            hover_times[widget.id] += dt;
        } else {
            hover_times.erase(widget.id);
        }
        if (widget.id == pressed) {
            press_times[widget.id] += dt;
        } else {
            press_times.erase(widget.id);
        }
    }
}

void Menu::execute(const Action& action) {
    switch (action.type) {
    case ActionType::None:
        break;
    case ActionType::Push:
        push(action.screen);
        break;
    case ActionType::Pop:
        pop();
        break;
    case ActionType::Replace:
        replace(action.screen);
        break;
    case ActionType::SetRoot:
        set_root(action.screen);
        break;
    case ActionType::Command:
        invoke_command(action.command, action.payload);
        break;
    }
}

void Menu::invoke_command(CommandId id, int payload) {
    if (id == invalid_command) {
        return;
    }
    std::size_t index = static_cast<std::size_t>(id - 1);
    if (index >= commands.size() || !commands[index]) {
        return;
    }
    BuildContext ctx{*this, current_screen(), user};
    commands[index](ctx, payload);
}

WidgetId Menu::first_selectable(const Screen& screen) const {
    for (const Widget& widget : screen.widgets) {
        if (is_selectable(widget)) {
            return widget.id;
        }
    }
    return invalid_widget;
}

WidgetId Menu::resolve_nav(const Screen& screen, WidgetId from, WidgetId explicit_target,
                           int direction) const {
    if (explicit_target != invalid_widget) {
        const Widget* target = find_widget(screen, explicit_target);
        if (target && is_selectable(*target)) {
            return explicit_target;
        }
    }

    std::vector<WidgetId> selectable;
    for (const Widget& widget : screen.widgets) {
        if (is_selectable(widget)) {
            selectable.push_back(widget.id);
        }
    }
    if (selectable.empty()) {
        return invalid_widget;
    }

    auto it = std::find(selectable.begin(), selectable.end(), from);
    if (it == selectable.end()) {
        return selectable.front();
    }
    int index = static_cast<int>(std::distance(selectable.begin(), it));
    int next = index + direction;
    if (next < 0) {
        next = static_cast<int>(selectable.size()) - 1;
    }
    if (next >= static_cast<int>(selectable.size())) {
        next = 0;
    }
    return selectable[static_cast<std::size_t>(next)];
}

const Widget* Menu::find_widget(const Screen& screen, WidgetId id) const {
    for (const Widget& widget : screen.widgets) {
        if (widget.id == id) {
            return &widget;
        }
    }
    return nullptr;
}

Widget* Menu::find_widget(Screen& screen, WidgetId id) const {
    for (Widget& widget : screen.widgets) {
        if (widget.id == id) {
            return &widget;
        }
    }
    return nullptr;
}

WidgetId Menu::hovered_widget(const Screen& screen, const Input& input) const {
    if (!input.mouse_valid) {
        return invalid_widget;
    }
    for (const DrawItem& item : items) {
        if (item.id == invalid_widget || !point_in_rect(input.mouse_x, input.mouse_y, item.rect)) {
            continue;
        }
        const Widget* widget = find_widget(screen, item.id);
        if (widget && is_selectable(*widget)) {
            return widget->id;
        }
    }
    return invalid_widget;
}

bool Menu::is_selectable(const Widget& widget) const {
    if (widget.disabled) {
        return false;
    }
    return widget.type != WidgetType::Label;
}

} // namespace gmenu
