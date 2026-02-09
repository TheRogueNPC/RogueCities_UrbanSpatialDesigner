// FILE: rc_ui_root.h
// PURPOSE: Root-level entry points for the procedural editor UI.

#pragma once

#include <imgui.h>

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

// Queue a dock reassignment request for an existing panel/window.
bool QueueDockWindow(const char* windowName, const char* dockArea, bool ownDockNode = false);

// Reset dock layout to default (useful if user messes up docking)
void ResetDockLayout();

// Track last docked area for windows (for re-docking on close).
void NotifyDockedWindow(const char* windowName, const char* dockArea);
void ReturnWindowToLastDock(const char* windowName, const char* fallbackArea);

struct DockableWindowState {
    bool open = true;
    bool was_docked = true;
};

// Begin a dockable window with unified behavior: X only when undocked, re-dock on close.
bool BeginDockableWindow(const char* windowName,
                         DockableWindowState& state,
                         const char* fallbackDockArea,
                         ImGuiWindowFlags flags = 0);

void EndDockableWindow();

} // namespace RC_UI
