#pragma once

#include "ui/commands/rc_command_palette.h"
#include "ui/commands/rc_context_menu_pie.h"
#include "ui/commands/rc_context_menu_smart.h"
#include "ui/rc_ui_input_gate.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <imgui.h>

namespace RC_UI::Viewport {

struct CommandMenuStateBundle {
    RC_UI::Commands::SmartMenuState* smart_menu{ nullptr };
    RC_UI::Commands::PieMenuState* pie_menu{ nullptr };
    RC_UI::Commands::CommandPaletteState* command_palette{ nullptr };
};

struct CommandInteractionParams {
    UiInputGateState input_gate{};
    bool in_viewport{ false };
    bool minimap_hovered{ false };
    ImVec2 mouse_pos{ 0.0f, 0.0f };
    const RogueCity::Core::Editor::EditorConfig* editor_config{ nullptr };
};

void ProcessViewportCommandTriggers(
    const CommandInteractionParams& params,
    const CommandMenuStateBundle& state_bundle);

} // namespace RC_UI::Viewport
