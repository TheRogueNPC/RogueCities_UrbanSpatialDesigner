// FILE: rc_panel_water_control.cpp
// PURPOSE: Control panel for water/river editing (PASS 2 stub)
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_WATER_CONTROL_PANEL_STUB
// AGENT: UI/UX_Master
// CATEGORY: Control_Panel_Stub
// NOTE: Full implementation in PASS 2 Task 2.1

#include "ui/panels/rc_panel_water_control.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>

namespace RC_UI::Panels::WaterControl {

void Draw(float dt)
{
    using namespace RogueCity::Core::Editor;
    
    // State-reactive: Only show if in WaterEditing mode
    EditorHFSM& hfsm = GetEditorHFSM();
    if (hfsm.state() != EditorState::Editing_Water) {
        return;
    }
    
    const bool open = ImGui::Begin("Water Editing Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Water Editing Control",
            "WaterControl",
            "water_editing",
            "Right",
            "visualizer/src/ui/panels/rc_panel_water_control.cpp",
            {"water", "controls", "stub"}
        },
        open
    );
    
    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    // === STUB PLACEHOLDER ===
    ImGui::TextWrapped("Water/River editing tools will be implemented in PASS 2 Task 2.1.");
    ImGui::Spacing();
    ImGui::Separator();
    
    // === Planned Features ===
    ImGui::SeparatorText("Planned Features (PASS 2)");
    ImGui::BulletText("Lake polygon drawing");
    ImGui::BulletText("River spline tracing");
    ImGui::BulletText("Ocean boundary editing");
    ImGui::BulletText("Shore detail generation");
    ImGui::BulletText("Depth painting");
    
    ImGui::Spacing();
    
    // === Status Display ===
    ImGui::SeparatorText("Status");
    
    GlobalState& gs = GetGlobalState();
    ImGui::Text("Total Water Bodies: %zu", gs.waterbodies.size());
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Full implementation: PASS 2");
    
    uiint.EndPanel();
    ImGui::End();
}

} // namespace RC_UI::Panels::WaterControl
