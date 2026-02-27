// FILE: WorkspaceProfile.hpp
// PURPOSE: Data-only WorkspacePersona + WorkspaceProfile structs and
//          built-in profile factory for the RC-USD persona system.
//
// Lives in visualizer/ (Layer 2) — may call ThemeManager (app/ Layer 2).
// Must NOT be included from core/ or generators/.

#pragma once
#include "RogueCity/App/UI/ThemeManager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace RC_UI {

// ---------------------------------------------------------------------------
// Persona enumeration — drives layout behaviour flags on WorkspaceProfile
// ---------------------------------------------------------------------------
enum class WorkspacePersona {
    Rogue,       // The intended planner — full cockpit aesthetic, all panels
    Enterprise,  // Sterile, minimal, core properties visible, high readability
    Hobbyist,    // Visual first, big buttons, discover by clicking, forgiving
    Planner,     // Spreadsheet-style, indices visible, data-dense, precise
    Artist       // World-fill mode, POI/road anchors, narrative-first layout
};

// ---------------------------------------------------------------------------
// WorkspaceProfile — pure data, no methods, no singletons
// ---------------------------------------------------------------------------
struct WorkspaceProfile {
    std::string      id;           // "rogue" | "enterprise" | "hobbyist" | "planner" | "artist"
    std::string      display_name;
    WorkspacePersona persona;

    // Visual layer
    RogueCity::UI::ThemeProfile theme;

    // Layout behaviour flags
    float info_density    = 0.5f;  // 0.0=minimal labels, 1.0=full telemetry readout
    bool  show_indices    = false; // Show FVA/SIV index columns in panels (Planner mode)
    bool  visual_first    = false; // Prioritize large viewport, reduce panel chrome
    bool  narrative_mode  = false; // Artist — show description fields, POI notes prominently
    bool  show_diagnostics = true; // Urban Hell Diagnostics panel visible by default

    // Default tool configuration
    std::vector<std::string> pinned_tools;        // tool IDs visible in toolbar by default
    std::string              default_entry_state; // HFSM state name to enter on load

    // Data export targets (for self-driving / urban-data headless mode)
    std::vector<std::string> active_export_pipelines;
};

// ---------------------------------------------------------------------------
// Factory — returns all 5 built-in persona profiles
// ---------------------------------------------------------------------------
std::vector<WorkspaceProfile> GetBuiltInWorkspaceProfiles();

// ---------------------------------------------------------------------------
// JSON serialization (for user-saved custom profiles)
// ---------------------------------------------------------------------------
nlohmann::json   WorkspaceProfileToJson(const WorkspaceProfile& profile);
WorkspaceProfile WorkspaceProfileFromJson(const nlohmann::json& j);

} // namespace RC_UI
