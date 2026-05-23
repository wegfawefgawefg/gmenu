#include "gmenu/menu.hpp"

namespace gmenu {

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
