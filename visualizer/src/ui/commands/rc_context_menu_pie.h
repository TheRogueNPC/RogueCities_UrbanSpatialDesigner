#pragma once

#include "ui/commands/rc_context_command_registry.h"

#include <imgui.h>
#include <optional>

namespace RC_UI::Commands {

struct PieMenuState {
    bool open_requested{ false };
    ImVec2 open_pos{ 0.0f, 0.0f };
    bool has_preferred_library{ false };
    ToolLibrary preferred_library{ ToolLibrary::Axiom };
};

void RequestOpenPieMenu(
    PieMenuState& state,
    const ImVec2& screen_pos,
    std::optional<ToolLibrary> preferred_library = std::nullopt);
void DrawPieMenu(
    PieMenuState& state,
    ToolLibrary preferred_library,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id = "ViewportPieCommandMenu");

} // namespace RC_UI::Commands
