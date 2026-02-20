#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <limits>

namespace RC_UI::Viewport::Handlers {

bool HandleDistrictPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::District;
    using RogueCity::Core::DistrictType;
    using RogueCity::Core::Editor::DistrictSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Districts) {
        return false;
    }

    const bool left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !context.io.KeyAlt;
    const bool add_click = left_click && !context.io.KeyCtrl;
    const bool ctrl_double_click = context.io.KeyCtrl && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    if (left_click &&
        (context.gs.tool_runtime.district_subtool == DistrictSubtool::Select ||
         context.gs.tool_runtime.district_subtool == DistrictSubtool::Inspect)) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::District) {
            SetPrimarySelection(context.gs, VpEntityKind::District, picked->id);
            return true;
        }
    }

    if (add_click &&
        context.gs.tool_runtime.district_subtool == DistrictSubtool::Merge &&
        context.gs.selection.selected_district) {
        const uint32_t primary_id = context.gs.selection.selected_district->id;
        const auto target = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
        if (target.has_value() &&
            target->kind == VpEntityKind::District &&
            target->id != primary_id) {
            RogueCity::Core::Vec2 primary_anchor{};
            RogueCity::Core::Vec2 target_anchor{};
            const bool anchors_ok =
                ResolveSelectionAnchor(context.gs, VpEntityKind::District, primary_id, primary_anchor) &&
                ResolveSelectionAnchor(context.gs, VpEntityKind::District, target->id, target_anchor);
            if (anchors_ok &&
                primary_anchor.distanceTo(target_anchor) <= context.geometry_policy.merge_radius_world) {
                if (RogueCity::Core::District* primary = FindDistrictMutable(context.gs, primary_id);
                    primary != nullptr) {
                    const uint32_t target_id = target->id;
                    if (RogueCity::Core::District* other = FindDistrictMutable(context.gs, target_id);
                        other != nullptr && !other->border.empty()) {
                        primary->border.insert(primary->border.end(), other->border.begin(), other->border.end());
                        for (size_t i = 0; i < context.gs.districts.size(); ++i) {
                            auto handle = context.gs.districts.createHandleFromData(i);
                            if (handle && handle->id == target_id) {
                                context.gs.districts.remove(handle);
                                break;
                            }
                        }
                        SetPrimarySelection(context.gs, VpEntityKind::District, primary_id);
                        MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                        return true;
                    }
                }
            }
        }
    }

    if (add_click &&
        context.gs.tool_runtime.district_subtool == DistrictSubtool::Split &&
        context.gs.selection.selected_district) {
        RogueCity::Core::District* district = FindDistrictMutable(
            context.gs,
            context.gs.selection.selected_district->id);
        if (district != nullptr && district->border.size() >= 3) {
            double min_x = district->border.front().x;
            double max_x = district->border.front().x;
            double min_y = district->border.front().y;
            double max_y = district->border.front().y;
            for (const auto& p : district->border) {
                min_x = std::min(min_x, p.x);
                max_x = std::max(max_x, p.x);
                min_y = std::min(min_y, p.y);
                max_y = std::max(max_y, p.y);
            }
            const RogueCity::Core::Vec2 center((min_x + max_x) * 0.5, (min_y + max_y) * 0.5);
            const bool vertical_split =
                std::abs(context.world_pos.x - center.x) >= std::abs(context.world_pos.y - center.y);

            District new_district = *district;
            new_district.id = NextDistrictId(context.gs);
            new_district.is_user_placed = true;
            new_district.generation_tag = RogueCity::Core::GenerationTag::M_user;
            new_district.generation_locked = true;

            if (vertical_split) {
                const double width = max_x - min_x;
                const double split_x = (width <= 2.0)
                    ? (min_x + max_x) * 0.5
                    : std::clamp(context.world_pos.x, min_x + 1.0, max_x - 1.0);
                district->border = {
                    {min_x, min_y}, {split_x, min_y}, {split_x, max_y}, {min_x, max_y}
                };
                new_district.border = {
                    {split_x, min_y}, {max_x, min_y}, {max_x, max_y}, {split_x, max_y}
                };
            } else {
                const double height = max_y - min_y;
                const double split_y = (height <= 2.0)
                    ? (min_y + max_y) * 0.5
                    : std::clamp(context.world_pos.y, min_y + 1.0, max_y - 1.0);
                district->border = {
                    {min_x, min_y}, {max_x, min_y}, {max_x, split_y}, {min_x, split_y}
                };
                new_district.border = {
                    {min_x, split_y}, {max_x, split_y}, {max_x, max_y}, {min_x, max_y}
                };
            }
            context.gs.districts.add(std::move(new_district));
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
            return true;
        }
    }

    if ((context.gs.tool_runtime.district_subtool == DistrictSubtool::Paint ||
         context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone) &&
        (add_click || (ctrl_double_click && context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone))) {
        if (context.gs.tool_runtime.district_subtool == DistrictSubtool::Paint &&
            context.gs.selection.selected_district) {
            RogueCity::Core::District* district = FindDistrictMutable(
                context.gs,
                context.gs.selection.selected_district->id);
            if (district != nullptr) {
                district->is_user_placed = true;
                district->generation_tag = RogueCity::Core::GenerationTag::M_user;
                district->generation_locked = true;
                if (district->border.empty() ||
                    district->border.back().distanceTo(context.world_pos) >= context.interaction_metrics.world_lasso_step) {
                    district->border.push_back(context.world_pos);
                    MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                    return true;
                }
            }
        } else if (context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone) {
            RogueCity::Core::District* district = context.gs.selection.selected_district
                ? FindDistrictMutable(context.gs, context.gs.selection.selected_district->id)
                : nullptr;
            if (district == nullptr) {
                District new_district{};
                new_district.id = NextDistrictId(context.gs);
                new_district.type = DistrictType::Residential;
                new_district.border.push_back(context.world_pos);
                new_district.orientation = {1.0, 0.0};
                new_district.is_user_placed = true;
                new_district.generation_tag = RogueCity::Core::GenerationTag::M_user;
                new_district.generation_locked = true;
                context.gs.districts.add(std::move(new_district));
                const uint32_t new_id = NextDistrictId(context.gs) - 1u;
                SetPrimarySelection(context.gs, VpEntityKind::District, new_id);
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                return true;
            }

            district->is_user_placed = true;
            district->generation_tag = RogueCity::Core::GenerationTag::M_user;
            district->generation_locked = true;

            if (ctrl_double_click && district->border.size() >= 3) {
                if (district->border.back().distanceTo(district->border.front()) > 1e-3) {
                    district->border.push_back(district->border.front());
                }
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                return true;
            }

            const double append_step = std::max(0.25, context.interaction_metrics.world_lasso_step * 0.5);
            if (district->border.empty() ||
                district->border.back().distanceTo(context.world_pos) >= append_step) {
                district->border.push_back(context.world_pos);
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                return true;
            }
        } else {
            District district{};
            district.id = NextDistrictId(context.gs);
            district.type = (context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone)
                ? DistrictType::Residential
                : DistrictType::Mixed;
            district.border = MakeSquareBoundary(context.world_pos, context.geometry_policy.district_placement_half_extent);
            district.orientation = {1.0, 0.0};
            district.is_user_placed = true;
            district.generation_tag = RogueCity::Core::GenerationTag::M_user;
            district.generation_locked = true;
            context.gs.districts.add(std::move(district));
            const uint32_t new_id = NextDistrictId(context.gs) - 1u;
            SetPrimarySelection(context.gs, VpEntityKind::District, new_id);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
            return true;
        }
    }

    if ((context.gs.tool_runtime.district_subtool == DistrictSubtool::Paint ||
         context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone) &&
        context.gs.selection.selected_district &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl) {
        RogueCity::Core::District* district = FindDistrictMutable(
            context.gs,
            context.gs.selection.selected_district->id);
        if (district != nullptr &&
            (district->border.empty() ||
             district->border.back().distanceTo(context.world_pos) >=
                 std::max(0.25, context.interaction_metrics.world_lasso_step *
                     (context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone ? 0.55 : 1.0)))) {
            district->border.push_back(context.world_pos);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
            return true;
        }
    }

    return false;
}

