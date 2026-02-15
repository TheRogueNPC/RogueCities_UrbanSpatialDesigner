// FILE: rc_panel_lot_control.cpp
// PURPOSE: Control panel for lot subdivision with state-reactive rendering
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_LOT_CONTROL_PANEL
// AGENT: UI/UX_Master + Coder_Agent
// CATEGORY: Control_Panel

#include "ui/panels/rc_panel_lot_control.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/App/Integration/ZoningBridge.hpp>

#include <imgui.h>

namespace RC_UI::Panels::LotControl {

// Lot generation parameters (persistent UI state)
static RogueCity::App::Integration::ZoningBridge::UiConfig s_lot_params;
static bool s_is_generating = false;
static float s_gen_start_time = 0.0f;
static RogueCity::App::Integration::ZoningBridge s_zoning_bridge;

void DrawContent(float dt)
{
    using namespace RogueCity::Core::Editor;
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    // === Parameters Section ===
    ImGui::SeparatorText("Lot Parameters");
    
    ImGui::SliderInt("Min Lot Width (m)", &s_lot_params.min_lot_width, 5, 30);
    uiint.RegisterWidget({"slider", "Min Lot Width", "lot.min_width", {"lot", "sizing"}});
    
    ImGui::SliderInt("Max Lot Width (m)", &s_lot_params.max_lot_width, 20, 80);
    uiint.RegisterWidget({"slider", "Max Lot Width", "lot.max_width", {"lot", "sizing"}});
    
    ImGui::SliderInt("Min Lot Depth (m)", &s_lot_params.min_lot_depth, 10, 40);
    uiint.RegisterWidget({"slider", "Min Lot Depth", "lot.min_depth", {"lot", "sizing"}});
    
    ImGui::SliderInt("Max Lot Depth (m)", &s_lot_params.max_lot_depth, 30, 100);
    uiint.RegisterWidget({"slider", "Max Lot Depth", "lot.max_depth", {"lot", "sizing"}});
    
    ImGui::Spacing();
    
    // === Building Coverage ===
    ImGui::SeparatorText("Building Coverage");
    
    ImGui::SliderFloat("Min Coverage", &s_lot_params.min_building_coverage, 0.2f, 0.6f, "%.1f%%");
    uiint.RegisterWidget({"slider", "Min Coverage", "lot.min_coverage", {"lot", "building"}});
    
    ImGui::SliderFloat("Max Coverage", &s_lot_params.max_building_coverage, 0.5f, 0.9f, "%.1f%%");
    uiint.RegisterWidget({"slider", "Max Coverage", "lot.max_coverage", {"lot", "building"}});
    
    ImGui::Spacing();
    
    // === Generation Button (Y2K pulse affordance) ===
    if (s_is_generating) {
        const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 4.0f);
        const ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
    }
    
    if (ImGui::Button("Generate Lots", ImVec2(-1, 40))) {
        // DEBUG_TAG: LOT_CONTROL_GENERATE_CLICKED
        GlobalState& gs = GetGlobalState();
        s_zoning_bridge.Generate(s_lot_params, gs);
        s_is_generating = true;
        s_gen_start_time = static_cast<float>(ImGui::GetTime());
    }
    uiint.RegisterWidget({"button", "Generate Lots", "action:lot.generate", {"action", "lot"}});
    uiint.RegisterAction({"lot.generate", "Generate Lots", "LotControl", {}, "ZoningBridge::Generate"});
    
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
    ImGui::Text("Total Lots: %zu", gs.lots.size());
    
    // Show last generation stats
    auto stats = s_zoning_bridge.GetLastStats();
    if (stats.lots_created > 0) {
        ImGui::Text("Last Generation:");
        ImGui::Indent();
        ImGui::Text("  Lots Created: %d", stats.lots_created);
        ImGui::Text("  Buildings: %d", stats.buildings_placed);
        ImGui::Text("  Generation Time: %.1f ms", stats.generation_time_ms);
        ImGui::Unindent();
    }
    
}

void Draw(float dt)
{
    using namespace RogueCity::Core::Editor;
    
    // State-reactive: Only show if in LotPlacement mode
    EditorHFSM& hfsm = GetEditorHFSM();
    if (hfsm.state() != EditorState::Editing_Lots) {
        return;
    }
    
    const bool open = Components::BeginTokenPanel(
        "Lot Subdivision Control",
        UITokens::SuccessGreen,
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize);
    
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Lot Subdivision Control",
            "LotControl",
            "lot_subdivision",
            "Right",
            "visualizer/src/ui/panels/rc_panel_lot_control.cpp",
            {"generation", "lot", "controls"}
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

} // namespace RC_UI::Panels::LotControl
