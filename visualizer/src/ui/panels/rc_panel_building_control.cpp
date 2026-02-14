// FILE: rc_panel_building_control.cpp
// PURPOSE: Control panel for building placement with state-reactive rendering
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_BUILDING_CONTROL_PANEL
// AGENT: UI/UX_Master + Coder_Agent
// CATEGORY: Control_Panel

#include "ui/panels/rc_panel_building_control.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/App/Integration/ZoningBridge.hpp>

#include <imgui.h>

namespace RC_UI::Panels::BuildingControl {

// Building placement parameters (persistent UI state)
static RogueCity::App::Integration::ZoningBridge::UiConfig s_building_params;
static bool s_is_generating = false;
static float s_gen_start_time = 0.0f;
static RogueCity::App::Integration::ZoningBridge s_zoning_bridge;

void DrawContent(float dt)
{
    using namespace RogueCity::Core::Editor;
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    // === Building Parameters ===
    ImGui::SeparatorText("Building Parameters");
    
    ImGui::SliderFloat("Min Coverage", &s_building_params.min_building_coverage, 0.2f, 0.6f, "%.1f%%");
    uiint.RegisterWidget({"slider", "Min Coverage", "building.min_coverage", {"building", "sizing"}});
    
    ImGui::SliderFloat("Max Coverage", &s_building_params.max_building_coverage, 0.5f, 0.9f, "%.1f%%");
    uiint.RegisterWidget({"slider", "Max Coverage", "building.max_coverage", {"building", "sizing"}});
    
    ImGui::Spacing();
    
    // === Budget & Population ===
    ImGui::SeparatorText("Budget & Population");
    
    ImGui::SliderFloat("Budget per Capita", &s_building_params.budget_per_capita, 50000.0f, 200000.0f, "%.0f");
    uiint.RegisterWidget({"slider", "Budget per Capita", "building.budget_per_capita", {"building", "economy"}});
    
    ImGui::SliderInt("Target Population", &s_building_params.target_population, 10000, 100000);
    uiint.RegisterWidget({"slider", "Target Population", "building.target_population", {"building", "population"}});
    
    ImGui::Spacing();
    
    // === Performance Options ===
    ImGui::SeparatorText("Performance");
    
    ImGui::Checkbox("Auto Threading", &s_building_params.auto_threading);
    uiint.RegisterWidget({"checkbox", "Auto Threading", "building.auto_threading", {"building", "performance"}});
    
    if (!s_building_params.auto_threading) {
        ImGui::SliderInt("Threading Threshold", &s_building_params.threading_threshold, 10, 500);
        uiint.RegisterWidget({"slider", "Threading Threshold", "building.threading_threshold", {"building", "performance"}});
    }
    
    ImGui::Spacing();
    
    // === Generation Button (Y2K pulse affordance) ===
    if (s_is_generating) {
        const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 4.0f);
        const ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
    }
    
    if (ImGui::Button("Place Buildings", ImVec2(-1, 40))) {
        // DEBUG_TAG: BUILDING_CONTROL_GENERATE_CLICKED
        GlobalState& gs = GetGlobalState();
        s_zoning_bridge.Generate(s_building_params, gs);
        s_is_generating = true;
        s_gen_start_time = static_cast<float>(ImGui::GetTime());
    }
    uiint.RegisterWidget({"button", "Place Buildings", "action:building.generate", {"action", "building"}});
    uiint.RegisterAction({"building.generate", "Place Buildings", "BuildingControl", {}, "ZoningBridge::Generate"});
    
    if (s_is_generating) {
        ImGui::PopStyleColor();
        // Reset pulse after 1 second
        if (ImGui::GetTime() - s_gen_start_time > 1.0f) {
            s_is_generating = false;
        }
    }
    
    ImGui::Spacing();
    
    // === Status Display ===
    ImGui::SeparatorText("Status");
    
    GlobalState& gs = GetGlobalState();
    ImGui::Text("Total Buildings: %zu", gs.buildings.size());
    
    // Show last generation stats
    auto stats = s_zoning_bridge.GetLastStats();
    if (stats.buildings_placed > 0) {
        ImGui::Text("Last Generation:");
        ImGui::Indent();
        ImGui::Text("  Buildings Placed: %d", stats.buildings_placed);
        ImGui::Text("  Projected Population: %d", stats.projected_population);
        ImGui::Text("  Budget Allocated: %.2f", stats.total_budget_allocated);
        ImGui::Text("  Generation Time: %.1f ms", stats.generation_time_ms);
        ImGui::Unindent();
    }
    
}

void Draw(float dt)
{
    using namespace RogueCity::Core::Editor;
    
    // State-reactive: Only show if in BuildingPlacement mode
    EditorHFSM& hfsm = GetEditorHFSM();
    if (hfsm.state() != EditorState::Editing_Buildings) {
        return;
    }
    
    const bool open = Components::BeginTokenPanel(
        "Building Placement Control",
        UITokens::AmberGlow,
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize);
    
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Building Placement Control",
            "BuildingControl",
            "building_placement",
            "Right",
            "visualizer/src/ui/panels/rc_panel_building_control.cpp",
            {"generation", "building", "controls"}
        },
        open
    );
    
    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }
    
    DrawContent(dt);
    
    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::BuildingControl
