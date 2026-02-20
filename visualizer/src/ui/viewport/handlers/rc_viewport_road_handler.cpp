#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace RC_UI::Viewport::Handlers {
namespace {

void ApplyTinySplinePass(
    std::vector<RogueCity::Core::Vec2>& points,
    const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.spline_editor.enabled || points.size() < 3) {
        return;
    }

    RogueCity::App::EditorManipulation::SplineOptions options{};
    options.closed = false;
    options.samples_per_segment = std::clamp(gs.spline_editor.samples_per_segment, 2, 5);
    options.tension = std::clamp(gs.spline_editor.tension, 0.0f, 1.0f);
    auto smoothed = RogueCity::App::EditorManipulation::BuildCatmullRomSpline(points, options);
    if (smoothed.size() >= 2 && smoothed.size() <= 256) {
        points = std::move(smoothed);
    }
}

RogueCity::Core::RoadType DefaultRoadTypeForSubtool(RogueCity::Core::Editor::RoadSubtool subtool) {
    using RogueCity::Core::Editor::RoadSubtool;
    using RogueCity::Core::RoadType;
    switch (subtool) {
    case RoadSubtool::Bridge:
        return RoadType::Highway;
    case RoadSubtool::Grid:
        return RoadType::Street;
    case RoadSubtool::Stub:
        return RoadType::M_Minor;
    case RoadSubtool::Strengthen:
        return RoadType::M_Major;
    case RoadSubtool::Curve:
        return RoadType::Avenue;
    case RoadSubtool::Select:
    case RoadSubtool::Disconnect:
    case RoadSubtool::Inspect:
    case RoadSubtool::Spline:
    default:
        return RoadType::M_Minor;
    }
}

void StrengthenRoadType(RogueCity::Core::RoadType& type) {
    using RogueCity::Core::RoadType;
    switch (type) {
    case RoadType::Driveway:
    case RoadType::Drive:
    case RoadType::CulDeSac:
    case RoadType::Alleyway:
    case RoadType::Lane:
    case RoadType::Street:
    case RoadType::M_Minor:
        type = RoadType::M_Major;
        break;
    case RoadType::Boulevard:
    case RoadType::Avenue:
    case RoadType::Arterial:
    case RoadType::M_Major:
        type = RoadType::Highway;
        break;
    case RoadType::Highway:
    default:
        break;
    }
}

RogueCity::Core::Road* ResolveRoadTarget(
    ViewportInteractionContext& context,
    const RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t selected_id) {
    if (selected_id != 0u) {
        if (auto* road = FindRoadMutable(context.gs, selected_id); road != nullptr) {
            return road;
        }
    }

    const auto picked = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
    if (!picked.has_value() || picked->kind != kind) {
        return nullptr;
    }
    SetPrimarySelection(context.gs, kind, picked->id);
    return FindRoadMutable(context.gs, picked->id);
}

} // namespace

