#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace RC_UI::Viewport::Handlers {

bool HandleRoadPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    (void)interaction_state;

    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::RoadSplineSubtool;
    using RogueCity::Core::Editor::RoadSubtool;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::Road;
    using RogueCity::Core::RoadType;

    if (context.editor_state != EditorState::Editing_Roads) {
        return false;
    }

    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl;
    if (!add_click || context.gs.tool_runtime.road_spline_subtool != RoadSplineSubtool::Pen) {
        return false;
    }

    auto* road = context.gs.selection.selected_road
        ? FindRoadMutable(context.gs, context.gs.selection.selected_road->id)
        : nullptr;
    if (road == nullptr) {
        Road new_road{};
        new_road.id = NextRoadId(context.gs);
        new_road.type = (context.gs.tool_runtime.road_subtool == RoadSubtool::Bridge)
            ? RoadType::M_Major
            : RoadType::M_Minor;
        new_road.is_user_created = true;
        new_road.generation_tag = RogueCity::Core::GenerationTag::M_user;
        new_road.generation_locked = true;
        new_road.points.push_back(context.world_pos);
        context.gs.roads.add(std::move(new_road));
        const uint32_t new_id = NextRoadId(context.gs) - 1u;
        SetPrimarySelection(context.gs, VpEntityKind::Road, new_id);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
    } else {
        road->points.push_back(context.world_pos);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
    }
    return true;
}

bool HandleRoadVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::RoadSplineSubtool;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Roads || !context.gs.selection.selected_road) {
        return false;
    }

    const uint32_t road_id = context.gs.selection.selected_road->id;
    RogueCity::Core::Road* road = FindRoadMutable(context.gs, road_id);
    if (road == nullptr || road->points.size() < 2) {
        return false;
    }

    bool consumed_interaction = false;
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius;

    if (!consumed_interaction &&
        context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::ConvertAnchor &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift &&
        road->points.size() >= 3) {
        double best = std::numeric_limits<double>::max();
        size_t best_idx = 1;
        for (size_t i = 1; i + 1 < road->points.size(); ++i) {
            const double d = road->points[i].distanceTo(context.world_pos);
            if (d < best) {
                best = d;
                best_idx = i;
            }
        }
        if (best <= pick_radius) {
            road->points[best_idx] = (road->points[best_idx - 1] + road->points[best_idx + 1]) * 0.5;
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            consumed_interaction = true;
        }
    }

    if (!consumed_interaction &&
        context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::JoinSplit &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift) {
        if (context.io.KeyCtrl) {
            double best_edge = std::numeric_limits<double>::max();
            size_t edge_idx = 0;
            for (size_t i = 0; i + 1 < road->points.size(); ++i) {
                const double d = DistanceToSegment(context.world_pos, road->points[i], road->points[i + 1]);
                if (d < best_edge) {
                    best_edge = d;
                    edge_idx = i;
                }
            }
            if (best_edge <= pick_radius * context.geometry_policy.edge_insert_multiplier) {
                const RogueCity::Core::Vec2 midpoint = (road->points[edge_idx] + road->points[edge_idx + 1]) * 0.5;
                road->points.insert(road->points.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1), midpoint);
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
                consumed_interaction = true;
            }
        } else if (road->points.size() >= 4) {
            double best = std::numeric_limits<double>::max();
            size_t best_idx = 1;
            for (size_t i = 1; i + 1 < road->points.size(); ++i) {
                const double d = road->points[i].distanceTo(context.world_pos);
                if (d < best) {
                    best = d;
                    best_idx = i;
                }
            }
            if (best <= pick_radius) {
                RogueCity::Core::Road split_road{};
                split_road.id = NextRoadId(context.gs);
                split_road.type = road->type;
                split_road.is_user_created = road->is_user_created;
                split_road.generation_tag = road->generation_tag;
                split_road.generation_locked = road->generation_locked;
                split_road.points.assign(
                    road->points.begin() + static_cast<std::ptrdiff_t>(best_idx),
                    road->points.end());
                road->points.erase(
                    road->points.begin() + static_cast<std::ptrdiff_t>(best_idx),
                    road->points.end());
                context.gs.roads.add(std::move(split_road));
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
                consumed_interaction = true;
            }
        }
    }

    if (!consumed_interaction &&
        context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Simplify &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift) {
        if (SimplifyPolyline(road->points, false)) {
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            consumed_interaction = true;
        }
    }

    if (!interaction_state.road_vertex_drag.active &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt &&
        (context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Selection ||
         context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::DirectSelect ||
         context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::HandleTangents ||
         context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::SnapAlign)) {
        double best = std::numeric_limits<double>::max();
        size_t best_idx = 0;
        for (size_t i = 0; i < road->points.size(); ++i) {
            const double d = road->points[i].distanceTo(context.world_pos);
            if (d < best) {
                best = d;
                best_idx = i;
            }
        }
        if (best <= pick_radius) {
            interaction_state.road_vertex_drag.active = true;
            interaction_state.road_vertex_drag.road_id = road_id;
            interaction_state.road_vertex_drag.vertex_index = best_idx;
            consumed_interaction = true;
        }
    }

    if (!consumed_interaction &&
        context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::AddRemoveAnchor &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift) {
        if (context.io.KeyCtrl && road->points.size() > 2) {
            double best = std::numeric_limits<double>::max();
            size_t best_idx = 0;
            for (size_t i = 0; i < road->points.size(); ++i) {
                const double d = road->points[i].distanceTo(context.world_pos);
                if (d < best) {
                    best = d;
                    best_idx = i;
                }
            }
            if (best <= pick_radius) {
                road->points.erase(road->points.begin() + static_cast<std::ptrdiff_t>(best_idx));
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
                consumed_interaction = true;
            }
        } else {
            double best_edge = std::numeric_limits<double>::max();
            size_t edge_idx = 0;
            for (size_t i = 0; i + 1 < road->points.size(); ++i) {
                const double d = DistanceToSegment(context.world_pos, road->points[i], road->points[i + 1]);
                if (d < best_edge) {
                    best_edge = d;
                    edge_idx = i;
                }
            }
            if (best_edge <= pick_radius * context.geometry_policy.edge_insert_multiplier) {
                road->points.insert(road->points.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1), context.world_pos);
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
                consumed_interaction = true;
            }
        }
    }

    if (interaction_state.road_vertex_drag.active) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            RogueCity::Core::Vec2 snapped = context.world_pos;
            SnapToGrid(
                snapped,
                context.gs.district_boundary_editor.snap_to_grid,
                context.gs.district_boundary_editor.snap_size);
            if (context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::SnapAlign &&
                interaction_state.road_vertex_drag.vertex_index > 0 &&
                interaction_state.road_vertex_drag.vertex_index < road->points.size()) {
                ApplyAxisAlign(snapped, road->points[interaction_state.road_vertex_drag.vertex_index - 1]);
            }
            RogueCity::App::EditorManipulation::MoveRoadVertex(
                *road,
                interaction_state.road_vertex_drag.vertex_index,
                snapped);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            consumed_interaction = true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            interaction_state.road_vertex_drag.active = false;
            consumed_interaction = true;
        }
    }

    return consumed_interaction;
}

} // namespace RC_UI::Viewport::Handlers
