#include "AiAssist.h"
#include "tools/HttpClient.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <iostream>

using json = nlohmann::json;

namespace RogueCity::UI {

// ============================================================================
// BUILD SNAPSHOT FROM CURRENT EDITOR STATE
// ============================================================================

AI::UiSnapshot AiAssist::BuildSnapshot() {
    AI::UiSnapshot s;
    s.app = "RogueCity Visualizer";

    // TODO: Wire to actual RogueNav state
    s.header.left   = "ROGUENAV";
    s.header.mode   = "SOLITON";  // TODO: Read from minimap mode
    s.header.filter = "NORMAL";   // TODO: Read from alert level

    // ImGui docking config
    s.dockingEnabled       = ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable;
    s.multiViewportEnabled = ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable;

    // TODO: Replace with real panel visibility + dock position info
    // For now, hardcode known panels from docs/Intergration Notes
    s.panels.push_back({"Analytics",        "Right",  true});
    s.panels.push_back({"Tools",            "Bottom", true});
    s.panels.push_back({"AxiomBar",         "Top",    true});
    s.panels.push_back({"AxiomLibrary",     "Right",  false});  // Hidden by default
    s.panels.push_back({"RogueVisualizer",  "Center", true});
    s.panels.push_back({"Log",              "Bottom", true});
    s.panels.push_back({"DistrictIndex",    "Bottom", true});
    s.panels.push_back({"RoadIndex",        "Bottom", true});
    s.panels.push_back({"LotIndex",         "Bottom", true});
    s.panels.push_back({"RiverIndex",       "Bottom", true});

    // TODO: Wire from your real state structs
    s.state.flowRate       = 1.0;
    s.state.livePreview    = true;
    s.state.debounceSec    = 0.2;
    s.state.seed           = 123456;
    s.state.activeTool     = "AXIOM_MODE_ACTIVE";
    s.state.selectedAxioms = {/* fill from selection */};

    // TODO: Hook into your logging system; last ~20 lines
    s.logTail = {
        "Generator: Ready",
        "Axiom placement mode active",
        "Viewport: Rendering at 60 FPS",
    };

    return s;
}

// ============================================================================
// APPLY AI COMMANDS TO EDITOR
// ============================================================================

void AiAssist::ApplyCommand(const AI::UiCommand& c, const AI::UiSnapshot& snap) {
    if (c.cmd == "SetHeader") {
        if (c.mode) {
            std::cout << "[AI] SetHeader mode: " << *c.mode << "\n";
            // TODO: set your current mode state from *c.mode
            // e.g., s_minimap_mode = ParseMinimapMode(*c.mode);
        }
        if (c.filter) {
            std::cout << "[AI] SetHeader filter: " << *c.filter << "\n";
            // TODO: set your current filter state from *c.filter
            // e.g., s_nav_alert_level = ParseAlertLevel(*c.filter);
        }
    }
    else if (c.cmd == "RenamePanel") {
        if (c.from && c.to) {
            std::cout << "[AI] RenamePanel: " << *c.from << " -> " << *c.to << "\n";
            // TODO: update your internal panel/window title/ID registry
            // from *c.from to *c.to
        }
    }
    else if (c.cmd == "DockPanel") {
        if (c.panel && c.targetDock) {
            std::cout << "[AI] DockPanel: " << *c.panel << " -> " << *c.targetDock << "\n";
            // TODO: map targetDock ("Left","Right","Bottom","Top","Center")
            // to your dockspace node IDs and move the panel accordingly.
            // Use c.ownDockNode.value_or(false) to decide if it should be its own dock node.
        }
    }
    else if (c.cmd == "SetState") {
        if (!c.key) return;
        const std::string& key = *c.key;

        std::cout << "[AI] SetState: " << key << "\n";

        if (key == "flowRate" && c.valueNumber) {
            std::cout << "[AI]   flowRate = " << *c.valueNumber << "\n";
            // TODO: write into your real flowRate
        }
        else if (key == "livePreview" && c.valueBool) {
            std::cout << "[AI]   livePreview = " << (*c.valueBool ? "true" : "false") << "\n";
            // TODO: write into your livePreview flag
        }
        else if (key == "debounceSec" && c.valueNumber) {
            std::cout << "[AI]   debounceSec = " << *c.valueNumber << "\n";
            // TODO: write into your debounce
        }
        else if (key == "seed" && c.valueNumber) {
            std::cout << "[AI]   seed = " << static_cast<uint64_t>(*c.valueNumber) << "\n";
            // TODO: write into your seed
        }
        // Extend with other keys as needed.
    }
    else if (c.cmd == "Request") {
        std::cout << "[AI] Request for more info: ";
        for (const auto& f : c.requestFields) {
            std::cout << f << " ";
        }
        std::cout << "\n";
        // Optional: surface a message in UI that AI requested more info
    }
    else {
        std::cout << "[AI] Unknown command: " << c.cmd << "\n";
    }
    // Unknown commands: ignore safely.
}

void AiAssist::ApplyCommands(const std::vector<AI::UiCommand>& cmds, 
                            const AI::UiSnapshot& snap) {
    std::cout << "[AI] Applying " << cmds.size() << " commands\n";
    for (const auto& c : cmds) {
        ApplyCommand(c, snap);
    }
}

// ============================================================================
// QUERY AI AGENT
// ============================================================================

std::vector<AI::UiCommand> AiAssist::QueryAgent(const AI::UiSnapshot& snap,
                                                const std::string& goal) {
    const std::string url = "http://127.0.0.1:7077/ui_agent";

    // Build payload
    json payload;
    payload["snapshot"] = json::parse(AI::UiAgentJson::SnapshotToJson(snap));
    payload["goal"] = goal;

    // Call toolserver
    const std::string response = AI::HttpClient::PostJson(url, payload.dump());
    
    // Parse commands
    return AI::UiAgentJson::CommandsFromJson(response);
}

// ============================================================================
// UI CONTROLS
// ============================================================================

void AiAssist::DrawControls(float dt) {
    static char goalBuf[512] = "Make Inspector its own dock and fix viewport border issues.";
    static bool processing = false;
    static std::string lastResult = "";

    ImGui::SeparatorText("AI Assist");
    
    ImGui::InputTextMultiline("AI Goal", goalBuf, sizeof(goalBuf), ImVec2(-1, 80));
    
    if (ImGui::Button("AI Assist Layout") && !processing) {
        processing = true;
        lastResult = "Processing...";
        
        // Build snapshot
        auto snap = BuildSnapshot();
        
        // Query AI
        auto cmds = QueryAgent(snap, std::string(goalBuf));
        
        // Apply commands
        ApplyCommands(cmds, snap);
        
        // Update result
        if (cmds.empty()) {
            lastResult = "AI returned no commands (check HTTP stub implementation)";
        } else {
            lastResult = "Applied " + std::to_string(cmds.size()) + " commands";
        }
        
        processing = false;
    }
    
    if (!lastResult.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", lastResult.c_str());
    }
    
    ImGui::TextWrapped("Note: HTTP client is currently a stub. Implement AI/tools/HttpClient.cpp "
                      "with your preferred HTTP library (cpr, httplib.h, WinHTTP, etc.)");
}

} // namespace RogueCity::UI
