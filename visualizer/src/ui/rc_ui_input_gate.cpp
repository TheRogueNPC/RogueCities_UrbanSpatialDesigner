// FILE: rc_ui_input_gate.cpp
// PURPOSE: Centralized input capture arbitration for viewport interactions.

#include "ui/rc_ui_input_gate.h"

namespace RC_UI {

UiInputGateState BuildUiInputGateState(bool viewport_window_hovered,
                                       bool viewport_rect_hovered,
                                       bool any_item_active,
                                       bool blocked_by_overlay) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool any_popup_open = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

    UiInputGateState state{};
    state.imgui_wants_mouse = io.WantCaptureMouse;
    state.imgui_wants_keyboard = io.WantCaptureKeyboard;
    state.imgui_wants_text_input = io.WantTextInput;

    state.allow_viewport_mouse_actions =
        viewport_window_hovered &&
        viewport_rect_hovered &&
        !any_item_active &&
        !blocked_by_overlay &&
        !state.imgui_wants_mouse &&
        !any_popup_open;

    state.allow_viewport_key_actions =
        viewport_window_hovered &&
        !blocked_by_overlay &&
        !state.imgui_wants_keyboard &&
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
