#include "AiAssist.h"
#include "tools/HttpClient.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>

using json = nlohmann::json;

namespace RogueCity::UI {

namespace {

struct PanelRuntimeEntry {
    std::string windowName;
    std::string dockArea;
    bool visible = true;
    std::string role;
    std::string ownerModule;
};

struct RuntimeUiState {
    std::string filter = "NORMAL";
    double flowRate = 1.0;
    bool livePreview = true;
    double debounceSec = 0.2;
    uint64_t seed = 123456;
    std::map<std::string, PanelRuntimeEntry> panels{
        {"Analytics", {"Analytics", "Right", true, "inspector", "rc_panel_telemetry"}},
        {"Tools", {"Tools", "Bottom", true, "toolbox", "rc_panel_tools"}},
        {"Axiom Bar", {"Axiom Bar", "Top", true, "toolbox", "rc_panel_axiom_bar"}},
        {"Axiom Library", {"Axiom Library", "Right", false, "toolbox", "rc_panel_axiom_editor"}},
        {"RogueVisualizer", {"RogueVisualizer", "Center", true, "viewport", "rc_panel_axiom_editor"}},
        {"Log", {"Log", "Bottom", true, "log", "rc_panel_log"}},
        {"District Index", {"District Index", "Bottom", true, "index", "rc_panel_district_index"}},
        {"Road Index", {"Road Index", "Bottom", true, "index", "rc_panel_road_index"}},
        {"Lot Index", {"Lot Index", "Bottom", true, "index", "rc_panel_lot_index"}},
        {"River Index", {"River Index", "Bottom", true, "index", "rc_panel_river_index"}}
    };
};

RuntimeUiState& GetRuntimeUiState() {
    static RuntimeUiState state;
    return state;
}

std::string ToUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

std::string ActiveModeFromHFSM() {
    using RogueCity::Core::Editor::EditorState;
    const auto state = RogueCity::Core::Editor::GetEditorHFSM().state();
    switch (state) {
        case EditorState::Editing_Axioms:
        case EditorState::Viewport_PlaceAxiom: return "AXIOM";
        case EditorState::Editing_Roads:
        case EditorState::Viewport_DrawRoad: return "ROAD";
        case EditorState::Editing_Districts: return "DISTRICT";
        case EditorState::Editing_Lots: return "LOT";
        case EditorState::Editing_Buildings: return "BUILDING";
        case EditorState::Simulating: return "SIM";
        default: return "IDLE";
    }
}

} // namespace

// ============================================================================
// BUILD SNAPSHOT FROM CURRENT EDITOR STATE
// ============================================================================

AI::UiSnapshot AiAssist::BuildSnapshot() {
    AI::UiSnapshot s;
    s.app = "RogueCity Visualizer";
    auto& runtime = GetRuntimeUiState();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    s.header.left   = "ROGUENAV";
    s.header.mode   = ActiveModeFromHFSM();
    s.header.filter = runtime.filter;

    // ImGui docking config
    const ImGuiIO& io = ImGui::GetIO();
    s.dockingEnabled       = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
    s.multiViewportEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;

    for (const auto& [panelId, panel] : runtime.panels) {
        AI::UiPanelInfo info;
        info.id = panel.windowName;
        info.dock = panel.dockArea;
        info.visible = panel.visible;
        info.role = panel.role;
        info.owner_module = panel.ownerModule;
        s.panels.push_back(std::move(info));
    }

    s.state.flowRate = runtime.flowRate;
    s.state.livePreview = runtime.livePreview;
    s.state.debounceSec = runtime.debounceSec;
    s.state.seed = runtime.seed;
    s.state.activeTool = s.header.mode;
    s.state.state_model["frame_counter"] = std::to_string(gs.frame_counter);
    s.state.state_model["roads.count"] = std::to_string(gs.roads.size());
    s.state.state_model["districts.count"] = std::to_string(gs.districts.size());
    s.state.state_model["lots.count"] = std::to_string(gs.lots.size());

    if (gs.selection.selected_road) {
        s.state.state_model["roads.selected"] = std::to_string(gs.selection.selected_road->id);
    }
    if (gs.selection.selected_district) {
        s.state.state_model["district.selected"] = std::to_string(gs.selection.selected_district->id);
    }
    if (gs.selection.selected_lot) {
        s.state.state_model["lot.selected"] = std::to_string(gs.selection.selected_lot->id);
    }

    s.logTail = {
        "Mode: " + s.header.mode,
        "Filter: " + s.header.filter,
        "Roads: " + std::to_string(gs.roads.size()),
        "Districts: " + std::to_string(gs.districts.size()),
        "Lots: " + std::to_string(gs.lots.size())
    };

    return s;
}

// ============================================================================
// APPLY AI COMMANDS TO EDITOR
// ============================================================================

void AiAssist::ApplyCommand(const AI::UiCommand& c, const AI::UiSnapshot& snap) {
    (void)snap;
    auto& runtime = GetRuntimeUiState();
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    if (c.cmd == "SetHeader") {
        if (c.mode) {
            std::cout << "[AI] SetHeader mode: " << *c.mode << "\n";
            const std::string mode = ToUpperCopy(*c.mode);
            if (mode == "AXIOM") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::Tool_Axioms, gs);
            } else if (mode == "ROAD") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::Tool_Roads, gs);
            } else if (mode == "DISTRICT") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::Tool_Districts, gs);
            } else if (mode == "LOT") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::Tool_Lots, gs);
            } else if (mode == "BUILDING") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::Tool_Buildings, gs);
            } else if (mode == "IDLE") {
                hfsm.handle_event(RogueCity::Core::Editor::EditorEvent::GotoIdle, gs);
            }
        }
        if (c.filter) {
            std::cout << "[AI] SetHeader filter: " << *c.filter << "\n";
            runtime.filter = ToUpperCopy(*c.filter);
        }
    }
    else if (c.cmd == "RenamePanel") {
        if (c.from && c.to) {
            std::cout << "[AI] RenamePanel: " << *c.from << " -> " << *c.to << "\n";
            auto it = runtime.panels.find(*c.from);
            if (it != runtime.panels.end()) {
                PanelRuntimeEntry entry = it->second;
                entry.windowName = *c.to;
                runtime.panels.erase(it);
                runtime.panels[*c.to] = std::move(entry);
            }
        }
    }
    else if (c.cmd == "DockPanel") {
        if (c.panel && c.targetDock) {
            std::cout << "[AI] DockPanel: " << *c.panel << " -> " << *c.targetDock << "\n";
            auto it = runtime.panels.find(*c.panel);
            if (it != runtime.panels.end()) {
                it->second.dockArea = *c.targetDock;
            }
        }
    }
    else if (c.cmd == "SetState") {
        if (!c.key) return;
        const std::string& key = *c.key;

        std::cout << "[AI] SetState: " << key << "\n";

        if (key == "flowRate" && c.valueNumber) {
            std::cout << "[AI]   flowRate = " << *c.valueNumber << "\n";
            runtime.flowRate = *c.valueNumber;
        }
        else if (key == "livePreview" && c.valueBool) {
            std::cout << "[AI]   livePreview = " << (*c.valueBool ? "true" : "false") << "\n";
            runtime.livePreview = *c.valueBool;
        }
        else if (key == "debounceSec" && c.valueNumber) {
            std::cout << "[AI]   debounceSec = " << *c.valueNumber << "\n";
            runtime.debounceSec = *c.valueNumber;
        }
        else if (key == "seed" && c.valueNumber) {
            std::cout << "[AI]   seed = " << static_cast<uint64_t>(*c.valueNumber) << "\n";
            runtime.seed = static_cast<uint64_t>(*c.valueNumber);
            gs.params.seed = static_cast<uint32_t>(runtime.seed);
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

    auto snapshot = BuildSnapshot();
    std::ostringstream oss;
    oss << "Mode: " << snapshot.header.mode
        << " | Filter: " << snapshot.header.filter
        << " | Panels: " << snapshot.panels.size();
    ImGui::TextWrapped("%s", oss.str().c_str());
}

} // namespace RogueCity::UI
