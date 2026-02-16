#include "ui/tools/rc_tool_geometry_policy.h"

#include <algorithm>

namespace RC_UI::Tools {

ToolGeometryPolicy ResolveToolGeometryPolicy(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::EditorState editor_state) {
    using RogueCity::Core::Editor::BuildingSubtool;
    using RogueCity::Core::Editor::DistrictSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::LotSubtool;
    using RogueCity::Core::Editor::RoadSubtool;
    using RogueCity::Core::Editor::WaterSubtool;

    ToolGeometryPolicy policy{};
    const double scale = std::max(0.75, static_cast<double>(gs.config.ui_scale));
    policy.district_placement_half_extent *= scale;
    policy.lot_placement_half_extent *= scale;
    policy.merge_radius_world *= scale;
    policy.water_falloff_radius_world *= scale;

    if (editor_state == EditorState::Editing_Districts) {
        if (gs.tool_runtime.district_subtool == DistrictSubtool::Zone) {
            policy.district_placement_half_extent *= 1.15;
        } else if (gs.tool_runtime.district_subtool == DistrictSubtool::Merge) {
            policy.merge_radius_world *= 1.4;
        }
    } else if (editor_state == EditorState::Editing_Lots) {
        if (gs.tool_runtime.lot_subtool == LotSubtool::Plot) {
            policy.lot_placement_half_extent *= 1.1;
        } else if (gs.tool_runtime.lot_subtool == LotSubtool::Merge) {
            policy.merge_radius_world *= 1.2;
        }
    } else if (editor_state == EditorState::Editing_Buildings) {
        if (gs.tool_runtime.building_subtool == BuildingSubtool::Scale) {
            policy.building_default_scale = 1.1;
        }
    } else if (editor_state == EditorState::Editing_Roads) {
        if (gs.tool_runtime.road_subtool == RoadSubtool::Strengthen) {
            policy.merge_radius_world *= 1.15;
        }
    } else if (editor_state == EditorState::Editing_Water) {
        if (gs.tool_runtime.water_subtool == WaterSubtool::Erode) {
            policy.water_falloff_radius_world *= 1.35;
        } else if (gs.tool_runtime.water_subtool == WaterSubtool::Contour) {
            policy.water_falloff_radius_world *= 1.2;
        } else if (gs.tool_runtime.water_subtool == WaterSubtool::Flow) {
            policy.water_falloff_radius_world *= 0.9;
        }
    }

    return policy;
}

double ComputeFalloffWeight(double distance, double radius) {
    if (radius <= 1e-6) {
        return 0.0;
    }
    const double t = std::clamp(distance / radius, 0.0, 1.0);
    const double one_minus_t = 1.0 - t;
    // Smoothstep for stable brush falloff.
    return one_minus_t * one_minus_t * (3.0 - 2.0 * one_minus_t);
}

} // namespace RC_UI::Tools

