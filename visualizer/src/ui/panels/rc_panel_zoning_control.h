// FILE: rc_panel_zoning_control.h
// PURPOSE: Control panel for ZoningGenerator parameters
// PHASE 2: Step 7 - User Control
// Y2K GEOMETRY: Glow on hover, pulse on change, state-reactive

#pragma once

#include "RogueCity/App/Integration/ZoningBridge.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <string>

namespace RC_UI::Panels::ZoningControl {

// Draw the zoning control panel (legacy - creates window)
void Draw(float dt);

// Draw panel content only (for Master Panel drawer)
void DrawContent(float dt);
//todo consider splitting this into smaller components if it grows too complex, such as separate sections for lot sizing, building constraints, and budget/population controls, each with their own state and animations to keep the code organized and maintainable as we add more features in future phases.
// Panel state (persists between frames)
struct PanelState {
    // UI parameters
    RogueCity::App::Integration::ZoningBridge::UiConfig config;
    RogueCity::App::Integration::ZoningBridge bridge;
    
    // UI state
    bool parameters_changed = false;
    float glow_intensity = 0.0f;
    float pulse_phase = 0.0f;
    bool is_generating = false;
    std::string last_error;
    
    // Preview
    bool show_preview = false;
};

// Get panel state (singleton)
PanelState& GetPanelState();

} // namespace RC_UI::Panels::ZoningControl
