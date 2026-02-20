#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>

namespace RC_UI::Viewport::Handlers {
namespace {

void ApplyTinySplinePass(
    std::vector<RogueCity::Core::Vec2>& points,
    const RogueCity::Core::Editor::GlobalState& gs,
    bool closed_shape) {
    if (!gs.spline_editor.enabled || points.size() < 3) {
        return;
    }

    RogueCity::App::EditorManipulation::SplineOptions options{};
    options.closed = closed_shape;
    options.samples_per_segment = std::clamp(gs.spline_editor.samples_per_segment, 2, 5);
    options.tension = std::clamp(gs.spline_editor.tension, 0.0f, 1.0f);
    auto smoothed = RogueCity::App::EditorManipulation::BuildCatmullRomSpline(points, options);
    if (smoothed.size() >= 3 && smoothed.size() <= 256) {
        points = std::move(smoothed);
    }
}

RogueCity::Core::Vec2 PolylineCentroid(const std::vector<RogueCity::Core::Vec2>& points) {
    if (points.empty()) {
        return {};
    }
    RogueCity::Core::Vec2 centroid{};
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<double>(points.size());
    return centroid;
}

RogueCity::Core::Vec2 TangentAt(
    const std::vector<RogueCity::Core::Vec2>& points,
    size_t index,
    bool closed) {
    if (points.size() < 2) {
        return {};
    }
    if (index >= points.size()) {
        index = points.size() - 1;
    }

    if (closed && points.size() >= 3) {
        const size_t prev = (index == 0) ? (points.size() - 1) : (index - 1);
        const size_t next = (index + 1) % points.size();
        return points[next] - points[prev];
    }

    if (index == 0) {
        return points[1] - points[0];
    }
    if (index + 1 >= points.size()) {
        return points[index] - points[index - 1];
    }
    return points[index + 1] - points[index - 1];
}

} // namespace

bool HandleWaterPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::Editor::WaterSplineSubtool;
    using RogueCity::Core::Editor::WaterSubtool;
    using RogueCity::Core::WaterBody;
    using RogueCity::Core::WaterType;

    if (context.editor_state != EditorState::Editing_Water) {
        return false;
    }

    const bool left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !context.io.KeyAlt;
    const bool add_click = left_click && !context.io.KeyCtrl;
    const bool ctrl_double_click = context.io.KeyCtrl && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    const auto* primary = context.gs.selection_manager.Primary();
    uint32_t selected_water_id =
        (primary != nullptr && primary->kind == VpEntityKind::Water) ? primary->id : 0u;
    WaterBody* selected_water =
        selected_water_id != 0u ? FindWaterMutable(context.gs, selected_water_id) : nullptr;
    const bool precision_pick_mode = context.io.KeyShift && context.io.KeyCtrl;
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius *
        (precision_pick_mode ? 0.65 : 1.35);

