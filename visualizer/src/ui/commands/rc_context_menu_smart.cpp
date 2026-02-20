#include "ui/commands/rc_context_menu_smart.h"

#include <string>

namespace RC_UI::Commands {

void RequestOpenSmartMenu(
    SmartMenuState& state,
    const ImVec2& screen_pos,
    std::optional<ToolLibrary> preferred_library) {
    state.open_requested = true;
    state.open_pos = screen_pos;
    state.has_preferred_library = preferred_library.has_value();
    if (preferred_library.has_value()) {
        state.preferred_library = *preferred_library;
    }
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
        state.has_preferred_library = false;
        return;
    }

    const auto draw_library_menu = [&](ToolLibrary library) {
        if (!ImGui::BeginMenu(CommandLibraryName(library))) {
            return;
        }

        const auto actions = GetCommandRegistry();
        for (const auto& action : actions) {
            if (action.library != library) {
                continue;
            }
            ImGui::PushID(static_cast<int>(action.id));

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
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        ImGui::EndMenu();
    };

    if (state.has_preferred_library) {
        draw_library_menu(state.preferred_library);
    } else {
        for (ToolLibrary library : kToolLibraryOrder) {
            draw_library_menu(library);
        }
    }

    if (ImGui::BeginMenu("Global")) {
        int command_index = 0;
        for (const auto& command : GetGlobalViewportCommands()) {
            ImGui::PushID(command_index++);
            if (ImGui::Selectable(command.label, false)) {
                std::string status;
                const bool executed = ExecuteGlobalViewportCommand(command.id, dispatch_context, &status);
                (void)executed;
                ImGui::CloseCurrentPopup();
                ImGui::PopID();
                break;
            }
            if (ImGui::IsItemHovered() && command.tooltip != nullptr && command.tooltip[0] != '\0') {
                ImGui::SetTooltip("%s", command.tooltip);
            }
            ImGui::PopID();
        }
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

} // namespace RC_UI::Commands
