#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace RogueCity::UIInt {

// ============================================================================
// Introspection Types (ui-introspect-v1)
// ============================================================================

struct UiWidgetInfo {
    std::string type;   // "button" | "checkbox" | "slider" | "table" | "tree" | "property_editor" | ...
    std::string label;  // user-facing label
    std::string binding; // optional semantic binding key (e.g. "axiom.metrics.complexity" or "action:axiom.create")
    std::vector<std::string> tags;
};

struct UiPanelInfo {
    std::string id;          // stable ID (prefer ImGui window name)
    std::string title;       // user-facing title (may match id)
    std::string role;        // "inspector" | "toolbox" | "viewport" | "nav" | "log" | "settings" | "index"
    std::string dock_area;   // "Left" | "Right" | "Center" | "Bottom" | "Top" | "Floating" | "Unknown"
    bool visible = true;
    std::string owner_module; // source file / module hint
    std::vector<std::string> tags;
    std::vector<UiWidgetInfo> widgets;
};

struct UiActionInfo {
    std::string id;       // e.g. "axiom.create"
    std::string label;    // e.g. "Create Axiom"
    std::string panel_id; // panel where exposed
    std::vector<std::string> context_tags;
    std::string handler_symbol; // optional code hint
};

struct DockTreeNode {
    std::string id;                 // node id (for groups)
    std::string orientation;        // "horizontal" | "vertical" | ""
    std::string panel_id;           // leaf panel id (if leaf)
    std::vector<DockTreeNode> children;
};

struct UiIntrospectionSnapshot {
    std::string app_name = "RogueCity Visualizer";
    std::string protocol_version = "ui-introspect-v1";
    std::string active_mode; // "AXIOM" | "ROAD" | "DISTRICT" | ...
    std::vector<std::string> context_tags;
    std::vector<UiPanelInfo> panels;
    std::vector<UiActionInfo> actions;
    DockTreeNode dock_tree;
    std::vector<nlohmann::json> patterns_detected;
};

// ============================================================================
// JSON Serialization
// ============================================================================

void to_json(nlohmann::json& j, const UiWidgetInfo& w);
void from_json(const nlohmann::json& j, UiWidgetInfo& w);

void to_json(nlohmann::json& j, const UiPanelInfo& p);
void from_json(const nlohmann::json& j, UiPanelInfo& p);

void to_json(nlohmann::json& j, const UiActionInfo& a);
void from_json(const nlohmann::json& j, UiActionInfo& a);

void to_json(nlohmann::json& j, const DockTreeNode& n);
void from_json(const nlohmann::json& j, DockTreeNode& n);

void to_json(nlohmann::json& j, const UiIntrospectionSnapshot& s);
void from_json(const nlohmann::json& j, UiIntrospectionSnapshot& s);

// ============================================================================
// Collector API
// ============================================================================

struct PanelMeta {
    std::string id;
    std::string title;
    std::string role;
    std::string dock_area;
    std::string owner_module;
    std::vector<std::string> tags;
};

class UiIntrospector {
public:
    static UiIntrospector& Instance();

    void BeginFrame(const std::string& activeMode, bool devShellActive);
    void SetDockTree(const DockTreeNode& dockTree);

    void BeginPanel(const PanelMeta& meta, bool visible);
    void RegisterWidget(const UiWidgetInfo& w);
    void RegisterAction(const UiActionInfo& a);
    void EndPanel();

    const UiIntrospectionSnapshot& Snapshot() const { return m_snapshot; }
    nlohmann::json SnapshotJson() const;
    bool SaveSnapshotJson(const std::string& filename, std::string* outError = nullptr) const;

private:
    UiIntrospector() = default;

    UiIntrospectionSnapshot m_snapshot;
    std::vector<int> m_panelStack;
};

} // namespace RogueCity::UIInt
