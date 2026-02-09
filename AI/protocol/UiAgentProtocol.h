#pragma once
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace RogueCity::AI {

// ============================================================================
// UI SNAPSHOT TYPES
// ============================================================================
// These types capture the current state of the editor UI for AI analysis

struct UiPanelInfo {
    std::string id{};       // e.g. "Analytics", "Inspector", "RogueVisualizer"
    std::string dock{};     // "Left","Right","Bottom","Top","Center"
    bool visible = true;
    
    // === CODE-SHAPE METADATA (Phase 4: Refactoring) ===
    std::string role{};              // "inspector" | "toolbox" | "viewport" | "nav" | "log"
    std::string owner_module{};      // e.g. "rc_ui_panel_axiom_editor"
    std::vector<std::string> data_bindings{};      // ["axiom.selected_id", "road.brush_radius"]
    std::vector<std::string> interaction_patterns{}; // ["list+detail", "toolbar+canvas"]
};

struct UiHeaderInfo {
    std::string left{};     // e.g. "ROGUENAV"
    std::string mode{};     // e.g. "SOLITON", "REACTIVE", "SATELLITE"
    std::string filter{};   // e.g. "NORMAL", "CAUTION", "EVASION", "ALERT"
};

struct UiStateInfo {
    double flowRate = 1.0;
    bool livePreview = false;
    double debounceSec = 0.0;
    uint64_t seed = 0;
    std::string activeTool{};                // e.g. "AXIOM_MODE_ACTIVE"
    std::vector<std::string> selectedAxioms{};
    
    // === STATE MODEL (Phase 4: Code-aware) ===
    // Key-value pairs representing current application state
    // e.g. {"axiom.selected_id": "A123", "road.brush_radius": "15.0"}
    std::map<std::string, std::string> state_model{};
};

struct UiSnapshot {
    std::string app{};                       // "RogueCity Visualizer"
    UiHeaderInfo header{};
    bool dockingEnabled = true;
    bool multiViewportEnabled = false;
    std::vector<UiPanelInfo> panels{};
    UiStateInfo state{};
    std::vector<std::string> logTail{};      // Last ~20 log lines
};

// ============================================================================
// UI COMMAND TYPE
// ============================================================================
// Commands that the AI can issue to modify the UI

struct UiCommand {
    std::string cmd; // "SetHeader","RenamePanel","DockPanel","SetState","Request"

    // Optional fields used by various commands:
    std::optional<std::string> mode;
    std::optional<std::string> filter;

    std::optional<std::string> from;
    std::optional<std::string> to;

    std::optional<std::string> panel;
    std::optional<std::string> targetDock;
    std::optional<bool> ownDockNode;

    std::optional<std::string> key;        // for SetState
    std::optional<std::string> valueStr;
    std::optional<double> valueNumber;
    std::optional<bool> valueBool;

    std::vector<std::string> requestFields; // for {"cmd":"Request","fields":[...]}
};

// ============================================================================
// JSON SERIALIZATION HELPERS
// ============================================================================
// Convert between C++ types and JSON for AI communication

struct UiAgentJson {
    /// Convert snapshot to JSON string for sending to AI
    static std::string SnapshotToJson(const UiSnapshot& s);
    
    /// Parse JSON response from AI into command list
    static std::vector<UiCommand> CommandsFromJson(const std::string& jsonStr);
};

} // namespace RogueCity::AI
