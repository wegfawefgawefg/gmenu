#include "gmenu/menu.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gmenu {

namespace {

float clamp_value(float value, float min, float max) {
    if (min > max) {
        std::swap(min, max);
    }
    return std::clamp(value, min, max);
}

} // namespace

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
    focused = invalid_widget;
    hovered = invalid_widget;
    pressed = invalid_widget;
    editing = invalid_widget;
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
    if (instances.empty()) {
        items.clear();
        return;
    }
    screen = build_current_screen(width, height, form_factor);
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
    BuildContext ctx{*this, def->id, user, def->data};
    def->build(ctx, screen);
    apply_nav_overrides(screen, width, height, form_factor);
    return screen;
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

    const bool was_text_editing = editing != invalid_widget;
    update_text_input(screen, input);
    const bool text_editing = editing != invalid_widget;

    if (!text_editing) {
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
            } else if (widget && (widget->type == WidgetType::Slider1D ||
                                  widget->type == WidgetType::OptionCycle)) {
                adjust_widget(*widget, -1);
            } else if (widget && widget->nav_left != invalid_widget) {
                focused = resolve_nav(screen, focused, widget->nav_left, -1);
            }
        } else if (right.trigger) {
            const Widget* widget = find_widget(screen, focused);
            if (widget && widget->on_right.type != ActionType::None) {
                execute(widget->on_right);
            } else if (widget && (widget->type == WidgetType::Slider1D ||
                                  widget->type == WidgetType::OptionCycle)) {
                adjust_widget(*widget, 1);
            } else if (widget && widget->nav_right != invalid_widget) {
                focused = resolve_nav(screen, focused, widget->nav_right, 1);
            }
        }
    }

    const bool mouse_clicked = input.mouse_valid && input.mouse_down && !prev_mouse_down;
    const bool mouse_released = input.mouse_valid && !input.mouse_down && prev_mouse_down;
    prev_mouse_down = input.mouse_down;
    if (mouse_clicked && hovered != invalid_widget) {
        focused = hovered;
        pressed = hovered;
    } else if (mouse_released) {
        const Widget* clicked = find_widget(screen, pressed);
        if (clicked && hovered == pressed) {
            if (!text_editing) {
                if (clicked->type == WidgetType::Slider1D) {
                    set_slider_from_mouse(*clicked, input);
                } else {
                    activate_widget(*clicked);
                }
            } else if (clicked->id != editing) {
                editing = invalid_widget;
                activate_widget(*clicked);
            }
        }
        pressed = invalid_widget;
    } else if (!input.mouse_down) {
        pressed = invalid_widget;
    }

    const Widget* active = find_widget(screen, focused);
    if (!text_editing && !was_text_editing && input.select && active && !active->disabled) {
        activate_widget(*active);
    } else if (!text_editing && input.page_prev && active) {
        adjust_widget(*active, -1);
    } else if (!text_editing && input.page_next && active) {
        adjust_widget(*active, 1);
    } else if (!text_editing && input.back && active && active->on_back.type != ActionType::None) {
        execute(active->on_back);
    } else if (!text_editing && input.back && instances.size() > 1) {
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

void Menu::update_text_input(const Screen& screen, const Input& input) {
    const Widget* active = find_widget(screen, editing);
    if (!active || active->type != WidgetType::TextInput || active->text_value == nullptr) {
        editing = invalid_widget;
        return;
    }

    if (input.back) {
        editing = invalid_widget;
        return;
    }

    if (input.select) {
        editing = invalid_widget;
        return;
    }

    if (input.backspace && !active->text_value->empty()) {
        if (input.delete_word) {
            while (!active->text_value->empty() && active->text_value->back() == ' ') {
                active->text_value->pop_back();
            }
            while (!active->text_value->empty() && active->text_value->back() != ' ') {
                active->text_value->pop_back();
            }
        } else {
            active->text_value->pop_back();
        }
    }

    if (!input.text.empty()) {
        std::size_t max_len = std::numeric_limits<std::size_t>::max();
        if (active->text_max_len > 0) {
            max_len = static_cast<std::size_t>(active->text_max_len);
        }
        for (char ch : input.text) {
            if (active->text_value->size() >= max_len) {
                break;
            }
            if (ch >= 32 && ch != 127) {
                active->text_value->push_back(ch);
            }
        }
    }
}

void Menu::activate_widget(const Widget& widget) {
    if (widget.disabled) {
        return;
    }
    if (widget.type == WidgetType::Toggle && widget.bool_value) {
        *widget.bool_value = !*widget.bool_value;
    } else if (widget.type == WidgetType::Slider1D && widget.float_value) {
        *widget.float_value =
            clamp_value(*widget.float_value + widget.step, widget.min, widget.max);
    } else if (widget.type == WidgetType::OptionCycle) {
        adjust_widget(widget, 1);
    } else if (widget.type == WidgetType::TextInput && widget.text_value &&
               widget.select_enters_text) {
        editing = widget.id;
    }
    execute(widget.on_select);
}

void Menu::adjust_widget(const Widget& widget, int direction) {
    if (widget.disabled) {
        return;
    }
    if (widget.type == WidgetType::Slider1D && widget.float_value) {
        *widget.float_value =
            clamp_value(*widget.float_value + (widget.step * static_cast<float>(direction)),
                        widget.min, widget.max);
    } else if (widget.type == WidgetType::OptionCycle && widget.option_index &&
               widget.option_count > 0) {
        int next = *widget.option_index + direction;
        if (next < 0) {
            next = widget.option_count - 1;
        }
        if (next >= widget.option_count) {
            next = 0;
        }
        *widget.option_index = next;
    }
}

void Menu::set_slider_from_mouse(const Widget& widget, const Input& input) {
    if (widget.disabled || widget.type != WidgetType::Slider1D || !widget.float_value ||
        !input.mouse_valid) {
        return;
    }

    glayout::Rect rect;
    bool found = false;
    for (const DrawItem& item : items) {
        if (item.id == widget.id) {
            rect = item.rect;
            found = true;
            break;
        }
    }
    if (!found || rect.w <= 0.0f) {
        return;
    }

    float ratio = (input.mouse_x - rect.x) / rect.w;
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    *widget.float_value = widget.min + ((widget.max - widget.min) * ratio);
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
    case ActionType::SetInt:
        if (action.int_value) {
            *action.int_value = action.payload;
        }
        break;
    case ActionType::DeltaInt:
        if (action.int_value) {
            int min_value = action.min;
            int max_value = action.max;
            if (min_value > max_value) {
                std::swap(min_value, max_value);
            }
            *action.int_value =
                std::clamp(*action.int_value + action.payload, min_value, max_value);
        }
        break;
    case ActionType::RemoveButtonBind:
        if (action.input_profile) {
            ginput::remove_button_bind(
                *action.input_profile,
                ginput::ButtonBind{action.encoded_control, action.input_action});
        }
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
    const ScreenDef* def = find_screen(current_screen());
    BuildContext ctx{*this, current_screen(), user, def ? def->data : nullptr};
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
    (void)from;
    (void)direction;
    if (explicit_target == invalid_widget) {
        return invalid_widget;
    }
    const Widget* target = find_widget(screen, explicit_target);
    if (!target || !is_selectable(*target)) {
        return invalid_widget;
    }
    return explicit_target;
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

bool Menu::is_selectable(const Widget& widget) const {
    if (widget.disabled) {
        return false;
    }
    return widget.type != WidgetType::Label;
}

} // namespace gmenu
