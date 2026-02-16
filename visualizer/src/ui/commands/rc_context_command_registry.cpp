#include "ui/commands/rc_context_command_registry.h"

#include "ui/panels/rc_panel_axiom_editor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace RC_UI::Commands {
namespace {

constexpr std::array<GlobalViewportCommandSpec, 3> kGlobalViewportCommands{{
    {GlobalViewportCommandId::ToggleMinimap, "Toggle Minimap", "Show or hide the RogueNav minimap overlay."},
    {GlobalViewportCommandId::ForceGenerate, "Force Generate", "Run immediate city regeneration from current axioms."},
    {GlobalViewportCommandId::ResetDockLayout, "Reset Dock Layout", "Rebuild the dock layout to the default contract."},
}};

std::string ToLowerCopy(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (char c : value) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lowered;
}

bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }

    const std::string lower_haystack = ToLowerCopy(haystack);
    const std::string lower_needle = ToLowerCopy(needle);
    return lower_haystack.find(lower_needle) != std::string::npos;
}

} // namespace

std::span<const RC_UI::Tools::ToolActionSpec> GetCommandRegistry() {
    return RC_UI::Tools::GetToolActionCatalog();
}

std::span<const GlobalViewportCommandSpec> GetGlobalViewportCommands() {
    return std::span<const GlobalViewportCommandSpec>(kGlobalViewportCommands.data(), kGlobalViewportCommands.size());
}

std::vector<const RC_UI::Tools::ToolActionSpec*> FilterCommandRegistry(
    std::string_view filter,
    bool include_disabled) {
    std::vector<const RC_UI::Tools::ToolActionSpec*> filtered;
    const auto registry = GetCommandRegistry();
    filtered.reserve(registry.size());

    for (const auto& action : registry) {
        if (!include_disabled && !RC_UI::Tools::IsToolActionEnabled(action)) {
            continue;
        }

        if (!filter.empty()) {
            const bool matches = ContainsCaseInsensitive(action.label, filter) ||
                ContainsCaseInsensitive(action.tooltip, filter) ||
                ContainsCaseInsensitive(RC_UI::Tools::ToolActionName(action.id), filter);
            if (!matches) {
                continue;
            }
        }

        filtered.push_back(&action);
    }

    return filtered;
}

bool ExecuteCommand(
    RC_UI::Tools::ToolActionId action_id,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    std::string* out_status) {
    const auto result = RC_UI::Tools::DispatchToolAction(action_id, dispatch_context, out_status);
    return result == RC_UI::Tools::DispatchResult::Handled;
}

bool ExecuteGlobalViewportCommand(
    GlobalViewportCommandId command_id,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    std::string* out_status) {
    (void)dispatch_context;
    switch (command_id) {
        case GlobalViewportCommandId::ToggleMinimap:
            RC_UI::Panels::AxiomEditor::ToggleMinimapVisible();
            if (out_status != nullptr) {
                *out_status = RC_UI::Panels::AxiomEditor::IsMinimapVisible() ? "minimap-visible" : "minimap-hidden";
            }
            return true;
        case GlobalViewportCommandId::ForceGenerate:
            if (!RC_UI::Panels::AxiomEditor::CanGenerate()) {
                if (out_status != nullptr) {
                    *out_status = "force-generate-not-ready";
                }
                return false;
            }
            RC_UI::Panels::AxiomEditor::ForceGenerate();
            if (out_status != nullptr) {
                *out_status = "force-generate-triggered";
            }
            return true;
        case GlobalViewportCommandId::ResetDockLayout:
            RC_UI::ResetDockLayout();
            if (out_status != nullptr) {
                *out_status = "dock-layout-reset";
            }
            return true;
        default:
            if (out_status != nullptr) {
                *out_status = "unknown-global-command";
            }
            return false;
    }
}

ToolLibrary CommandLibraryForDomain(RogueCity::Core::Editor::ToolDomain domain) {
    using Domain = RogueCity::Core::Editor::ToolDomain;
    switch (domain) {
        case Domain::Axiom:
            return ToolLibrary::Axiom;
        case Domain::Water:
        case Domain::Flow:
            return ToolLibrary::Water;
        case Domain::Road:
        case Domain::Paths:
            return ToolLibrary::Road;
        case Domain::District:
        case Domain::Zone:
            return ToolLibrary::District;
        case Domain::Lot:
            return ToolLibrary::Lot;
        case Domain::Building:
        case Domain::FloorPlan:
        case Domain::Furnature:
        default:
            return ToolLibrary::Building;
    }
}

const char* CommandLibraryName(ToolLibrary library) {
    switch (library) {
        case ToolLibrary::Axiom:
            return "Axiom";
        case ToolLibrary::Water:
            return "Water";
        case ToolLibrary::Road:
            return "Road";
        case ToolLibrary::District:
            return "District / Zone";
        case ToolLibrary::Lot:
            return "Lot";
        case ToolLibrary::Building:
        default:
            return "Building";
    }
}

} // namespace RC_UI::Commands
