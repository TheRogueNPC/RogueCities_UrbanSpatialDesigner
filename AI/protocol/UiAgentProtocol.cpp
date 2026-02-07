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
        panels.push_back({
            {"id",      p.id},
            {"dock",    p.dock},
            {"visible", p.visible},
        });
    }
    j["layout"]["panels"] = panels;

    json state;
    state["flowRate"]       = snap.state.flowRate;
    state["livePreview"]    = snap.state.livePreview;
    state["debounceSec"]    = snap.state.debounceSec;
    state["seed"]           = snap.state.seed;
    state["activeTool"]     = snap.state.activeTool;
    state["selectedAxioms"] = snap.state.selectedAxioms;
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
