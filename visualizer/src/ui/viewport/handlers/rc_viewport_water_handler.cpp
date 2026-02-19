#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace RC_UI::Viewport::Handlers {

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

    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl;

    const auto* primary = context.gs.selection_manager.Primary();
    uint32_t selected_water_id =
        (primary != nullptr && primary->kind == VpEntityKind::Water) ? primary->id : 0u;
    WaterBody* selected_water =
        selected_water_id != 0u ? FindWaterMutable(context.gs, selected_water_id) : nullptr;
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius;

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
            (context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Selection ||
             context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::DirectSelect ||
             context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents ||
             context.gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::SnapAlign ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
             context.gs.tool_runtime.water_subtool == WaterSubtool::Flow);
        if (!interaction_state.water_vertex_drag.active &&
            allow_water_vertex_drag &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !context.io.KeyAlt) {
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

                    if (use_falloff && context.geometry_policy.water_falloff_radius_world > 0.0) {
                        for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                            const double distance = selected_water->boundary[i].distanceTo(anchor_before);
                            if (distance > context.geometry_policy.water_falloff_radius_world) {
                                continue;
                            }
                            double weight = RC_UI::Tools::ComputeFalloffWeight(
                                distance,
                                context.geometry_policy.water_falloff_radius_world);
                            if (context.gs.tool_runtime.water_subtool == WaterSubtool::Erode) {
                                weight *= 0.72;
                            } else if (context.gs.tool_runtime.water_subtool == WaterSubtool::Flow) {
                                weight *= 0.9;
                            }
                            selected_water->boundary[i] += delta * weight;
                        }
                    } else {
                        selected_water->boundary[anchor_index] = snapped;
                    }
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::Water);
                }
                return true;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                interaction_state.water_vertex_drag.active = false;
                return true;
            }
        }
    }

    return false;
}

} // namespace RC_UI::Viewport::Handlers
