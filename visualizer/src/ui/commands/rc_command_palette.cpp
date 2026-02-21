// This file implements the Command Palette UI for the RogueCities visualizer.
// It provides functionality to display a searchable list of commands and execute them.

#include "ui/commands/rc_command_palette.h"

#include <imgui.h>

#include <cctype>
#include <string>
#include <string_view>

namespace {

// Helper function to perform a case-insensitive substring search.
// Converts both the haystack and needle to lowercase before searching.
bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }

    std::string lower_haystack;
    lower_haystack.reserve(haystack.size());
    for (char c : haystack) {
        lower_haystack.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    std::string lower_needle;
    lower_needle.reserve(needle.size());
    for (char c : needle) {
        lower_needle.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lower_haystack.find(lower_needle) != std::string::npos;
}

} // namespace

namespace RC_UI::Commands {

// Marks the Command Palette as open, so it will be displayed in the next frame.
void RequestOpenCommandPalette(CommandPaletteState& state) {
    state.open_requested = true;
}

// Draws the Command Palette modal window.
// Displays a list of commands filtered by the user's input and allows execution of selected commands.
void DrawCommandPalette(
    CommandPaletteState& state,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id) {
    if (state.open_requested) {
        ImGui::OpenPopup(popup_id);
        state.open_requested = false;
    }

    // Center the popup window on the main viewport.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport != nullptr) {
        const ImVec2 center(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }
    ImGui::SetNextWindowSize(ImVec2(640.0f, 420.0f), ImGuiCond_Appearing);

    // Begin the modal popup window.
    if (!ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_NoResize)) {
        return;
    }

    // Close the popup if the Escape key is pressed.
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    // Display the title and a separator.
    ImGui::TextUnformatted("Global Command Palette");
    ImGui::Separator();

    // Input field for filtering commands.
    ImGui::InputTextWithHint("##CommandFilter", "Search commands...", state.filter, sizeof(state.filter));
    const std::vector<const RC_UI::Tools::ToolActionSpec*> filtered = FilterCommandRegistry(state.filter, true);

    // Display the list of filtered commands.
    ImGui::BeginChild("##CommandPaletteList", ImVec2(0.0f, -36.0f), true);
    for (const auto* action : filtered) {
        if (action == nullptr) {
            continue;
        }

        // Check if the command is enabled and display it accordingly.
        const bool enabled = RC_UI::Tools::IsToolActionEnabled(*action);
        if (!enabled) {
            ImGui::BeginDisabled(true);
        }

        const bool selected = ImGui::Selectable(action->label, false);
        if (!enabled) {
            ImGui::EndDisabled();
        }

        // Show a tooltip if the item is hovered.
        if (ImGui::IsItemHovered()) {
            const char* tooltip = action->tooltip != nullptr ? action->tooltip : "";
            ImGui::SetTooltip("%s", tooltip);
        }

        // Execute the selected command and close the popup.
        if (selected) {
            std::string status;
            const bool executed = ExecuteCommand(action->id, dispatch_context, &status);
            (void)executed;
            ImGui::CloseCurrentPopup();
            break;
        }
    }

    // Display global viewport commands that match the filter.
    const std::string_view filter_text(state.filter);
    for (const auto& command : GetGlobalViewportCommands()) {
        const bool matches_filter = ContainsCaseInsensitive(command.label, filter_text) ||
            ContainsCaseInsensitive(command.tooltip != nullptr ? command.tooltip : "", filter_text);
        if (!matches_filter) {
            continue;
        }

        const bool selected = ImGui::Selectable(command.label, false);
        if (ImGui::IsItemHovered() && command.tooltip != nullptr && command.tooltip[0] != '\0') {
            ImGui::SetTooltip("%s", command.tooltip);
        }
        if (selected) {
            std::string status;
            const bool executed = ExecuteGlobalViewportCommand(command.id, dispatch_context, &status);
            (void)executed;
            ImGui::CloseCurrentPopup();
            break;
        }
    }
    ImGui::EndChild();

    // Close button to dismiss the popup.
    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f))) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

} // namespace RC_UI::Commands
