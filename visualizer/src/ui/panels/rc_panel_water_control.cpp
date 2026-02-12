// FILE: rc_panel_water_control.cpp
// PURPOSE: Control panel for water/river editing.
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_WATER_CONTROL_PANEL
// AGENT: UI/UX_Master
// CATEGORY: Control_Panel_Authoring
// NOTE: This panel now provides functional authoring seeds and live diagnostics.

#include "ui/panels/rc_panel_water_control.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>
#include <algorithm>
#include <array>

namespace RC_UI::Panels::WaterControl {

void Draw(float dt)
{
    using namespace RogueCity::Core::Editor;
    
    // State-reactive: Only show if in WaterEditing mode
    EditorHFSM& hfsm = GetEditorHFSM();
    if (hfsm.state() != EditorState::Editing_Water) {
        return;
    }
    
    const bool open = Components::BeginTokenPanel(
        "Water Editing Control",
        UITokens::InfoBlue,
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize);
    
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Water Editing Control",
            "WaterControl",
            "water_editing",
            "Right",
            "visualizer/src/ui/panels/rc_panel_water_control.cpp",
            {"water", "controls", "authoring"}
        },
        open
    );
    
    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    GlobalState& gs = GetGlobalState();
    static int selected_type = 0;
    static float depth_m = 6.0f;
    static bool generate_shore = true;
    static ButtonFeedback add_feedback{};

    const char* types[] = {"Lake", "River", "Ocean", "Pond"};
    ImGui::SetNextItemWidth(170.0f);
    ImGui::Combo("Water Type", &selected_type, types, IM_ARRAYSIZE(types));
    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderFloat("Depth (m)", &depth_m, 0.5f, 40.0f, "%.1f");
    ImGui::Checkbox("Generate Shore", &generate_shore);

    if (Components::AnimatedActionButton("spawn_water", "Add Sample Body", add_feedback, dt, false, ImVec2(170.0f, 30.0f))) {
        RogueCity::Core::WaterBody body{};
        body.id = static_cast<uint32_t>(gs.waterbodies.size() + 1u);
        body.type = static_cast<RogueCity::Core::WaterType>(std::clamp(selected_type, 0, 3));
        body.depth = depth_m;
        body.generate_shore = generate_shore;
        body.is_user_placed = true;

        const RogueCity::Core::Bounds bounds = gs.world_constraints.isValid()
            ? RogueCity::Core::Bounds{
                {0.0, 0.0},
                {
                    static_cast<double>(gs.world_constraints.width) * gs.world_constraints.cell_size,
                    static_cast<double>(gs.world_constraints.height) * gs.world_constraints.cell_size
                }}
            : gs.texture_space_bounds;
        const RogueCity::Core::Vec2 c = bounds.center();
        const double r = std::max(35.0, std::min(bounds.width(), bounds.height()) * 0.04);
        body.boundary = {
            {c.x - r, c.y - r},
            {c.x + r, c.y - r * 0.5},
            {c.x + r * 0.8, c.y + r},
            {c.x - r * 0.7, c.y + r * 0.7}
        };

        gs.waterbodies.add(std::move(body));
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Last")) {
        const auto size = gs.waterbodies.size();
        if (size > 0) {
            const auto handle = gs.waterbodies.createHandleFromData(size - 1u);
            // Validate handle and size hasn't changed during creation
            if (handle && gs.waterbodies.size() == size) {
                gs.waterbodies.remove(handle);
            }
        }
    }

    ImGui::SeparatorText("Status");
    ImGui::Text("Total Water Bodies: %zu", gs.waterbodies.size());
    Components::StatusChip(gs.waterbodies.size() > 0 ? "ACTIVE" : "EMPTY",
        gs.waterbodies.size() > 0 ? UITokens::SuccessGreen : UITokens::YellowWarning,
        true);
    ImGui::TextDisabled("Sample authoring is now live; spline/paint tools remain next.");

    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::WaterControl
