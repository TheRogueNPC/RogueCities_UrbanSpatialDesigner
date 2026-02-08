// FILE: rc_panel_zoning_control.h
// PURPOSE: Control panel for ZoningGenerator parameters
// PHASE 2: Step 7 - User Control
// Y2K GEOMETRY: Glow on hover, pulse on change, state-reactive

#pragma once

#include "RogueCity/App/Integration/ZoningBridge.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RC_UI::Panels::ZoningControl {

// Draw the zoning control panel
void Draw(float dt);

// Panel state (persists between frames)
struct PanelState {
    // UI parameters
    RogueCity::App::Integration::ZoningBridge::UiConfig config;
    
    // UI state
    bool parameters_changed = false;
    float glow_intensity = 0.0f;
    float pulse_phase = 0.0f;
    bool is_generating = false;
    
    // Preview
    bool show_preview = false;
};

// Get panel state (singleton)
PanelState& GetPanelState();

} // namespace RC_UI::Panels::ZoningControl
