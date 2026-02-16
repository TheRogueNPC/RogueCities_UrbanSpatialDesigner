#pragma once

#include "ui/commands/rc_context_command_registry.h"

#include <imgui.h>

namespace RC_UI::Commands {

struct PieMenuState {
    bool open_requested{ false };
    ImVec2 open_pos{ 0.0f, 0.0f };
};

void RequestOpenPieMenu(PieMenuState& state, const ImVec2& screen_pos);
void DrawPieMenu(
    PieMenuState& state,
    ToolLibrary preferred_library,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id = "ViewportPieCommandMenu");

} // namespace RC_UI::Commands
