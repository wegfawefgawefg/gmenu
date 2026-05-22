#pragma once

#include "ginput/profile.hpp"
#include "glayout/layout.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gmenu {

using ScreenId = std::uint32_t;
using WidgetId = std::uint32_t;
using CommandId = std::uint32_t;
using StyleId = std::uint32_t;

inline constexpr ScreenId invalid_screen = 0;
inline constexpr WidgetId invalid_widget = 0;
inline constexpr CommandId invalid_command = 0;
inline constexpr StyleId default_style = 1;

enum class WidgetType : std::uint8_t {
    Label,
    Button,
    Card,
    Toggle,
    Slider1D,
    OptionCycle,
    TextInput,
};

enum class ActionType : std::uint8_t {
    None,
    Push,
    Pop,
    Replace,
    SetRoot,
    Command,
    SetInt,
    DeltaInt,
    RemoveButtonBind,
};

enum class NavDirection : std::uint8_t {
    Up,
    Down,
    Left,
    Right,
};

enum class NavSource : std::uint8_t {
    None,
    Explicit,
    Override,
};

enum class FeedbackType : std::uint8_t {
    FocusMoved,
    Activated,
    Rejected,
    AdjustedLeft,
    AdjustedRight,
    TextEditStarted,
    TextEditEnded,
};

struct FeedbackEvent {
    FeedbackType type = FeedbackType::Rejected;
    WidgetId widget = invalid_widget;
    WidgetId from = invalid_widget;
    WidgetId to = invalid_widget;
};

struct FeedbackHooks {
    void* user = nullptr;
    void (*focus_moved)(void* user, WidgetId from, WidgetId to) = nullptr;
    void (*activated)(void* user, WidgetId widget) = nullptr;
    void (*rejected)(void* user, WidgetId widget) = nullptr;
    void (*adjusted_left)(void* user, WidgetId widget) = nullptr;
    void (*adjusted_right)(void* user, WidgetId widget) = nullptr;
    void (*text_edit_started)(void* user, WidgetId widget) = nullptr;
    void (*text_edit_ended)(void* user, WidgetId widget) = nullptr;
};

struct NavLinks {
    WidgetId up = invalid_widget;
    WidgetId down = invalid_widget;
    WidgetId left = invalid_widget;
    WidgetId right = invalid_widget;
};

struct NavScope {
    ScreenId screen = invalid_screen;
    int layout_id = 0;
    int width = 0;
    int height = 0;
    glayout::FormFactor form_factor = glayout::FormFactor::Desktop;
    bool match_resolution = false;
    bool match_form_factor = false;
};

struct NavOverride {
    NavScope scope;
    WidgetId widget = invalid_widget;
    NavLinks links;
};

struct NavValidationIssue {
    ScreenId screen = invalid_screen;
    WidgetId widget = invalid_widget;
    WidgetId target = invalid_widget;
    NavDirection direction = NavDirection::Down;
    std::string message;
};

struct Action {
    ActionType type = ActionType::None;
    ScreenId screen = invalid_screen;
    CommandId command = invalid_command;
    int payload = 0;
    int* int_value = nullptr;
    int min = 0;
    int max = 0;
    ginput::InputProfile* input_profile = nullptr;
    ginput::ActionId input_action = -1;
    ginput::EncodedControl encoded_control = 0;

    static Action none();
    static Action push(ScreenId id);
    static Action pop();
    static Action replace(ScreenId id);
    static Action set_root(ScreenId id);
    static Action command_id(CommandId id, int value = 0);
    static Action set_int(int& value, int target);
    static Action delta_int(int& value, int delta, int min_value, int max_value);
    static Action remove_button_bind(ginput::InputProfile& profile, ginput::ActionId action,
                                     ginput::EncodedControl encoded_control);
};

struct Widget {
    WidgetId id = invalid_widget;
    WidgetType type = WidgetType::Label;
    std::string slot;
    std::string label;
    std::string secondary;
    std::string value;
    StyleId style = default_style;
    bool disabled = false;
    bool select_enters_text = true;

    bool* bool_value = nullptr;
    float* float_value = nullptr;
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.1f;
    int option_count = 0;
    int* option_index = nullptr;
    std::vector<std::string> options;
    std::string* text_value = nullptr;
    int text_max_len = 0;

    Action on_select;
    Action on_left;
    Action on_right;
    Action on_back;
    Action on_commit;

    WidgetId nav_up = invalid_widget;
    WidgetId nav_down = invalid_widget;
    WidgetId nav_left = invalid_widget;
    WidgetId nav_right = invalid_widget;
};

struct Screen {
    ScreenId id = invalid_screen;
    int layout_id = 0;
    WidgetId default_focus = invalid_widget;
    std::vector<Widget> widgets;
};

struct Input {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool select = false;
    bool back = false;
    bool page_prev = false;
    bool page_next = false;
    bool mouse_valid = false;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    bool mouse_down = false;
    std::string text;
    bool backspace = false;
    bool delete_word = false;
};

struct VisualState {
    bool focused = false;
    bool hovered = false;
    bool pressed = false;
    bool disabled = false;
    bool editing = false;
    float focus_time = 0.0f;
    float hover_time = 0.0f;
    float press_time = 0.0f;
};

struct DrawItem {
    WidgetId id = invalid_widget;
    WidgetType type = WidgetType::Label;
    glayout::Rect rect;
    StyleId style = default_style;
    std::string label;
    std::string secondary;
    std::string value;
    VisualState state;
    WidgetId nav_up = invalid_widget;
    WidgetId nav_down = invalid_widget;
    WidgetId nav_left = invalid_widget;
    WidgetId nav_right = invalid_widget;
    NavSource nav_up_source = NavSource::None;
    NavSource nav_down_source = NavSource::None;
    NavSource nav_left_source = NavSource::None;
    NavSource nav_right_source = NavSource::None;
};

} // namespace gmenu
