// FILE: rc_ui_input_gate.cpp
// PURPOSE: Centralized input capture arbitration for viewport interactions.
// CONTRACT: Canvas-local gating - viewport interactions allowed when canvas hovered,
//           regardless of global WantCaptureMouse (which is true for ImGui windows).

#include "ui/rc_ui_input_gate.h"

namespace RC_UI {

const char* GetBlockReasonString(InputBlockReason reason) {
    switch (reason) {
        case InputBlockReason::None: return "None";
        case InputBlockReason::CanvasNotHovered: return "Canvas not hovered";
        case InputBlockReason::PopupOrModalActive: return "Popup/modal active";
        case InputBlockReason::TextInputActive: return "Text input active";
        case InputBlockReason::OverlayBlocking: return "Overlay blocking";
        case InputBlockReason::WindowNotHovered: return "Window not hovered";
        default: return "Unknown";
    }
}

UiInputGateState BuildUiInputGateState(bool viewport_window_hovered,
                                       bool viewport_canvas_hovered,
                                       bool viewport_canvas_active,
                                       bool any_item_active,
                                       bool blocked_by_overlay) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool any_popup_open = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
    const bool any_modal_open = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

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
    state.any_modal_open = any_modal_open;

    // CRITICAL: Viewport-local mouse interaction contract.
    // Mouse actions are allowed when:
    //   1. Canvas is hovered (not just window) - this is the canonical authority
    //   2. No popup/modal is stealing input
    //   3. No text input field is active
    //   4. No overlay (minimap, etc.) is blocking
    // NOTE: We do NOT block on imgui_wants_mouse because the viewport canvas
    //       itself is an ImGui widget, so WantCaptureMouse is always true!
    //       The canvas hover check is our viewport-local gate.
    state.mouse_block_reason = InputBlockReason::None;
    if (!viewport_canvas_hovered) {
        state.mouse_block_reason = InputBlockReason::CanvasNotHovered;
    } else if (any_popup_open || any_modal_open) {
        state.mouse_block_reason = InputBlockReason::PopupOrModalActive;
    } else if (state.imgui_wants_text_input) {
        state.mouse_block_reason = InputBlockReason::TextInputActive;
    } else if (blocked_by_overlay) {
        state.mouse_block_reason = InputBlockReason::OverlayBlocking;
    }

    state.allow_viewport_mouse_actions =
        viewport_canvas_hovered &&
        !state.blocked_by_active_item &&
        !blocked_by_overlay &&
        !state.imgui_wants_text_input &&
        !any_popup_open &&
        !any_modal_open;

    // Keyboard actions are less strict - allow when window hovered
    state.key_block_reason = InputBlockReason::None;
    if (!viewport_window_hovered) {
        state.key_block_reason = InputBlockReason::WindowNotHovered;
    } else if (any_popup_open || any_modal_open) {
        state.key_block_reason = InputBlockReason::PopupOrModalActive;
    } else if (state.imgui_wants_text_input) {
        state.key_block_reason = InputBlockReason::TextInputActive;
    } else if (blocked_by_overlay) {
        state.key_block_reason = InputBlockReason::OverlayBlocking;
    }

    state.allow_viewport_key_actions =
        viewport_window_hovered &&
        !state.blocked_by_active_item &&
        !blocked_by_overlay &&
        !state.imgui_wants_text_input &&
        !any_popup_open &&
        !any_modal_open;

    return state;
}

bool AllowViewportMouseActions(const UiInputGateState& gate) {
    return gate.allow_viewport_mouse_actions;
}

bool AllowViewportKeyActions(const UiInputGateState& gate) {
    return gate.allow_viewport_key_actions;
}

} // namespace RC_UI
