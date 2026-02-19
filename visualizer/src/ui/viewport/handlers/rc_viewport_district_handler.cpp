#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"

#include <algorithm>
#include <limits>

namespace RC_UI::Viewport::Handlers {

bool HandleDistrictPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    (void)interaction_state;

    using RogueCity::Core::District;
    using RogueCity::Core::DistrictType;
    using RogueCity::Core::Editor::DistrictSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::VpEntityKind;

    if (context.editor_state != EditorState::Editing_Districts) {
        return false;
    }

    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl;

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
        (context.gs.tool_runtime.district_subtool == DistrictSubtool::Paint ||
         context.gs.tool_runtime.district_subtool == DistrictSubtool::Zone)) {
        District district{};
        district.id = NextDistrictId(context.gs);
        district.type = DistrictType::Mixed;
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
    const double pick_radius = context.interaction_metrics.world_vertex_pick_radius;

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