    if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Selection && left_click) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::Water) {
            SetPrimarySelection(context.gs, VpEntityKind::Water, picked->id);
            return true;
        }
    }

    if (selected_water == nullptr &&
        left_click &&
        context.gs.tool_runtime.water_spline_subtool != WaterSplineSubtool::Pen) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            precision_pick_mode ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::Water) {
            SetPrimarySelection(context.gs, VpEntityKind::Water, picked->id);
            selected_water_id = picked->id;
            selected_water = FindWaterMutable(context.gs, selected_water_id);
        }
    }

    if (add_click && context.gs.tool_runtime.water_subtool == WaterSubtool::Mask) {
        if (selected_water == nullptr) {
            const auto picked = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
            if (picked.has_value() && picked->kind == VpEntityKind::Water) {
                selected_water = FindWaterMutable(context.gs, picked->id);
                if (selected_water != nullptr) {
                    SetPrimarySelection(context.gs, VpEntityKind::Water, picked->id);
                }
            }
        }
        if (selected_water != nullptr) {
            selected_water->generate_shore = !selected_water->generate_shore;
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
            return true;
        }
    }

    if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Pen) {
        if (ctrl_double_click && selected_water != nullptr && selected_water->boundary.size() >= 3) {
            const RogueCity::Core::Vec2 start = selected_water->boundary.front();
            if (selected_water->boundary.back().distanceTo(start) > 1e-3) {
                selected_water->boundary.push_back(start);
            }
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
            interaction_state.water_pen_drag.active = false;
            return true;
        }

        if (selected_water != nullptr &&
            interaction_state.water_pen_drag.active &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            !context.io.KeyShift &&
            !context.io.KeyCtrl) {
            const double step = std::max(0.25, context.interaction_metrics.world_lasso_step * 0.5);
            if (context.world_pos.distanceTo(interaction_state.water_pen_drag.last_point) >= step) {
                selected_water->boundary.push_back(context.world_pos);
                interaction_state.water_pen_drag.last_point = context.world_pos;
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                return true;
            }
        }
        if (interaction_state.water_pen_drag.active && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            interaction_state.water_pen_drag.active = false;
        }
    }

    if (add_click && context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Pen) {
        if (selected_water == nullptr) {
            WaterBody water{};
            water.id = NextWaterId(context.gs);
            water.type = WaterType::River;
            water.depth = static_cast<float>(context.geometry_policy.water_default_depth);
            water.generate_shore = true;
            water.is_user_placed = true;
            water.generation_tag = RogueCity::Core::GenerationTag::M_user;
            water.generation_locked = true;
            water.boundary.push_back(context.world_pos);
            context.gs.waterbodies.add(std::move(water));
            SetPrimarySelection(context.gs, VpEntityKind::Water, NextWaterId(context.gs) - 1u);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
        } else {
            selected_water->boundary.push_back(context.world_pos);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
        }
        interaction_state.water_pen_drag.active = !context.io.KeyShift;
        interaction_state.water_pen_drag.entity_id = selected_water == nullptr
            ? (NextWaterId(context.gs) - 1u)
            : selected_water->id;
        interaction_state.water_pen_drag.last_point = context.world_pos;
        return true;
    }

    if (selected_water != nullptr && !selected_water->boundary.empty()) {
        if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::ConvertAnchor &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt && !context.io.KeyShift &&
            selected_water->boundary.size() >= 3) {
            double best = std::numeric_limits<double>::max();
            size_t best_idx = 0;
            for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                const double d = selected_water->boundary[i].distanceTo(context.world_pos);
                if (d < best) {
                    best = d;
                    best_idx = i;
                }
            }
            if (best <= pick_radius) {
                const size_t prev_idx =
                    (best_idx == 0) ? selected_water->boundary.size() - 1 : best_idx - 1;
                const size_t next_idx = (best_idx + 1) % selected_water->boundary.size();
                selected_water->boundary[best_idx] =
                    (selected_water->boundary[prev_idx] + selected_water->boundary[next_idx]) * 0.5;
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                return true;
            }
        }

        if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::AddRemoveAnchor &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt && !context.io.KeyShift) {
            if (context.io.KeyCtrl && selected_water->boundary.size() > 2) {
                double best = std::numeric_limits<double>::max();
                size_t best_idx = 0;
                for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                    const double d = selected_water->boundary[i].distanceTo(context.world_pos);
                    if (d < best) {
                        best = d;
                        best_idx = i;
                    }
                }
                if (best <= pick_radius) {
                    selected_water->boundary.erase(
                        selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(best_idx));
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                    return true;
                }
            } else {
                double best_edge = std::numeric_limits<double>::max();
                size_t edge_idx = 0;
                for (size_t i = 0; i + 1 < selected_water->boundary.size(); ++i) {
                    const double d = DistanceToSegment(
                        context.world_pos,
                        selected_water->boundary[i],
                        selected_water->boundary[i + 1]);
                    if (d < best_edge) {
                        best_edge = d;
                        edge_idx = i;
                    }
                }
                if (best_edge <= pick_radius * context.geometry_policy.edge_insert_multiplier) {
                    selected_water->boundary.insert(
                        selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1),
                        context.world_pos);
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                    return true;
                }
            }
        }

        if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::JoinSplit &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt && !context.io.KeyShift) {
            if (context.io.KeyCtrl) {
                double best_edge = std::numeric_limits<double>::max();
                size_t edge_idx = 0;
                for (size_t i = 0; i + 1 < selected_water->boundary.size(); ++i) {
                    const double d = DistanceToSegment(
                        context.world_pos,
                        selected_water->boundary[i],
                        selected_water->boundary[i + 1]);
                    if (d < best_edge) {
                        best_edge = d;
                        edge_idx = i;
                    }
                }
                if (best_edge <= pick_radius * context.geometry_policy.edge_insert_multiplier) {
                    const RogueCity::Core::Vec2 midpoint =
                        (selected_water->boundary[edge_idx] + selected_water->boundary[edge_idx + 1]) * 0.5;
                    selected_water->boundary.insert(
                        selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1),
                        midpoint);
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                    return true;
                }
            } else if (selected_water->boundary.size() >= 5) {
                double best = std::numeric_limits<double>::max();
                size_t best_idx = 1;
                for (size_t i = 1; i + 1 < selected_water->boundary.size(); ++i) {
                    const double d = selected_water->boundary[i].distanceTo(context.world_pos);
                    if (d < best) {
                        best = d;
                        best_idx = i;
                    }
                }
                if (best <= pick_radius) {
                    WaterBody split_water{};
                    split_water.id = NextWaterId(context.gs);
                    split_water.type = selected_water->type;
                    split_water.depth = selected_water->depth;
                    split_water.generate_shore = selected_water->generate_shore;
                    split_water.is_user_placed = true;
                    split_water.generation_tag = RogueCity::Core::GenerationTag::M_user;
                    split_water.generation_locked = true;
                    split_water.boundary.assign(
                        selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(best_idx),
                        selected_water->boundary.end());
                    selected_water->boundary.erase(
                        selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(best_idx),
                        selected_water->boundary.end());
                    context.gs.waterbodies.add(std::move(split_water));
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                    return true;
                }
            }
        }

        if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Simplify &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt && !context.io.KeyShift) {
            const bool closed_shape = selected_water->type != WaterType::River;
            if (SimplifyPolyline(selected_water->boundary, closed_shape)) {
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                return true;
            }
        }

        const bool allow_water_vertex_drag =
            (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::DirectSelect ||
             context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents ||
             context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::SnapAlign ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Flow);
        if (!interaction_state.water_vertex_drag.active &&
            allow_water_vertex_drag &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt) {
            if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents &&
                selected_water->boundary.size() >= 3) {
                constexpr double kHandleScale = 3.0;
                for (size_t i = 1; i + 1 < selected_water->boundary.size(); ++i) {
                    const RogueCity::Core::Vec2 anchor = selected_water->boundary[i];
                    const RogueCity::Core::Vec2 prev = selected_water->boundary[i - 1];
                    const RogueCity::Core::Vec2 next = selected_water->boundary[i + 1];
                    RogueCity::Core::Vec2 in_dir = anchor - prev;
                    RogueCity::Core::Vec2 out_dir = anchor - next;
                    if (in_dir.lengthSquared() > 1e-8) {
                        in_dir.normalize();
                        const RogueCity::Core::Vec2 handle = anchor + in_dir * (pick_radius * kHandleScale);
                        if (handle.distanceTo(context.world_pos) <= pick_radius * 1.2) {
                            interaction_state.water_vertex_drag.active = true;
                            interaction_state.water_vertex_drag.water_id = selected_water->id;
                            interaction_state.water_vertex_drag.vertex_index = i;
                            interaction_state.water_vertex_drag.tangent_handle_active = true;
                            interaction_state.water_vertex_drag.tangent_outgoing = false;
                            return true;
                        }
                    }
                    if (out_dir.lengthSquared() > 1e-8) {
                        out_dir.normalize();
                        const RogueCity::Core::Vec2 handle = anchor + out_dir * (pick_radius * kHandleScale);
                        if (handle.distanceTo(context.world_pos) <= pick_radius * 1.2) {
                            interaction_state.water_vertex_drag.active = true;
                            interaction_state.water_vertex_drag.water_id = selected_water->id;
                            interaction_state.water_vertex_drag.vertex_index = i;
                            interaction_state.water_vertex_drag.tangent_handle_active = true;
                            interaction_state.water_vertex_drag.tangent_outgoing = true;
                            return true;
                        }
                    }
                }
            }

            double best = std::numeric_limits<double>::max();
            size_t best_idx = 0;
            for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                const double d = selected_water->boundary[i].distanceTo(context.world_pos);
                if (d < best) {
                    best = d;
                    best_idx = i;
                }
            }
            if (best <= pick_radius) {
                interaction_state.water_vertex_drag.active = true;
                interaction_state.water_vertex_drag.water_id = selected_water->id;
                interaction_state.water_vertex_drag.vertex_index = best_idx;
                interaction_state.water_vertex_drag.tangent_handle_active = false;
                return true;
            }
        }

        if (interaction_state.water_vertex_drag.active &&
            interaction_state.water_vertex_drag.water_id == selected_water->id) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (interaction_state.water_vertex_drag.vertex_index < selected_water->boundary.size()) {
                    RogueCity::Core::Vec2 snapped = context.world_pos;
                    SnapToGrid(
                        snapped,
                        context.gs.district_boundary_editor.snap_to_grid,
                        context.gs.district_boundary_editor.snap_size);

                    const size_t anchor_index = interaction_state.water_vertex_drag.vertex_index;
                    const RogueCity::Core::Vec2 anchor_before = selected_water->boundary[anchor_index];
                    if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::SnapAlign) {
                        ApplyAxisAlign(snapped, anchor_before);
                    }
                    const RogueCity::Core::Vec2 delta = snapped - anchor_before;
                    const bool use_falloff =
                        (context.gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
                         context.gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
                         context.gs.tool_runtime.water_subtool == WaterSubtool::Flow);

                    if (interaction_state.water_vertex_drag.tangent_handle_active &&
                        anchor_index > 0 &&
                        anchor_index + 1 < selected_water->boundary.size()) {
                        const RogueCity::Core::Vec2 tangent = snapped - anchor_before;
                        const size_t neighbor_i = interaction_state.water_vertex_drag.tangent_outgoing
                            ? (anchor_index + 1)
                            : (anchor_index - 1);
                        selected_water->boundary[neighbor_i] = anchor_before - tangent;
                    } else if (use_falloff && context.geometry_policy.water_falloff_radius_world > 0.0) {
                        const bool closed_shape = selected_water->type != WaterType::River;
                        const RogueCity::Core::Vec2 centroid = PolylineCentroid(selected_water->boundary);
                        RogueCity::Core::Vec2 anchor_tangent =
                            TangentAt(selected_water->boundary, anchor_index, closed_shape);
                        if (anchor_tangent.lengthSquared() <= 1e-8) {
                            anchor_tangent = delta;
                        }
                        if (anchor_tangent.lengthSquared() > 1e-8) {
                            anchor_tangent.normalize();
                        }
                        RogueCity::Core::Vec2 anchor_radial = anchor_before - centroid;
                        if (anchor_radial.lengthSquared() > 1e-8) {
                            anchor_radial.normalize();
                        }
                        const double radial_magnitude = delta.dot(anchor_radial);
                        RogueCity::Core::Vec2 contour_delta = delta;
                        if (std::abs(delta.x) >= std::abs(delta.y)) {
                            contour_delta.y = 0.0;
                        } else {
                            contour_delta.x = 0.0;
                        }
                        const double flow_magnitude = delta.dot(anchor_tangent);

                        for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                            const double distance = selected_water->boundary[i].distanceTo(anchor_before);
                            if (distance > context.geometry_policy.water_falloff_radius_world) {
                                continue;
                            }
                            double weight = RC_UI::Tools::ComputeFalloffWeight(
                                distance,
                                context.geometry_policy.water_falloff_radius_world);
                            RogueCity::Core::Vec2 movement = delta;
                            if (context.gs.tool_runtime.water_subtool == WaterSubtool::Erode) {
                                RogueCity::Core::Vec2 radial = selected_water->boundary[i] - centroid;
                                if (radial.lengthSquared() > 1e-8) {
                                    radial.normalize();
                                    movement = radial * radial_magnitude;
                                } else {
                                    movement = delta * 0.5;
                                }
                                weight *= 0.72;
                            } else if (context.gs.tool_runtime.water_subtool == WaterSubtool::Flow) {
                                RogueCity::Core::Vec2 tangent_i =
                                    TangentAt(selected_water->boundary, i, closed_shape);
                                if (tangent_i.lengthSquared() <= 1e-8) {
                                    tangent_i = anchor_tangent;
                                }
                                if (tangent_i.lengthSquared() > 1e-8) {
                                    tangent_i.normalize();
                                    movement = tangent_i * flow_magnitude;
                                }
                                weight *= 0.9;
                            } else if (context.gs.tool_runtime.water_subtool == WaterSubtool::Contour) {
                                movement = contour_delta;
                                weight *= 0.82;
                            }
                            selected_water->boundary[i] += movement * weight;
                        }
                    } else {
                        selected_water->boundary[anchor_index] = snapped;
                    }
                    if (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents) {
                        const bool closed_shape = selected_water->type != WaterType::River;
                        ApplyTinySplinePass(selected_water->boundary, context.gs, closed_shape);
                    }
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                }
                return true;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                interaction_state.water_vertex_drag.active = false;
                interaction_state.water_vertex_drag.tangent_handle_active = false;
                return true;
            }
        }
    }

    return false;
}

} // namespace RC_UI::Viewport::Handlers
