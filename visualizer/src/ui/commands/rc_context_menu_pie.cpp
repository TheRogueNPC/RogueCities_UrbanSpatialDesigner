#include "ui/commands/rc_context_menu_pie.h"

#include <string>
#include <vector>

namespace RC_UI::Commands {
namespace {

std::vector<const RC_UI::Tools::ToolActionSpec*> BuildPieSliceActions(ToolLibrary preferred_library) {
    std::vector<const RC_UI::Tools::ToolActionSpec*> slices;
    slices.reserve(8);

    const auto registry = GetCommandRegistry();
    for (const auto& action : registry) {
        if (action.library != preferred_library) {
            continue;
        }
        slices.push_back(&action);
        if (slices.size() == 8) {
            return slices;
        }
    }

    for (const auto& action : registry) {
        if (action.library == preferred_library) {
            continue;
        }
        slices.push_back(&action);
        if (slices.size() == 8) {
            break;
        }
    }

    return slices;
}

} // namespace

void RequestOpenPieMenu(PieMenuState& state, const ImVec2& screen_pos) {
    state.open_requested = true;
    state.open_pos = screen_pos;
}

void DrawPieMenu(
    PieMenuState& state,
    ToolLibrary preferred_library,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id) {
    if (state.open_requested) {
        ImGui::SetNextWindowPos(state.open_pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup(popup_id);
        state.open_requested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(360.0f, 240.0f), ImGuiCond_Always);
    if (!ImGui::BeginPopup(popup_id)) {
        return;
    }

    const auto slices = BuildPieSliceActions(preferred_library);
    if (slices.empty()) {
        ImGui::TextUnformatted("No commands available.");
        ImGui::EndPopup();
        return;
    }

    ImGui::Text("Pie: %s", CommandLibraryName(preferred_library));
    ImGui::Separator();

    size_t action_index = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (row == 1 && col == 1) {
                ImGui::Dummy(ImVec2(92.0f, 32.0f));
            } else if (action_index < slices.size()) {
                const auto* action = slices[action_index];
                const bool enabled = action != nullptr && RC_UI::Tools::IsToolActionEnabled(*action);
                if (!enabled) {
                    ImGui::BeginDisabled(true);
                }

                const char* label = (action != nullptr && action->label != nullptr) ? action->label : "(invalid)";
                if (ImGui::Button(label, ImVec2(92.0f, 32.0f)) && action != nullptr) {
                    std::string status;
                    const bool executed = ExecuteCommand(action->id, dispatch_context, &status);
                    (void)executed;
                    ImGui::CloseCurrentPopup();
                }

                if (!enabled) {
                    ImGui::EndDisabled();
                }

                if (action != nullptr && ImGui::IsItemHovered() && action->tooltip != nullptr && action->tooltip[0] != '\0') {
                    ImGui::SetTooltip("%s", action->tooltip);
                }
                ++action_index;
            }

            if (col < 2) {
                ImGui::SameLine();
            }
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Global");
    for (const auto& command : GetGlobalViewportCommands()) {
        if (ImGui::Button(command.label, ImVec2(112.0f, 26.0f))) {
            std::string status;
            const bool executed = ExecuteGlobalViewportCommand(command.id, dispatch_context, &status);
            (void)executed;
            ImGui::CloseCurrentPopup();
            break;
        }
        if (ImGui::IsItemHovered() && command.tooltip != nullptr && command.tooltip[0] != '\0') {
            ImGui::SetTooltip("%s", command.tooltip);
        }
        ImGui::SameLine();
    }

    ImGui::EndPopup();
}

} // namespace RC_UI::Commands