bool HandleRoadPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::RoadSplineSubtool;
    using RogueCity::Core::Editor::RoadSubtool;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::Road;
    using RogueCity::Core::RoadType;

    if (context.editor_state != EditorState::Editing_Roads) {
        return false;
    }

    const bool left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !context.io.KeyAlt;
    const bool add_click = left_click && !context.io.KeyCtrl;
    const bool ctrl_double_click = context.io.KeyCtrl && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    if (!left_click && !interaction_state.road_pen_drag.active) {
        return false;
    }

    const auto active_subtool = context.gs.tool_runtime.road_subtool;
    const auto active_spline_subtool = context.gs.tool_runtime.road_spline_subtool;

    if (active_subtool == RoadSubtool::Strengthen) {
        const uint32_t selected_id = context.gs.selection.selected_road ? context.gs.selection.selected_road->id : 0u;
        if (Road* road = ResolveRoadTarget(context, VpEntityKind::Road, selected_id); road != nullptr) {
            StrengthenRoadType(road->type);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }
        return false;
    }

    if (active_subtool == RoadSubtool::Disconnect) {
        const uint32_t selected_id = context.gs.selection.selected_road ? context.gs.selection.selected_road->id : 0u;
        Road* road = ResolveRoadTarget(context, VpEntityKind::Road, selected_id);
        if (road == nullptr || road->points.size() < 2) {
            return false;
        }

        const double pick_radius = context.interaction_metrics.world_vertex_pick_radius;
        double best = std::numeric_limits<double>::max();
        size_t best_idx = 0;
        for (size_t i = 0; i < road->points.size(); ++i) {
            const double d = road->points[i].distanceTo(context.world_pos);
            if (d < best) {
                best = d;
                best_idx = i;
            }
        }
        if (best > pick_radius) {
            return false;
        }

        if (road->points.size() <= 2) {
            const uint32_t remove_id = road->id;
            for (size_t i = 0; i < context.gs.roads.size(); ++i) {
                auto handle = context.gs.roads.createHandleFromData(i);
                if (handle && handle->id == remove_id) {
                    context.gs.roads.remove(handle);
                    break;
                }
            }
            context.gs.selection_manager.Clear();
            context.gs.selection.selected_road = {};
            context.gs.selection.selected_district = {};
            context.gs.selection.selected_lot = {};
            context.gs.selection.selected_building = {};
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }

        if (best_idx == 0u) {
            road->points.erase(road->points.begin());
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }
        if (best_idx + 1u >= road->points.size()) {
            road->points.pop_back();
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }

        Road split_road{};
        split_road.id = NextRoadId(context.gs);
        split_road.type = road->type;
        split_road.is_user_created = true;
        split_road.generation_tag = RogueCity::Core::GenerationTag::M_user;
        split_road.generation_locked = true;
        split_road.points.assign(
            road->points.begin() + static_cast<std::ptrdiff_t>(best_idx),
            road->points.end());
        road->points.erase(
            road->points.begin() + static_cast<std::ptrdiff_t>(best_idx),
            road->points.end());
        if (split_road.points.size() >= 2) {
            context.gs.roads.add(std::move(split_road));
        }
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
        return true;
    }

    if (active_spline_subtool != RoadSplineSubtool::Pen) {
        return false;
    }

    auto* road = context.gs.selection.selected_road
        ? FindRoadMutable(context.gs, context.gs.selection.selected_road->id)
        : nullptr;

    if (ctrl_double_click && road != nullptr && road->points.size() >= 3) {
        const RogueCity::Core::Vec2 start = road->points.front();
        if (road->points.back().distanceTo(start) > 1e-3) {
            road->points.push_back(start);
        }
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
        interaction_state.road_pen_drag.active = false;
        return true;
    }

    if (active_subtool == RoadSubtool::Stub) {
        constexpr double kStubLength = 18.0;
        if (road != nullptr && !road->points.empty()) {
            RogueCity::Core::Vec2 snapped = context.world_pos;
            SnapToGrid(
                snapped,
                context.gs.district_boundary_editor.snap_to_grid,
                context.gs.district_boundary_editor.snap_size);
            if (snapped.distanceTo(road->points.back()) < 1.0) {
                snapped = road->points.back() + RogueCity::Core::Vec2(kStubLength, 0.0);
            }
            road->points.push_back(snapped);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }

        Road stub{};
        stub.id = NextRoadId(context.gs);
        stub.type = DefaultRoadTypeForSubtool(active_subtool);
        stub.is_user_created = true;
        stub.generation_tag = RogueCity::Core::GenerationTag::M_user;
        stub.generation_locked = true;
        stub.points.push_back(context.world_pos);
        stub.points.push_back(context.world_pos + RogueCity::Core::Vec2(kStubLength, 0.0));
        context.gs.roads.add(std::move(stub));
        SetPrimarySelection(context.gs, VpEntityKind::Road, NextRoadId(context.gs) - 1u);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
        return true;
    }

    if (road != nullptr &&
        interaction_state.road_pen_drag.active &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !context.io.KeyShift &&
        !context.io.KeyCtrl) {
        const double step = std::max(0.25, context.interaction_metrics.world_lasso_step * 0.5);
        if (context.world_pos.distanceTo(interaction_state.road_pen_drag.last_point) >= step) {
            RogueCity::Core::Vec2 free_point = context.world_pos;
            if (active_subtool == RoadSubtool::Grid) {
                SnapToGrid(
                    free_point,
                    context.gs.district_boundary_editor.snap_to_grid,
                    context.gs.district_boundary_editor.snap_size);
            }
            road->points.push_back(free_point);
            interaction_state.road_pen_drag.last_point = free_point;
            if (active_subtool == RoadSubtool::Curve) {
                ApplyTinySplinePass(road->points, context.gs);
            }
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            return true;
        }
    }
    if (interaction_state.road_pen_drag.active && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        interaction_state.road_pen_drag.active = false;
    }

    if (!add_click) {
        return false;
    }

    RogueCity::Core::Vec2 next_point = context.world_pos;
    if (active_subtool == RoadSubtool::Grid) {
        SnapToGrid(
            next_point,
            context.gs.district_boundary_editor.snap_to_grid,
            context.gs.district_boundary_editor.snap_size);
    }

    if (road == nullptr) {
        Road new_road{};
        new_road.id = NextRoadId(context.gs);
        new_road.type = DefaultRoadTypeForSubtool(active_subtool);
        new_road.is_user_created = true;
        new_road.generation_tag = RogueCity::Core::GenerationTag::M_user;
        new_road.generation_locked = true;
        new_road.points.push_back(next_point);
        context.gs.roads.add(std::move(new_road));
        const uint32_t new_id = NextRoadId(context.gs) - 1u;
        SetPrimarySelection(context.gs, VpEntityKind::Road, new_id);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
    } else {
        road->points.push_back(next_point);
        if (active_subtool == RoadSubtool::Curve) {
            ApplyTinySplinePass(road->points, context.gs);
        }
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
    }

    interaction_state.road_pen_drag.active = !context.io.KeyShift;
    interaction_state.road_pen_drag.entity_id = road == nullptr
        ? (NextRoadId(context.gs) - 1u)
        : road->id;
    interaction_state.road_pen_drag.last_point = next_point;
    return true;
}

