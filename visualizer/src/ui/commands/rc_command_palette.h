#pragma once

#include "ui/commands/rc_context_command_registry.h"

namespace RC_UI::Commands {

struct CommandPaletteState {
    bool open_requested{ false };
    char filter[128]{};
};

void RequestOpenCommandPalette(CommandPaletteState& state);
void DrawCommandPalette(
    CommandPaletteState& state,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    const char* popup_id = "ViewportCommandPalette");

} // namespace RC_UI::Commands
