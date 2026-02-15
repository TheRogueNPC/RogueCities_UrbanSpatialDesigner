// FILE: rc_ui_root.h
// PURPOSE: Root-level entry points for the procedural editor UI.
// todo phase 2 - refactor to dynamic panel management system with factory registration for better modularity and decoupling. This file should serve as the main entry point for rendering the UI each frame, managing the dockspace, and providing shared utilities for panel management and workspace presets, while individual panels and components should be defined in their own headers and source files to maintain separation of concerns and avoid ODR violations.

#pragma once

#include <array>
#include <string>
#include <vector>
#include <imgui.h>
#include "ui/rc_ui_input_gate.h"

namespace RogueCity::App { class MinimapViewport; }

namespace RC_UI {

// Render the full UI frame, including the dockspace and all panels. Pass in delta time for animations.
void DrawRoot(float dt);

// Apply the custom UI theme (colors, rounding). Should be called once before drawing begins.
void ApplyTheme();

enum class ToolLibrary {
    Axiom,
    Water,
    Road,
    District,
    Lot,
    Building
};

inline constexpr std::array<ToolLibrary, 6> kToolLibraryOrder = {
    ToolLibrary::Axiom,
    ToolLibrary::Water,
    ToolLibrary::Road,
    ToolLibrary::District,
    ToolLibrary::Lot,
    ToolLibrary::Building
};

[[nodiscard]] bool IsToolLibraryOpen(ToolLibrary tool);
void ToggleToolLibrary(ToolLibrary tool);
void ActivateToolLibrary(ToolLibrary tool);
void PopoutToolLibrary(ToolLibrary tool);
[[nodiscard]] bool IsToolLibraryPopoutOpen(ToolLibrary tool);

void ApplyUnifiedWindowSchema(const ImVec2& baseSize = ImVec2(540.0f, 720.0f), float padding = 16.0f);
void PopUnifiedWindowSchema();
void BeginUnifiedTextWrap(float padding = 16.0f);
void EndUnifiedTextWrap();
bool BeginWindowContainer(const char* id = "##window_container", ImGuiWindowFlags flags = 0);
void EndWindowContainer();

struct DockLayoutPreferences {
    float left_panel_ratio = 0.32f;
    float right_panel_ratio = 0.22f;
    float tool_deck_ratio = 0.24f;
};

[[nodiscard]] DockLayoutPreferences GetDockLayoutPreferences();
[[nodiscard]] DockLayoutPreferences GetDefaultDockLayoutPreferences();
void SetDockLayoutPreferences(const DockLayoutPreferences& preferences);

// Axiom Deck -> Axiom Library toggle.
[[nodiscard]] bool IsAxiomLibraryOpen();
void ToggleAxiomLibrary();

// Get minimap viewport for camera sync (Phase 5: Polish)
RogueCity::App::MinimapViewport* GetMinimapViewport();

// Queue a dock reassignment request for an existing panel/window.
bool QueueDockWindow(const char* windowName, const char* dockArea, bool ownDockNode = false);

// Reset dock layout to default (useful if user messes up docking)
void ResetDockLayout();

// Workspace preset persistence (layout + docking state across runs).
bool SaveWorkspacePreset(const char* presetName, std::string* error = nullptr);
bool LoadWorkspacePreset(const char* presetName, std::string* error = nullptr);
std::vector<std::string> ListWorkspacePresets();

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

// Publish/inspect per-frame UI input arbitration for viewport actions.
void PublishUiInputGateState(const UiInputGateState& state);
[[nodiscard]] const UiInputGateState& GetUiInputGateState();

} // namespace RC_UI
