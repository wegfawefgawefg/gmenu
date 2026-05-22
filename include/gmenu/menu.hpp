#pragma once

#include "ginput/runtime.hpp"
#include "gmenu/input.hpp"
#include "gmenu/types.hpp"
#include "gmenu/widgets.hpp"

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace gmenu {

class Menu;

struct BuildContext {
    Menu& menu;
    ScreenId screen = invalid_screen;
    void* user = nullptr;
    const void* screen_data = nullptr;
};

using ScreenBuildFn = void (*)(BuildContext& ctx, Screen& out);
using CommandFn = void (*)(BuildContext& ctx, int payload);

struct ScreenDef {
    ScreenId id = invalid_screen;
    ScreenBuildFn build = nullptr;
    const void* data = nullptr;
};

class Menu {
  public:
    void set_layouts(const std::vector<glayout::Layout>* layout_list);
    void set_layout_store(const glayout::LayoutStore* store);
    void set_user_data(void* ptr);

    void register_screen(ScreenId id, ScreenBuildFn build, const void* data = nullptr);
    CommandId register_command(CommandFn fn);

    bool set_root(ScreenId id);
    bool push(ScreenId id);
    bool replace(ScreenId id);
    void pop();
    void clear();

    void update(const Input& input, float dt, int width, int height,
                glayout::FormFactor form_factor = glayout::FormFactor::Desktop);

    std::span<const DrawItem> draw_items() const;
    std::span<const ScreenId> stack() const;
    WidgetId focus() const;
    ScreenId current_screen() const;
    void* user_data() const;

    void set_nav_link(ScreenId screen, WidgetId widget, NavDirection direction, WidgetId target);
    void clear_nav_link(ScreenId screen, WidgetId widget, NavDirection direction);
    void clear_nav_links(ScreenId screen, WidgetId widget);
    NavLinks nav_links(ScreenId screen, WidgetId widget) const;

  private:
    struct ScreenInstance {
        ScreenId id = invalid_screen;
    };

    const ScreenDef* find_screen(ScreenId id) const;
    Screen build_current_screen(int width, int height, glayout::FormFactor form_factor);
    void apply_nav_overrides(Screen& screen) const;
    void rebuild_draw_items(const Screen& screen, int width, int height,
                            glayout::FormFactor form_factor);
    void update_focus(const Screen& screen, const Input& input, float dt);
    void update_text_input(const Screen& screen, const Input& input);
    void activate_widget(const Widget& widget);
    void adjust_widget(const Widget& widget, int direction);
    void set_slider_from_mouse(const Widget& widget, const Input& input);
    void execute(const Action& action);
    void invoke_command(CommandId id, int payload);
    WidgetId first_selectable(const Screen& screen) const;
    WidgetId resolve_nav(const Screen& screen, WidgetId from, WidgetId explicit_target,
                         int direction) const;
    const Widget* find_widget(const Screen& screen, WidgetId id) const;
    Widget* find_widget(Screen& screen, WidgetId id) const;
    WidgetId hovered_widget(const Screen& screen, const Input& input) const;
    bool is_selectable(const Widget& widget) const;

    std::vector<ScreenDef> screens;
    std::vector<CommandFn> commands;
    std::vector<ScreenInstance> instances;
    std::vector<ScreenId> public_stack;
    std::vector<DrawItem> items;
    const std::vector<glayout::Layout>* layouts = nullptr;
    const glayout::LayoutStore* layout_store = nullptr;
    std::unordered_map<std::uint64_t, NavLinks> nav_overrides;
    void* user = nullptr;

    WidgetId focused = invalid_widget;
    WidgetId hovered = invalid_widget;
    WidgetId pressed = invalid_widget;
    WidgetId editing = invalid_widget;
    bool prev_mouse_down = false;
    ginput::RepeatState repeat_up;
    ginput::RepeatState repeat_down;
    ginput::RepeatState repeat_left;
    ginput::RepeatState repeat_right;
    std::unordered_map<WidgetId, float> focus_times;
    std::unordered_map<WidgetId, float> hover_times;
    std::unordered_map<WidgetId, float> press_times;
};

} // namespace gmenu
