// FILE: rc_panel_workspace.h
// PURPOSE: Always-accessible workspace / persona selector panel.
//          Docks alongside the master panel; lets the user switch between the
//          5 built-in WorkspacePersonas and save/load layout+theme presets.
//          Also hosts the Mockup Live Link — hot-reload CSS tokens from
//          RC_UI_Mockup.html into the running theme and dock ratios.

#pragma once

#include "RogueCity/Visualizer/MockupTokenParser.hpp"

namespace RC_UI::Panels::Workspace {

bool IsOpen();
void Toggle();
void Draw(float dt);
void DrawContent(float dt);

/// Returns the most recently parsed mockup layout ratios (left / right / tool-deck).
/// Valid only when the Mockup Link is enabled and a parse has succeeded.
RogueCity::Visualizer::MockupLayoutTokens GetMockupLayoutTokens();

} // namespace RC_UI::Panels::Workspace
