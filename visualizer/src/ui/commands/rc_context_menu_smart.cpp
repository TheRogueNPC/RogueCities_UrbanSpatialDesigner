#include "ui/commands/rc_context_menu_smart.h"

#include <string>

namespace RC_UI::Commands {

void RequestOpenSmartMenu(SmartMenuState& state, const ImVec2& screen_pos) {
    state.open_requested = true;
    state.open_pos = screen_pos;
}

void DrawSmartMenu(
    SmartMenuState& state,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id) {
    if (state.open_requested) {
        ImGui::SetNextWindowPos(state.open_pos, ImGuiCond_Always);
        ImGui::OpenPopup(popup_id);
        state.open_requested = false;
    }

    if (!ImGui::BeginPopup(popup_id)) {
        return;
    }

    for (ToolLibrary library : kToolLibraryOrder) {
        if (!ImGui::BeginMenu(CommandLibraryName(library))) {
            continue;
        }

        const auto actions = GetCommandRegistry();
        for (const auto& action : actions) {
            if (action.library != library) {
                continue;
            }

            const bool enabled = RC_UI::Tools::IsToolActionEnabled(action);
            if (!enabled) {
                ImGui::BeginDisabled(true);
            }

            const bool selected = ImGui::Selectable(action.label, false);
            if (!enabled) {
                ImGui::EndDisabled();
            }

            if (ImGui::IsItemHovered() && action.tooltip != nullptr && action.tooltip[0] != '\0') {
                ImGui::SetTooltip("%s", action.tooltip);
            }

            if (selected) {
                std::string status;
                const bool executed = ExecuteCommand(action.id, dispatch_context, &status);
                (void)executed;
                ImGui::CloseCurrentPopup();
                break;
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

} // namespace RC_UI::Commands
