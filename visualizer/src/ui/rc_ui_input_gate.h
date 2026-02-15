// FILE: rc_ui_input_gate.h
// PURPOSE: Centralized input capture arbitration for viewport interactions.
#pragma once

#include <imgui.h>

namespace RC_UI {

struct UiInputGateState {
    bool imgui_wants_mouse = false;
    bool imgui_wants_keyboard = false;
    bool imgui_wants_text_input = false;
    bool allow_viewport_mouse_actions = false;
    bool allow_viewport_key_actions = false;
};

[[nodiscard]] UiInputGateState BuildUiInputGateState(bool viewport_window_hovered,
                                                     bool viewport_rect_hovered,
                                                     bool any_item_active,
                                                     bool blocked_by_overlay);

[[nodiscard]] bool AllowViewportMouseActions(const UiInputGateState& gate);
[[nodiscard]] bool AllowViewportKeyActions(const UiInputGateState& gate);

} // namespace RC_UI

