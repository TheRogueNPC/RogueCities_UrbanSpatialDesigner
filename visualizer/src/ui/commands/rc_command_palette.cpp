#include "ui/commands/rc_command_palette.h"

#include <imgui.h>

#include <string>

namespace RC_UI::Commands {

void RequestOpenCommandPalette(CommandPaletteState& state) {
    state.open_requested = true;
}

void DrawCommandPalette(
    CommandPaletteState& state,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id) {
    if (state.open_requested) {
        ImGui::OpenPopup(popup_id);
        state.open_requested = false;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport != nullptr) {
        const ImVec2 center(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }
    ImGui::SetNextWindowSize(ImVec2(640.0f, 420.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_NoResize)) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    ImGui::TextUnformatted("Global Command Palette");
    ImGui::Separator();

    ImGui::InputTextWithHint("##CommandFilter", "Search commands...", state.filter, sizeof(state.filter));
    const std::vector<const RC_UI::Tools::ToolActionSpec*> filtered = FilterCommandRegistry(state.filter, true);

    ImGui::BeginChild("##CommandPaletteList", ImVec2(0.0f, -36.0f), true);
    for (const auto* action : filtered) {
        if (action == nullptr) {
            continue;
        }

        const bool enabled = RC_UI::Tools::IsToolActionEnabled(*action);
        if (!enabled) {
            ImGui::BeginDisabled(true);
        }

        const bool selected = ImGui::Selectable(action->label, false);
        if (!enabled) {
            ImGui::EndDisabled();
        }

        if (ImGui::IsItemHovered()) {
            const char* tooltip = action->tooltip != nullptr ? action->tooltip : "";
            ImGui::SetTooltip("%s", tooltip);
        }

        if (selected) {
            std::string status;
            const bool executed = ExecuteCommand(action->id, dispatch_context, &status);
            (void)executed;
            ImGui::CloseCurrentPopup();
            break;
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f))) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

} // namespace RC_UI::Commands
