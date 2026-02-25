// FILE: visualizer/src/ui/panels/rc_panel_validation.h
// PURPOSE: Declaration for live validation panel.

#pragma once

namespace RC_UI::Panels::Validation {

// Draw the validation panel. Supply delta time to update reactive animations.
void Draw(float dt);
void DrawContent(float dt);

} // namespace RC_UI::Panels::Validation
