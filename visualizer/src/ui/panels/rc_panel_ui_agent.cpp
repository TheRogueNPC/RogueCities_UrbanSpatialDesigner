// FILE: rc_panel_ui_agent.cpp
// PURPOSE: UI Agent Assistant for AI-driven layout optimization AND design/refactor planning

#include "rc_panel_ui_agent.h"
#include "client/UiAgentClient.h"
#include "client/UiDesignAssistant.h"
#include "runtime/AiBridgeRuntime.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace RogueCity::UI {

// Helper to build snapshot with code-shape metadata
static AI::UiSnapshot BuildEnhancedSnapshot() {
    AI::UiSnapshot snapshot;
    snapshot.app = "RogueCity Visualizer";
    snapshot.header.left = "ROGUENAV";
    snapshot.header.mode = "SOLITON";  // TODO: Get from actual state
    snapshot.header.filter = "NORMAL";
    
    // Add panels with code-shape metadata (Phase 4)
    snapshot.panels.push_back({
        "Analytics", "Right", true,
        "inspector", "rc_panel_analytics",
        {"analytics.data_source", "analytics.time_range"},
        {"list+detail"}
    });
    
    snapshot.panels.push_back({
        "Tools", "Bottom", true,
        "toolbox", "rc_panel_tools",
        {"tool.active", "tool.settings.*"},
        {"toolbar+canvas"}
    });
    
    snapshot.panels.push_back({
        "RogueVisualizer", "Center", true,
        "viewport", "rc_ui_rogue_visualizer",
        {"camera.*", "view_mode", "selection"},
        {"canvas+overlay"}
    });
    
    snapshot.panels.push_back({
        "road_index", "Bottom", true,
        "index", "rc_panel_road_index",
        {"roads[]", "roads.selected_id"},
        {"table+selection"}
    });
    
    snapshot.panels.push_back({
        "district_index", "Bottom", true,
        "index", "rc_panel_district_index",
        {"districts[]", "districts.selected_id"},
        {"table+selection"}
    });
    
    snapshot.panels.push_back({
        "lot_index", "Bottom", true,
        "index", "rc_panel_lot_index",
        {"lots[]", "lots.selected_id"},
        {"table+selection"}
    });
    
    snapshot.state.flowRate = 1.0;
    snapshot.state.livePreview = true;
    snapshot.state.debounceSec = 0.2;
    snapshot.state.seed = 12345;
    snapshot.state.activeTool = "AXIOM_MODE";
    
    // Add state model (Phase 4)
    snapshot.state.state_model["axiom.selected_id"] = "A123";
    snapshot.state.state_model["road.brush_radius"] = "15.0";
    
    return snapshot;
}

void UiAgentPanel::Render() {
    if (!ImGui::Begin("UI Agent Assistant", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    
    // === MODE 1: LAYOUT COMMANDS ===
    DesignSystem::SectionHeader("Layout Optimization");
    
    ImGui::Text("Ask AI to adjust the UI layout:");
    ImGui::InputTextMultiline("##goal", m_goalBuffer, sizeof(m_goalBuffer), ImVec2(-1, 60));
    
    ImGui::BeginDisabled(m_processing);
    if (DesignSystem::ButtonPrimary("Apply AI Layout", ImVec2(180, 30))) {
        m_processing = true;
        m_lastResult = "Processing...";
        
        auto snapshot = BuildEnhancedSnapshot();
        auto commands = AI::UiAgentClient::QueryAgent(snapshot, std::string(m_goalBuffer));
        
        if (commands.empty()) {
            m_lastResult = "No commands returned (check console for errors)";
        } else {
            m_lastResult = "Received " + std::to_string(commands.size()) + " commands:\n";
            for (const auto& cmd : commands) {
                m_lastResult += "- " + std::string(cmd.cmd) + "\n";
                // TODO: Actually apply commands to the UI
            }
        }
        
        m_processing = false;
    }
    ImGui::EndDisabled();
    
    if (!m_lastResult.empty()) {
        ImGui::TextWrapped("%s", m_lastResult.c_str());
    }
    
    DesignSystem::Separator();
    
    // === MODE 2: DESIGN/REFACTOR PLANNING ===
    DesignSystem::SectionHeader("Design & Refactoring");
    
    ImGui::Text("Ask AI to analyze UI structure and suggest refactorings:");
    ImGui::InputTextMultiline("##design_goal", m_designGoalBuffer, sizeof(m_designGoalBuffer), ImVec2(-1, 60));
    
    ImGui::BeginDisabled(m_designProcessing);
    if (DesignSystem::ButtonSecondary("Generate Refactor Plan", ImVec2(180, 30))) {
        m_designProcessing = true;
        m_lastDesignResult = "Generating plan...";
        
        auto snapshot = BuildEnhancedSnapshot();
        auto plan = AI::UiDesignAssistant::GenerateDesignPlan(snapshot, std::string(m_designGoalBuffer));
        
        // Generate timestamped filename
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string filename = "AI/docs/ui/ui_refactor_" + ss.str() + ".json";
        
        if (AI::UiDesignAssistant::SaveDesignPlan(plan, filename)) {
            m_lastDesignResult = "Saved plan to: " + filename + "\n\n";
            m_lastDesignResult += "Patterns: " + std::to_string(plan.component_patterns.size()) + "\n";
            m_lastDesignResult += "Refactoring opportunities: " + std::to_string(plan.refactoring_opportunities.size()) + "\n\n";
            
            if (!plan.summary.empty()) {
                m_lastDesignResult += "Summary:\n" + plan.summary;
            }
        } else {
            m_lastDesignResult = "Failed to save plan (check console)";
        }
        
        m_designProcessing = false;
    }
    ImGui::EndDisabled();
    
    if (!m_lastDesignResult.empty()) {
        ImGui::TextWrapped("%s", m_lastDesignResult.c_str());
    }
    
    ImGui::End();
}

} // namespace RogueCity::UI
