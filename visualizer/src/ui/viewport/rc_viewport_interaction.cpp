#include "ui/viewport/rc_viewport_interaction.h"

namespace RC_UI::Viewport {
namespace {

void RequestDefaultContextCommandMenu(
    const RogueCity::Core::Editor::EditorConfig& config,
    const ImVec2& screen_pos,
    const CommandMenuStateBundle& state_bundle) {
    using RogueCity::Core::Editor::ViewportCommandMode;
    switch (config.viewport_context_default_mode) {
        case ViewportCommandMode::SmartList:
            if (state_bundle.smart_menu != nullptr) {
                RC_UI::Commands::RequestOpenSmartMenu(*state_bundle.smart_menu, screen_pos);
            }
            break;
        case ViewportCommandMode::Pie:
            if (state_bundle.pie_menu != nullptr) {
                RC_UI::Commands::RequestOpenPieMenu(*state_bundle.pie_menu, screen_pos);
            }
            break;
        case ViewportCommandMode::Palette:
        default:
            if (state_bundle.command_palette != nullptr) {
                RC_UI::Commands::RequestOpenCommandPalette(*state_bundle.command_palette);
            }
            break;
    }
}

} // namespace

void ProcessViewportCommandTriggers(
    const CommandInteractionParams& params,
    const CommandMenuStateBundle& state_bundle) {
    if (params.editor_config == nullptr) {
        return;
    }

    const bool allow_viewport_mouse_actions = RC_UI::AllowViewportMouseActions(params.input_gate);
    const bool allow_viewport_key_actions = RC_UI::AllowViewportKeyActions(params.input_gate);
    const ImGuiIO& io = ImGui::GetIO();

    if (allow_viewport_mouse_actions &&
        params.in_viewport &&
        !params.minimap_hovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !io.KeyAlt &&
        !io.KeyShift &&
        !io.KeyCtrl) {
        RequestDefaultContextCommandMenu(*params.editor_config, params.mouse_pos, state_bundle);
    }

    const bool allow_command_hotkeys =
        allow_viewport_key_actions &&
        params.in_viewport &&
        !params.minimap_hovered &&
        !io.WantTextInput &&
        !ImGui::IsAnyItemActive();
    if (!allow_command_hotkeys) {
        return;
    }

    if (params.editor_config->viewport_hotkey_space_enabled && ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (state_bundle.smart_menu != nullptr) {
            RC_UI::Commands::RequestOpenSmartMenu(*state_bundle.smart_menu, params.mouse_pos);
        }
    }

    if ((params.editor_config->viewport_hotkey_slash_enabled && ImGui::IsKeyPressed(ImGuiKey_Slash, false)) ||
        (params.editor_config->viewport_hotkey_grave_enabled && ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false))) {
        if (state_bundle.pie_menu != nullptr) {
            RC_UI::Commands::RequestOpenPieMenu(*state_bundle.pie_menu, params.mouse_pos);
        }
    }

    if (params.editor_config->viewport_hotkey_p_enabled &&
        !io.KeyCtrl &&
        ImGui::IsKeyPressed(ImGuiKey_P, false)) {
        if (state_bundle.command_palette != nullptr) {
            RC_UI::Commands::RequestOpenCommandPalette(*state_bundle.command_palette);
        }
    }
}

} // namespace RC_UI::Viewport