bool HandleRoadVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::RoadSplineSubtool;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Roads) {
        return false;
    }

    uint32_t road_id = context.gs.selection.selected_road ? context.gs.selection.selected_road->id : 0u;
    if (road_id == 0u &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::Road) {
            SetPrimarySelection(context.gs, VpEntityKind::Road, picked->id);
            road_id = picked->id;
        }
    }
    if (road_id == 0u) {
        return false;
    }

    RogueCity::Core::Road* road = FindRoadMutable(context.gs, road_id);
    if (road == nullptr || road->points.size() < 2) {
        return false;
    }

    if (interaction_state.road_vertex_drag.active &&
        interaction_state.road_vertex_drag.road_id != road_id) {
        interaction_state.road_vertex_drag.active = false;
        interaction_state.road_vertex_drag.tangent_handle_active = false;
    }

    bool consumed_interaction = false;
    const bool precision_pick_mode = context.io.KeyShift && context.io.KeyCtrl;
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius *
        (precision_pick_mode ? 0.65 : 1.35);

    if (!consumed_interaction &&
        context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Selection &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::Road) {
            SetPrimarySelection(context.gs, VpEntityKind::Road, picked->id);
            consumed_interaction = true;
        }
    }

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
                split_road.is_user_created = true;
                split_road.generation_tag = RogueCity::Core::GenerationTag::M_user;
                split_road.generation_locked = true;
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
        (context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::DirectSelect ||
         context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::HandleTangents ||
         context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::SnapAlign)) {
        if (context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::HandleTangents &&
            road->points.size() >= 3) {
            constexpr double kHandleScale = 3.0;
            for (size_t i = 1; i + 1 < road->points.size(); ++i) {
                const RogueCity::Core::Vec2 anchor = road->points[i];
                const RogueCity::Core::Vec2 prev = road->points[i - 1];
                const RogueCity::Core::Vec2 next = road->points[i + 1];
                RogueCity::Core::Vec2 in_dir = anchor - prev;
                RogueCity::Core::Vec2 out_dir = anchor - next;
                if (in_dir.lengthSquared() > 1e-8) {
                    in_dir.normalize();
                    const RogueCity::Core::Vec2 handle = anchor + in_dir * (pick_radius * kHandleScale);
                    if (handle.distanceTo(context.world_pos) <= pick_radius * 1.2) {
                        interaction_state.road_vertex_drag.active = true;
                        interaction_state.road_vertex_drag.road_id = road_id;
                        interaction_state.road_vertex_drag.vertex_index = i;
                        interaction_state.road_vertex_drag.tangent_handle_active = true;
                        interaction_state.road_vertex_drag.tangent_outgoing = false;
                        consumed_interaction = true;
                        break;
                    }
                }
                if (out_dir.lengthSquared() > 1e-8) {
                    out_dir.normalize();
                    const RogueCity::Core::Vec2 handle = anchor + out_dir * (pick_radius * kHandleScale);
                    if (handle.distanceTo(context.world_pos) <= pick_radius * 1.2) {
                        interaction_state.road_vertex_drag.active = true;
                        interaction_state.road_vertex_drag.road_id = road_id;
                        interaction_state.road_vertex_drag.vertex_index = i;
                        interaction_state.road_vertex_drag.tangent_handle_active = true;
                        interaction_state.road_vertex_drag.tangent_outgoing = true;
                        consumed_interaction = true;
                        break;
                    }
                }
            }
        }
        if (consumed_interaction) {
            return true;
        }

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
            interaction_state.road_vertex_drag.tangent_handle_active = false;
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
            if (interaction_state.road_vertex_drag.vertex_index >= road->points.size()) {
                interaction_state.road_vertex_drag.active = false;
                return consumed_interaction;
            }
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
            if (interaction_state.road_vertex_drag.tangent_handle_active &&
                interaction_state.road_vertex_drag.vertex_index > 0 &&
                interaction_state.road_vertex_drag.vertex_index + 1 < road->points.size()) {
                const size_t anchor_i = interaction_state.road_vertex_drag.vertex_index;
                const RogueCity::Core::Vec2 anchor = road->points[anchor_i];
                const RogueCity::Core::Vec2 tangent = snapped - anchor;
                const size_t neighbor_i = interaction_state.road_vertex_drag.tangent_outgoing
                    ? (anchor_i + 1)
                    : (anchor_i - 1);
                road->points[neighbor_i] = anchor - tangent;
            } else {
                RogueCity::App::EditorManipulation::MoveRoadVertex(
                    *road,
                    interaction_state.road_vertex_drag.vertex_index,
                    snapped);
            }
            if (context.gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::HandleTangents) {
                ApplyTinySplinePass(road->points, context.gs);
            }
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Road);
            consumed_interaction = true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            interaction_state.road_vertex_drag.active = false;
            interaction_state.road_vertex_drag.tangent_handle_active = false;
            consumed_interaction = true;
        }
    }

    return consumed_interaction;
}

} // namespace RC_UI::Viewport::Handlers
