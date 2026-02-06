// FILE: rc_panel_axiom_editor.h
#pragma once

namespace RC_UI::Panels::AxiomEditor {

/// Initialize axiom editor resources (viewports, tools)
void Initialize();

/// Cleanup axiom editor resources
void Shutdown();

/// Render axiom editor panel with integrated viewport and tool
void Draw(float dt);

} // namespace RC_UI::Panels::AxiomEditor
