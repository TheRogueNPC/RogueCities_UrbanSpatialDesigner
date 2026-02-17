// FILE: rc_ui_input_gate.h
// PURPOSE: Centralized input capture arbitration for viewport interactions.
// CONTRACT: Viewport-local interactions gate on canvas hover, NOT global WantCaptureMouse.
#pragma once

#include <imgui.h>
#include <string>

namespace RC_UI {

enum class InputBlockReason {
    None,
    CanvasNotHovered,
    PopupOrModalActive,
    TextInputActive,
    OverlayBlocking,
    WindowNotHovered
};

struct UiInputGateState {
    bool imgui_wants_mouse = false;
    bool imgui_wants_keyboard = false;
    bool imgui_wants_text_input = false;
    bool viewport_window_hovered = false;
    bool viewport_canvas_hovered = false;
    bool viewport_canvas_active = false;
    bool blocked_by_active_item = false;
    bool blocked_by_overlay = false;
    bool any_popup_open = false;
    bool any_modal_open = false;
    bool allow_viewport_mouse_actions = false;
    bool allow_viewport_key_actions = false;
    InputBlockReason mouse_block_reason = InputBlockReason::None;
    InputBlockReason key_block_reason = InputBlockReason::None;
};

[[nodiscard]] UiInputGateState BuildUiInputGateState(bool viewport_window_hovered,
                                                     bool viewport_canvas_hovered,
                                                     bool viewport_canvas_active,
                                                     bool any_item_active,
                                                     bool blocked_by_overlay);

[[nodiscard]] bool AllowViewportMouseActions(const UiInputGateState& gate);
[[nodiscard]] bool AllowViewportKeyActions(const UiInputGateState& gate);
[[nodiscard]] const char* GetBlockReasonString(InputBlockReason reason);

} // namespace RC_UI
