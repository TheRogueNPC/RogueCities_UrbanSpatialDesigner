#include "UiAgentProtocol.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace RogueCity::AI {

// ============================================================================
// SNAPSHOT TO JSON
// ============================================================================

static json ToJson(const UiSnapshot& snap) {
    json j;
    j["app"] = snap.app;
    
    j["header"] = {
        {"left",   snap.header.left},
        {"mode",   snap.header.mode},
        {"filter", snap.header.filter},
    };
    
    j["layout"] = {
        {"dockingEnabled",       snap.dockingEnabled},
        {"multiViewportEnabled", snap.multiViewportEnabled},
    };
    
    json panels = json::array();
    for (const auto& p : snap.panels) {
        json panel;
        panel["id"] = p.id;
        panel["dock"] = p.dock;
        panel["visible"] = p.visible;
        
        // Code-shape metadata (Phase 4)
        if (!p.role.empty()) panel["role"] = p.role;
        if (!p.owner_module.empty()) panel["owner_module"] = p.owner_module;
        if (!p.data_bindings.empty()) panel["data_bindings"] = p.data_bindings;
        if (!p.interaction_patterns.empty()) panel["interaction_patterns"] = p.interaction_patterns;
        
        panels.push_back(panel);
    }
    j["layout"]["panels"] = panels;

    json state;
    state["flowRate"]       = snap.state.flowRate;
    state["livePreview"]    = snap.state.livePreview;
    state["debounceSec"]    = snap.state.debounceSec;
    state["seed"]           = snap.state.seed;
    state["activeTool"]     = snap.state.activeTool;
    state["selectedAxioms"] = snap.state.selectedAxioms;
    
    // State model (Phase 4)
    if (!snap.state.state_model.empty()) {
        state["state_model"] = snap.state.state_model;
    }
    
    j["state"] = state;

    j["logTail"] = snap.logTail;
    
    return j;
}

// ============================================================================
// JSON TO COMMANDS
// ============================================================================

static std::vector<UiCommand> UiCommandsFromJsonImpl(const json& root) {
    std::vector<UiCommand> out;
    
    if (!root.is_array()) return out;
    
    for (const auto& item : root) {
        UiCommand c;
        
        if (!item.contains("cmd")) continue;
        c.cmd = item["cmd"].get<std::string>();

        // Helper lambdas for optional field extraction
        auto setOptStr = [&](const char* key, std::optional<std::string>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<std::string>();
        };
        
        auto setOptBool = [&](const char* key, std::optional<bool>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<bool>();
        };
        
        auto setOptDouble = [&](const char* key, std::optional<double>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<double>();
        };

        // Extract optional command parameters
        setOptStr("mode",        c.mode);
        setOptStr("filter",      c.filter);
        setOptStr("from",        c.from);
        setOptStr("to",          c.to);
        setOptStr("panel",       c.panel);
        setOptStr("targetDock",  c.targetDock);
        setOptBool("ownDockNode", c.ownDockNode);
        setOptStr("key",         c.key);
        setOptStr("valueStr",    c.valueStr);
        setOptDouble("valueNumber", c.valueNumber);
        setOptBool("valueBool",  c.valueBool);

        // Extract request fields array
        if (item.contains("fields") && item["fields"].is_array()) {
            for (auto& f : item["fields"])
                c.requestFields.push_back(f.get<std::string>());
        }

        out.push_back(std::move(c));
    }
    
    return out;
}

// ============================================================================
// PUBLIC API
// ============================================================================

std::string UiAgentJson::SnapshotToJson(const UiSnapshot& s) {
    return ToJson(s).dump();
}

std::vector<UiCommand> UiAgentJson::CommandsFromJson(const std::string& jsonStr) {
    try {
        json root = json::parse(jsonStr);
        return UiCommandsFromJsonImpl(root);
    } catch (...) {
        // Return empty list if JSON parsing fails
        // Caller should handle empty response gracefully
        return {};
    }
}

} // namespace RogueCity::AI
