// FILE: rc_panel_zoning_control.cpp
// PURPOSE: Implementation of zoning control panel
// Y2K GEOMETRY: Pulse animations, glow affordances, state-reactive colors

#include "ui/panels/rc_panel_zoning_control.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include <imgui.h>
#include <cmath>

namespace RC_UI::Panels::ZoningControl {

PanelState& GetPanelState() {
    static PanelState state;
    return state;
}

void Draw(float dt) {
    using RogueCity::Core::Editor::GetGlobalState;
    using RogueCity::Core::Editor::GetEditorHFSM;
    
    auto& gs = GetGlobalState();
    auto& hfsm = GetEditorHFSM();
    auto& state = GetPanelState();
    
    // Panel visibility based on HFSM state (Cockpit Doctrine)
    auto editor_state = hfsm.state();
    bool is_visible = (editor_state == RogueCity::Core::Editor::EditorState::Editing_Districts ||
                       editor_state == RogueCity::Core::Editor::EditorState::Editing_Lots ||
                       editor_state == RogueCity::Core::Editor::EditorState::Editing_Buildings);
    
    if (!is_visible) return;
    
    // State-reactive color (Y2K geometry)
    ImVec4 panel_tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (editor_state == RogueCity::Core::Editor::EditorState::Editing_Districts) {
        panel_tint = ImVec4(0.7f, 0.8f, 1.0f, 1.0f);  // Blue tint
    } else if (editor_state == RogueCity::Core::Editor::EditorState::Editing_Lots) {
        panel_tint = ImVec4(0.7f, 1.0f, 0.8f, 1.0f);  // Green tint
    } else if (editor_state == RogueCity::Core::Editor::EditorState::Editing_Buildings) {
        panel_tint = ImVec4(1.0f, 0.8f, 0.7f, 1.0f);  // Orange tint
    }
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(
        panel_tint.x * 0.15f, panel_tint.y * 0.15f, panel_tint.z * 0.15f, 0.95f
    ));
    
    const bool open = ImGui::Begin("Zoning Control", nullptr, ImGuiWindowFlags_NoCollapse);
    
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Zoning Control",
            "Zoning Control",
            "toolbox",
            "Right",
            "visualizer/src/ui/panels/rc_panel_zoning_control.cpp",
            {"zoning", "generator", "control"}
        },
        open
    );
    
    if (!open) {
        uiint.EndPanel();
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    
    // === PARAMETER SLIDERS ===
    ImGui::SeparatorText("Lot Sizing");
    
    bool changed = false;
    changed |= ImGui::SliderInt("Min Lot Width", &state.config.min_lot_width, 5, 30);
    changed |= ImGui::SliderInt("Max Lot Width", &state.config.max_lot_width, 30, 80);
    changed |= ImGui::SliderInt("Min Lot Depth", &state.config.min_lot_depth, 10, 40);
    changed |= ImGui::SliderInt("Max Lot Depth", &state.config.max_lot_depth, 40, 100);
    
    ImGui::SeparatorText("Building Constraints");
    changed |= ImGui::SliderFloat("Min Coverage", &state.config.min_building_coverage, 0.2f, 0.6f, "%.1f%%");
    changed |= ImGui::SliderFloat("Max Coverage", &state.config.max_building_coverage, 0.6f, 0.95f, "%.1f%%");
    
    ImGui::SeparatorText("Budget & Population");
    changed |= ImGui::SliderFloat("Budget per Capita", &state.config.budget_per_capita, 50000.0f, 200000.0f, "$%.0f");
    changed |= ImGui::SliderInt("Target Population", &state.config.target_population, 10000, 100000);
    
    ImGui::SeparatorText("Performance");
    changed |= ImGui::Checkbox("Auto Threading", &state.config.auto_threading);
    if (state.config.auto_threading) {
        changed |= ImGui::SliderInt("Thread Threshold", &state.config.threading_threshold, 50, 500);
    }
    
    state.parameters_changed = changed;
    
    // Update glow intensity (Y2K affordance)
    if (state.parameters_changed) {
        state.glow_intensity = std::min(state.glow_intensity + dt * 2.0f, 1.0f);
    } else {
        state.glow_intensity = std::max(state.glow_intensity - dt * 0.5f, 0.0f);
    }
    
    // === GENERATE BUTTON (with pulse animation) ===
    ImGui::Separator();
    
    if (state.parameters_changed) {
        state.pulse_phase += dt * 3.0f;
        float pulse = 0.5f + 0.5f * std::sin(state.pulse_phase);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(
            0.3f + pulse * 0.3f,
            0.6f + pulse * 0.2f,
            0.9f,
            1.0f
        ));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    }
    
    if (ImGui::Button("Generate Zones & Buildings", ImVec2(-1, 40))) {
        state.is_generating = true;
        
        // Invoke ZoningBridge
        static RogueCity::App::Integration::ZoningBridge bridge;
        bridge.Generate(state.config, gs);
        
        state.is_generating = false;
        state.parameters_changed = false;
        state.pulse_phase = 0.0f;
    }
    
    ImGui::PopStyleColor();
    
    // === STATISTICS DISPLAY ===
    ImGui::Separator();
    ImGui::Text("Last Generation:");
    
    static RogueCity::App::Integration::ZoningBridge bridge;
    auto stats = bridge.GetLastStats();
    
    ImGui::BulletText("Lots Created: %d", stats.lots_created);
    ImGui::BulletText("Buildings Placed: %d", stats.buildings_placed);
    ImGui::BulletText("Budget Allocated: $%.0f", stats.total_budget_allocated);
    ImGui::BulletText("Projected Population: %d", stats.projected_population);
    ImGui::BulletText("Generation Time: %.2f ms", stats.generation_time_ms);
    
    uiint.RegisterWidget({"button", "Generate", "generate_button", {"action"}});
    uiint.EndPanel();
    ImGui::PopStyleColor();
    ImGui::End();
}

} // namespace RC_UI::Panels::ZoningControl
