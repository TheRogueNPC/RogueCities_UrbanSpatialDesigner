// FILE: rc_ui_root.h
// PURPOSE: Root-level entry points for the procedural editor UI.

#pragma once

namespace RogueCity::App { class MinimapViewport; }

namespace RC_UI {

// Render the full UI frame, including the dockspace and all panels. Pass in delta time for animations.
void DrawRoot(float dt);

// Apply the custom UI theme (colors, rounding). Should be called once before drawing begins.
void ApplyTheme();

// Axiom Deck -> Axiom Library toggle.
[[nodiscard]] bool IsAxiomLibraryOpen();
void ToggleAxiomLibrary();

// Get minimap viewport for camera sync (Phase 5: Polish)
RogueCity::App::MinimapViewport* GetMinimapViewport();

} // namespace RC_UI
