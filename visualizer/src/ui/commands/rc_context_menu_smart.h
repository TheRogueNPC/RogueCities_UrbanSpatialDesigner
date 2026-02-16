#pragma once

#include "ui/commands/rc_context_command_registry.h"

#include <imgui.h>

namespace RC_UI::Commands {

struct SmartMenuState {
    bool open_requested{ false };
    ImVec2 open_pos{ 0.0f, 0.0f };
};

void RequestOpenSmartMenu(SmartMenuState& state, const ImVec2& screen_pos);
void DrawSmartMenu(
    SmartMenuState& state,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id = "ViewportSmartCommandMenu");

} // namespace RC_UI::Commands
