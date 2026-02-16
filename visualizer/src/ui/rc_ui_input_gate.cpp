// FILE: rc_ui_input_gate.cpp
// PURPOSE: Centralized input capture arbitration for viewport interactions.

#include "ui/rc_ui_input_gate.h"

namespace RC_UI {

UiInputGateState BuildUiInputGateState(bool viewport_window_hovered,
                                       bool viewport_canvas_hovered,
                                       bool viewport_canvas_active,
                                       bool any_item_active,
                                       bool blocked_by_overlay) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool any_popup_open = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

    UiInputGateState state{};
    state.imgui_wants_mouse = io.WantCaptureMouse;
    state.imgui_wants_keyboard = io.WantCaptureKeyboard;
    state.imgui_wants_text_input = io.WantTextInput;
    state.viewport_window_hovered = viewport_window_hovered;
    state.viewport_canvas_hovered = viewport_canvas_hovered;
    state.viewport_canvas_active = viewport_canvas_active;
    state.blocked_by_active_item = any_item_active && !viewport_canvas_active;
    state.blocked_by_overlay = blocked_by_overlay;
    state.any_popup_open = any_popup_open;

    state.allow_viewport_mouse_actions =
        viewport_window_hovered &&
        viewport_canvas_hovered &&
        !state.blocked_by_active_item &&
        !blocked_by_overlay &&
        !state.imgui_wants_text_input &&
        !any_popup_open;

    state.allow_viewport_key_actions =
        viewport_window_hovered &&
        !state.blocked_by_active_item &&
        !blocked_by_overlay &&
        !state.imgui_wants_text_input &&
        !any_popup_open;

    return state;
}

bool AllowViewportMouseActions(const UiInputGateState& gate) {
    return gate.allow_viewport_mouse_actions;
}

bool AllowViewportKeyActions(const UiInputGateState& gate) {
    return gate.allow_viewport_key_actions;
}

} // namespace RC_UI