bool HandleDistrictVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::Editor::DistrictSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Districts || !context.gs.selection.selected_district) {
        return false;
    }

    const uint32_t district_id = context.gs.selection.selected_district->id;
    RogueCity::Core::District* district = FindDistrictMutable(context.gs, district_id);
    if (district == nullptr || district->border.size() < 3) {
        return false;
    }

    bool consumed_interaction = false;
    const bool precision_pick_mode = context.io.KeyShift && context.io.KeyCtrl;
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius *
        (precision_pick_mode ? 0.65 : 1.35);

    if (!interaction_state.district_boundary_drag.active &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt) {
        double best = std::numeric_limits<double>::max();
        size_t best_idx = 0;
        for (size_t i = 0; i < district->border.size(); ++i) {
            const double d = district->border[i].distanceTo(context.world_pos);
            if (d < best) {
                best = d;
                best_idx = i;
            }
        }

        const bool remove_vertex = context.gs.district_boundary_editor.delete_mode;
        const bool insert_vertex =
            context.gs.tool_runtime.district_subtool == DistrictSubtool::Split ||
            context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone ||
            context.gs.district_boundary_editor.insert_mode;

        if (remove_vertex && best <= pick_radius) {
            RogueCity::App::EditorManipulation::RemoveDistrictVertex(*district, best_idx);
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
            consumed_interaction = true;
        } else if (insert_vertex) {
            double best_edge = std::numeric_limits<double>::max();
            size_t edge_idx = 0;
            for (size_t i = 0; i < district->border.size(); ++i) {
                const auto& a = district->border[i];
                const auto& b = district->border[(i + 1) % district->border.size()];
                const double d = DistanceToSegment(context.world_pos, a, b);
                if (d < best_edge) {
                    best_edge = d;
                    edge_idx = i;
                }
            }
            if (best_edge <= pick_radius * context.geometry_policy.edge_insert_multiplier) {
                RogueCity::App::EditorManipulation::InsertDistrictVertex(*district, edge_idx, context.world_pos);
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
                consumed_interaction = true;
            }
        } else if (best <= pick_radius) {
            interaction_state.district_boundary_drag.active = true;
            interaction_state.district_boundary_drag.district_id = district_id;
            interaction_state.district_boundary_drag.vertex_index = best_idx;
            consumed_interaction = true;
        }
    }

    if (interaction_state.district_boundary_drag.active) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            RogueCity::Core::Vec2 snapped = context.world_pos;
            SnapToGrid(
                snapped,
                context.gs.district_boundary_editor.snap_to_grid,
                context.gs.district_boundary_editor.snap_size);
            if (interaction_state.district_boundary_drag.vertex_index < district->border.size()) {
                district->border[interaction_state.district_boundary_drag.vertex_index] = snapped;
                MarkDirtyForSelectionKind(context.gs, VpEntityKind::District);
            }
            consumed_interaction = true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            interaction_state.district_boundary_drag.active = false;
            consumed_interaction = true;
        }
    }

    return consumed_interaction;
}

} // namespace RC_UI::Viewport::Handlers
