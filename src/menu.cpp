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

bool contains(glayout::Rect rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
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
    const WidgetId frame_start_focus = focused;

    if (!find_widget(screen, focused) || !is_selectable(*find_widget(screen, focused))) {
        focused = screen.default_focus != invalid_widget ? screen.default_focus
                                                         : first_selectable(screen);
    }

    ensure_mouse_focus_lock(input);
    unlock_mouse_focus_if_moved(input);

    hovered = hovered_widget(screen, input);
    if (allow_mouse_focus && hovered != invalid_widget && !input.mouse_down) {
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
                lock_mouse_focus(input);
                WidgetId target = resolve_return_nav(screen, focused, NavDirection::Up);
                if (target == invalid_widget) {
                    target = resolve_nav(screen, focused, widget->nav_up, -1);
                }
                if (target != invalid_widget) {
                    move_focus_with_return(screen, target, NavDirection::Down);
                } else {
                    record_widget_feedback(FeedbackType::Rejected, focused);
                }
            }
        } else if (down.trigger) {
            const Widget* widget = find_widget(screen, focused);
            if (widget) {
                lock_mouse_focus(input);
                WidgetId target = resolve_return_nav(screen, focused, NavDirection::Down);
                if (target == invalid_widget) {
                    target = resolve_nav(screen, focused, widget->nav_down, 1);
                }
                if (target != invalid_widget) {
                    move_focus_with_return(screen, target, NavDirection::Up);
                } else {
                    record_widget_feedback(FeedbackType::Rejected, focused);
                }
            }
        } else if (left.trigger) {
            const Widget* widget = find_widget(screen, focused);
            if (widget) {
                lock_mouse_focus(input);
            }
            if (widget && widget->on_left.type != ActionType::None) {
                record_widget_feedback(FeedbackType::AdjustedLeft, widget->id);
                execute(widget->on_left);
            } else if (widget && (widget->type == WidgetType::Slider1D ||
                                  widget->type == WidgetType::OptionCycle)) {
                adjust_widget(*widget, -1);
            } else if (widget) {
                WidgetId target = resolve_return_nav(screen, focused, NavDirection::Left);
                if (target == invalid_widget && widget->nav_left != invalid_widget) {
                    target = resolve_nav(screen, focused, widget->nav_left, -1);
                }
                if (target != invalid_widget) {
                    move_focus_with_return(screen, target, NavDirection::Right);
                } else {
                    record_widget_feedback(FeedbackType::Rejected, focused);
                }
            } else {
                record_widget_feedback(FeedbackType::Rejected, focused);
            }
        } else if (right.trigger) {
            const Widget* widget = find_widget(screen, focused);
            if (widget) {
                lock_mouse_focus(input);
            }
            if (widget && widget->on_right.type != ActionType::None) {
                record_widget_feedback(FeedbackType::AdjustedRight, widget->id);
                execute(widget->on_right);
            } else if (widget && (widget->type == WidgetType::Slider1D ||
                                  widget->type == WidgetType::OptionCycle)) {
                adjust_widget(*widget, 1);
            } else if (widget) {
                WidgetId target = resolve_return_nav(screen, focused, NavDirection::Right);
                if (target == invalid_widget && widget->nav_right != invalid_widget) {
                    target = resolve_nav(screen, focused, widget->nav_right, 1);
                }
                if (target != invalid_widget) {
                    move_focus_with_return(screen, target, NavDirection::Left);
                } else {
                    record_widget_feedback(FeedbackType::Rejected, focused);
                }
            } else {
                record_widget_feedback(FeedbackType::Rejected, focused);
            }
        }
    }

    const bool mouse_clicked = input.mouse_valid && input.mouse_down && !prev_mouse_down;
    const bool mouse_released = input.mouse_valid && !input.mouse_down && prev_mouse_down;
    prev_mouse_down = input.mouse_down;
    if (mouse_clicked) {
        unlock_mouse_focus_now();
    }
    if (mouse_clicked && hovered != invalid_widget) {
        focused = hovered;
        pressed = hovered;
        const Widget* clicked = find_widget(screen, hovered);
        const DrawItem* item = find_draw_item(hovered);
        if (clicked && item) {
            const ControlPart part = hit_control_part(*item, input.mouse_x, input.mouse_y);
            if (clicked->type == WidgetType::Slider1D && part == ControlPart::SliderTrack) {
                slider_dragging = clicked->id;
                set_slider_from_mouse(*clicked, input.mouse_x, false);
            }
        }
    } else if (mouse_released) {
        if (slider_dragging != invalid_widget) {
            const Widget* dragging = find_widget(screen, slider_dragging);
            if (dragging) {
                set_slider_from_mouse(*dragging, input.mouse_x, true);
            }
            slider_dragging = invalid_widget;
            pressed = invalid_widget;
        } else {
            const Widget* clicked = find_widget(screen, pressed);
            if (clicked && hovered == pressed) {
                if (!text_editing) {
                    const DrawItem* item = find_draw_item(clicked->id);
                    const ControlPart part =
                        item ? hit_control_part(*item, input.mouse_x, input.mouse_y)
                             : ControlPart::Body;
                    if (clicked->type == WidgetType::Slider1D) {
                        activate_control_part(*clicked, part, input.mouse_x);
                    } else if (clicked->type == WidgetType::OptionCycle) {
                        activate_control_part(*clicked, part, input.mouse_x);
                    } else {
                        activate_widget(*clicked);
                    }
                } else if (clicked->id != editing) {
                    const Widget* edited = find_widget(screen, editing);
                    if (edited) {
                        finish_text_edit(*edited, true);
                    } else {
                        editing = invalid_widget;
                        editing_start_value.clear();
                    }
                    activate_widget(*clicked);
                }
            }
            pressed = invalid_widget;
        }
    } else if (input.mouse_down && slider_dragging != invalid_widget) {
        const Widget* dragging = find_widget(screen, slider_dragging);
        if (dragging) {
            set_slider_from_mouse(*dragging, input.mouse_x, false);
        }
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
    } else if (!text_editing && input.select && active && active->disabled) {
        record_widget_feedback(FeedbackType::Rejected, active->id);
    } else if (!text_editing && input.select && active) {
        record_widget_feedback(FeedbackType::Rejected, active->id);
    }

    if (focused != frame_start_focus) {
        record_feedback(FeedbackEvent{
            FeedbackType::FocusMoved,
            focused,
            frame_start_focus,
            focused,
        });
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
        editing_start_value.clear();
        return;
    }

    if (input.back) {
        finish_text_edit(*active, true);
        return;
    }

    if (input.select) {
        finish_text_edit(*active, true);
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

void Menu::finish_text_edit(const Widget& widget, bool run_commit) {
    if (editing != widget.id) {
        return;
    }

    const bool changed = widget.text_value != nullptr && *widget.text_value != editing_start_value;
    editing = invalid_widget;
    editing_start_value.clear();
    record_widget_feedback(FeedbackType::TextEditEnded, widget.id);
    if (run_commit && changed) {
        execute(widget.on_commit);
    }
}

void Menu::activate_widget(const Widget& widget) {
    if (widget.disabled) {
        record_widget_feedback(FeedbackType::Rejected, widget.id);
        return;
    }
    bool activated = true;
    if (widget.type == WidgetType::Toggle && widget.bool_value) {
        *widget.bool_value = !*widget.bool_value;
    } else if (widget.type == WidgetType::Slider1D && widget.float_value) {
        adjust_widget(widget, 1);
        activated = false;
    } else if (widget.type == WidgetType::OptionCycle) {
        adjust_widget(widget, 1);
        activated = false;
    } else if (widget.type == WidgetType::TextInput && widget.text_value &&
               widget.select_enters_text) {
        editing = widget.id;
        editing_start_value = *widget.text_value;
        record_widget_feedback(FeedbackType::TextEditStarted, widget.id);
        activated = false;
    }
    if (activated) {
        record_widget_feedback(FeedbackType::Activated, widget.id);
    }
    execute(widget.on_select);
}

void Menu::adjust_widget(const Widget& widget, int direction) {
    if (widget.disabled) {
        record_widget_feedback(FeedbackType::Rejected, widget.id);
        return;
    }
    bool adjusted = false;
    if (widget.type == WidgetType::Slider1D && widget.float_value) {
        const float before = *widget.float_value;
        *widget.float_value =
            clamp_value(*widget.float_value + (widget.step * static_cast<float>(direction)),
                        widget.min, widget.max);
        adjusted = *widget.float_value != before;
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
        adjusted = true;
    }
    if (adjusted) {
        record_widget_feedback(
            direction < 0 ? FeedbackType::AdjustedLeft : FeedbackType::AdjustedRight, widget.id);
        execute(widget.on_commit);
    } else {
        record_widget_feedback(FeedbackType::Rejected, widget.id);
    }
}

void Menu::set_slider_from_mouse(const Widget& widget, float mouse_x, bool commit) {
    if (widget.disabled || widget.type != WidgetType::Slider1D || !widget.float_value) {
        return;
    }

    const DrawItem* item = find_draw_item(widget.id);
    if (!item || !item->controls.has_slider_track || item->controls.slider_track.w <= 0.0f) {
        return;
    }

    float ratio = (mouse_x - item->controls.slider_track.x) / item->controls.slider_track.w;
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    const float before = *widget.float_value;
    *widget.float_value = widget.min + ((widget.max - widget.min) * ratio);
    if (*widget.float_value != before) {
        record_widget_feedback(
            ratio < 0.5f ? FeedbackType::AdjustedLeft : FeedbackType::AdjustedRight, widget.id);
    }
    if (commit) {
        execute(widget.on_commit);
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

WidgetId Menu::resolve_return_nav(const Screen& screen, WidgetId from,
                                  NavDirection direction) const {
    for (auto it = nav_returns.rbegin(); it != nav_returns.rend(); ++it) {
        if (it->from != from || it->direction != direction || it->target == invalid_widget) {
            continue;
        }
        const Widget* target = find_widget(screen, it->target);
        if (target && is_selectable(*target)) {
            return it->target;
        }
    }
    return invalid_widget;
}

void Menu::remember_return_nav(WidgetId from, WidgetId to, NavDirection return_direction) {
    if (from == invalid_widget || to == invalid_widget || from == to) {
        return;
    }

    for (auto it = nav_returns.begin(); it != nav_returns.end();) {
        if (it->from == to && it->direction == return_direction) {
            it = nav_returns.erase(it);
        } else {
            ++it;
        }
    }

    nav_returns.push_back(NavReturn{to, return_direction, from});
    constexpr std::size_t max_returns = 8;
    while (nav_returns.size() > max_returns) {
        nav_returns.erase(nav_returns.begin());
    }
}

void Menu::move_focus_with_return(const Screen& screen, WidgetId target,
                                  NavDirection return_direction) {
    const Widget* target_widget = find_widget(screen, target);
    if (!target_widget || !is_selectable(*target_widget)) {
        return;
    }
    const WidgetId previous = focused;
    focused = target;
    remember_return_nav(previous, target, return_direction);
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

const DrawItem* Menu::find_draw_item(WidgetId id) const {
    for (const DrawItem& item : items) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

ControlPart Menu::hit_control_part(const DrawItem& item, float x, float y) const {
    if (item.controls.has_slider_track && contains(item.controls.slider_track, x, y)) {
        return ControlPart::SliderTrack;
    }
    if (item.controls.has_option_left && contains(item.controls.option_left, x, y)) {
        return ControlPart::OptionLeft;
    }
    if (item.controls.has_option_right && contains(item.controls.option_right, x, y)) {
        return ControlPart::OptionRight;
    }
    if (item.controls.has_option_value && contains(item.controls.option_value, x, y)) {
        return ControlPart::OptionValue;
    }
    if (contains(item.rect, x, y)) {
        return ControlPart::Body;
    }
    return ControlPart::None;
}

void Menu::activate_control_part(const Widget& widget, ControlPart part, float mouse_x) {
    if (part == ControlPart::SliderTrack && widget.type == WidgetType::Slider1D) {
        set_slider_from_mouse(widget, mouse_x, true);
        return;
    }
    if (part == ControlPart::OptionLeft && widget.type == WidgetType::OptionCycle) {
        if (widget.on_left.type != ActionType::None) {
            record_widget_feedback(FeedbackType::AdjustedLeft, widget.id);
            execute(widget.on_left);
        } else {
            adjust_widget(widget, -1);
        }
        return;
    }
    if (part == ControlPart::OptionRight && widget.type == WidgetType::OptionCycle) {
        if (widget.on_right.type != ActionType::None) {
            record_widget_feedback(FeedbackType::AdjustedRight, widget.id);
            execute(widget.on_right);
        } else {
            adjust_widget(widget, 1);
        }
        return;
    }
    if (part == ControlPart::OptionValue && widget.type == WidgetType::OptionCycle) {
        activate_widget(widget);
        return;
    }
    activate_widget(widget);
}

void Menu::lock_mouse_focus(const Input& input) {
    allow_mouse_focus = false;
    if (!input.mouse_valid) {
        mouse_focus_locked = false;
        return;
    }
    mouse_focus_locked = true;
    mouse_focus_lock_x = input.mouse_x;
    mouse_focus_lock_y = input.mouse_y;
}

void Menu::ensure_mouse_focus_lock(const Input& input) {
    if (allow_mouse_focus || mouse_focus_locked || !input.mouse_valid) {
        return;
    }
    lock_mouse_focus(input);
}

void Menu::unlock_mouse_focus_if_moved(const Input& input) {
    if (!mouse_focus_locked || !input.mouse_valid) {
        return;
    }
    if (input.mouse_x != mouse_focus_lock_x || input.mouse_y != mouse_focus_lock_y) {
        unlock_mouse_focus_now();
    }
}

void Menu::unlock_mouse_focus_now() {
    allow_mouse_focus = true;
    mouse_focus_locked = false;
}

void Menu::record_feedback(FeedbackEvent event) {
    feedback_events.push_back(event);
}

void Menu::record_widget_feedback(FeedbackType type, WidgetId widget) {
    FeedbackEvent event;
    event.type = type;
    event.widget = widget;
    record_feedback(event);
}

void Menu::flush_feedback() {
    if (!feedback_hooks) {
        feedback_events.clear();
        return;
    }
    for (const FeedbackEvent& event : feedback_events) {
        dispatch_feedback(event);
    }
    feedback_events.clear();
}

void Menu::dispatch_feedback(const FeedbackEvent& event) const {
    switch (event.type) {
    case FeedbackType::FocusMoved:
        if (feedback_hooks->focus_moved) {
            feedback_hooks->focus_moved(feedback_hooks->user, event.from, event.to);
        }
        break;
    case FeedbackType::Activated:
        if (feedback_hooks->activated) {
            feedback_hooks->activated(feedback_hooks->user, event.widget);
        }
        break;
    case FeedbackType::Rejected:
        if (feedback_hooks->rejected) {
            feedback_hooks->rejected(feedback_hooks->user, event.widget);
        }
        break;
    case FeedbackType::AdjustedLeft:
        if (feedback_hooks->adjusted_left) {
            feedback_hooks->adjusted_left(feedback_hooks->user, event.widget);
        }
        break;
    case FeedbackType::AdjustedRight:
        if (feedback_hooks->adjusted_right) {
            feedback_hooks->adjusted_right(feedback_hooks->user, event.widget);
        }
        break;
    case FeedbackType::TextEditStarted:
        if (feedback_hooks->text_edit_started) {
            feedback_hooks->text_edit_started(feedback_hooks->user, event.widget);
        }
        break;
    case FeedbackType::TextEditEnded:
        if (feedback_hooks->text_edit_ended) {
            feedback_hooks->text_edit_ended(feedback_hooks->user, event.widget);
        }
        break;
    }
}

} // namespace gmenu
