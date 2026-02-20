#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include <algorithm>

namespace RC_UI::Viewport::Handlers {

bool HandleBuildingPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    (void)interaction_state;

    using RogueCity::Core::BuildingSite;
    using RogueCity::Core::BuildingType;
    using RogueCity::Core::Editor::BuildingSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Buildings) {
        return false;
    }

    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl;
    if (!add_click) {
        return false;
    }

    if (context.gs.tool_runtime.building_subtool == BuildingSubtool::Place) {
        BuildingSite building{};
        building.id = NextBuildingId(context.gs);
        building.lot_id = context.gs.selection.selected_lot ? context.gs.selection.selected_lot->id : 0u;
        building.district_id = context.gs.selection.selected_district ? context.gs.selection.selected_district->id : 0u;
        building.position = context.world_pos;
        building.uniform_scale = static_cast<float>(context.geometry_policy.building_default_scale);
        building.type = BuildingType::Residential;
        building.is_user_placed = true;
        building.generation_tag = RogueCity::Core::GenerationTag::M_user;
        building.generation_locked = true;
        context.gs.buildings.push_back(building);
        SetPrimarySelection(context.gs, VpEntityKind::Building, building.id);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Building);
        return true;
    }

    if (context.gs.tool_runtime.building_subtool == BuildingSubtool::Assign) {
        RogueCity::Core::BuildingSite* building = nullptr;
        if (context.gs.selection.selected_building) {
            building = FindBuildingMutable(context.gs, context.gs.selection.selected_building->id);
        }
        if (building == nullptr) {
            const auto picked = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
            if (picked.has_value() && picked->kind == VpEntityKind::Building) {
                building = FindBuildingMutable(context.gs, picked->id);
                if (building != nullptr) {
                    SetPrimarySelection(context.gs, VpEntityKind::Building, building->id);
                }
            }
        }
        if (building != nullptr) {
            CycleBuildingType(building->type);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Building);
            return true;
        }
    }

    if (context.gs.tool_runtime.building_subtool == BuildingSubtool::Rotate ||
        context.gs.tool_runtime.building_subtool == BuildingSubtool::Scale) {
        RogueCity::Core::BuildingSite* building = nullptr;
        if (context.gs.selection.selected_building) {
            building = FindBuildingMutable(context.gs, context.gs.selection.selected_building->id);
        }
        if (building == nullptr) {
            const auto picked = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
            if (picked.has_value() && picked->kind == VpEntityKind::Building) {
                building = FindBuildingMutable(context.gs, picked->id);
                if (building != nullptr) {
                    SetPrimarySelection(context.gs, VpEntityKind::Building, picked->id);
                }
            }
        }
        if (building != nullptr) {
            if (context.gs.tool_runtime.building_subtool == BuildingSubtool::Rotate) {
                constexpr float kRotateStepRadians = 0.261799f; // 15deg
                building->rotation_radians += kRotateStepRadians;
            } else {
                constexpr float kScaleStep = 1.1f;
                building->uniform_scale = std::clamp(building->uniform_scale * kScaleStep, 0.1f, 25.0f);
            }
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Building);
            return true;
        }
    }

    return false;
}

} // namespace RC_UI::Viewport::Handlers
