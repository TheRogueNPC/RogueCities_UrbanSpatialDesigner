#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

#include <algorithm>
#include <cmath>

namespace RC_UI::Viewport::Handlers {
namespace {

RogueCity::Core::LotToken* ResolveEditableLotTarget(
    ViewportInteractionContext& context,
    bool allow_pick_from_cursor) {
    if (context.gs.selection.selected_lot) {
        if (auto* lot = FindLotMutable(context.gs, context.gs.selection.selected_lot->id);
            lot != nullptr) {
            return lot;
        }
    }

    if (!allow_pick_from_cursor) {
        return nullptr;
    }

    const auto picked = PickFromViewportIndex(
        context.gs,
        context.world_pos,
        context.interaction_metrics,
        context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
    if (!picked.has_value() || picked->kind != RogueCity::Core::Editor::VpEntityKind::Lot) {
        return nullptr;
    }
    SetPrimarySelection(context.gs, RogueCity::Core::Editor::VpEntityKind::Lot, picked->id);
    return FindLotMutable(context.gs, picked->id);
}

} // namespace

bool HandleLotPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    (void)interaction_state;

    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::LotSubtool;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::LotToken;

    if (context.editor_state != EditorState::Editing_Lots) {
        return false;
    }

    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !context.io.KeyAlt && !context.io.KeyShift && !context.io.KeyCtrl;

    if (add_click && context.gs.tool_runtime.lot_subtool == LotSubtool::Plot) {
        LotToken lot{};
        lot.id = NextLotId(context.gs);
        lot.district_id = context.gs.selection.selected_district ? context.gs.selection.selected_district->id : 0u;
        lot.centroid = context.world_pos;
        lot.boundary = MakeSquareBoundary(context.world_pos, context.geometry_policy.lot_placement_half_extent);
        lot.area = static_cast<float>(
            (context.geometry_policy.lot_placement_half_extent * 2.0) *
            (context.geometry_policy.lot_placement_half_extent * 2.0));
        lot.is_user_placed = true;
        lot.generation_tag = RogueCity::Core::GenerationTag::M_user;
        lot.generation_locked = true;
        context.gs.lots.add(std::move(lot));
        const uint32_t new_id = NextLotId(context.gs) - 1u;
        SetPrimarySelection(context.gs, VpEntityKind::Lot, new_id);
        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Lot);
        return true;
    }

    if (add_click &&
        context.gs.tool_runtime.lot_subtool == LotSubtool::Merge &&
        context.gs.selection.selected_lot) {
        const uint32_t primary_id = context.gs.selection.selected_lot->id;
        const auto target = PickFromViewportIndex(context.gs, context.world_pos, context.interaction_metrics);
        if (target.has_value() &&
            target->kind == VpEntityKind::Lot &&
            target->id != primary_id) {
            RogueCity::Core::Vec2 primary_anchor{};
            RogueCity::Core::Vec2 target_anchor{};
            const bool anchors_ok =
                ResolveSelectionAnchor(context.gs, VpEntityKind::Lot, primary_id, primary_anchor) &&
                ResolveSelectionAnchor(context.gs, VpEntityKind::Lot, target->id, target_anchor);
            if (anchors_ok &&
                primary_anchor.distanceTo(target_anchor) <= context.geometry_policy.merge_radius_world) {
                const uint32_t target_id = target->id;
                for (size_t i = 0; i < context.gs.lots.size(); ++i) {
                    auto h_primary = context.gs.lots.createHandleFromData(i);
                    if (!h_primary || h_primary->id != primary_id) {
                        continue;
                    }
                    for (size_t j = 0; j < context.gs.lots.size(); ++j) {
                        auto h_other = context.gs.lots.createHandleFromData(j);
                        if (!h_other || h_other->id != target_id) {
                            continue;
                        }
                        h_primary->boundary.insert(h_primary->boundary.end(), h_other->boundary.begin(), h_other->boundary.end());
                        h_primary->area += h_other->area;
                        h_primary->centroid = (h_primary->centroid + h_other->centroid) * 0.5;
                        context.gs.lots.remove(h_other);
                        SetPrimarySelection(context.gs, VpEntityKind::Lot, primary_id);
                        MarkDirtyForSelectionKind(context.gs, VpEntityKind::Lot);
                        return true;
                    }
                    break;
                }
            }
        }
    } else if (add_click &&
               context.gs.tool_runtime.lot_subtool == LotSubtool::Merge &&
               !context.gs.selection.selected_lot) {
        const auto picked = PickFromViewportIndex(
            context.gs,
            context.world_pos,
            context.interaction_metrics,
            context.io.KeyShift && context.io.KeyCtrl ? 0.6 : 1.6);
        if (picked.has_value() && picked->kind == VpEntityKind::Lot) {
            SetPrimarySelection(context.gs, VpEntityKind::Lot, picked->id);
            return true;
        }
    }

    if (add_click &&
        context.gs.tool_runtime.lot_subtool == LotSubtool::Slice) {
        LotToken* lot = ResolveEditableLotTarget(context, true);
        if (lot != nullptr) {
            RogueCity::Core::Vec2 center = lot->centroid;
            if (!lot->boundary.empty()) {
                center = PolygonCentroid(lot->boundary);
            }

            double min_x = center.x - context.geometry_policy.lot_placement_half_extent;
            double max_x = center.x + context.geometry_policy.lot_placement_half_extent;
            double min_y = center.y - context.geometry_policy.lot_placement_half_extent;
            double max_y = center.y + context.geometry_policy.lot_placement_half_extent;
            if (!lot->boundary.empty()) {
                min_x = max_x = lot->boundary.front().x;
                min_y = max_y = lot->boundary.front().y;
                for (const auto& p : lot->boundary) {
                    min_x = std::min(min_x, p.x);
                    max_x = std::max(max_x, p.x);
                    min_y = std::min(min_y, p.y);
                    max_y = std::max(max_y, p.y);
                }
            }

            const bool vertical_split =
                std::abs(context.world_pos.x - center.x) >= std::abs(context.world_pos.y - center.y);
            LotToken new_lot = *lot;
            new_lot.id = NextLotId(context.gs);
            new_lot.is_user_placed = true;
            new_lot.generation_tag = RogueCity::Core::GenerationTag::M_user;
            new_lot.generation_locked = true;

            if (vertical_split) {
                const double width = max_x - min_x;
                const double split_x = (width <= 2.0)
                    ? (min_x + max_x) * 0.5
                    : std::clamp(context.world_pos.x, min_x + 1.0, max_x - 1.0);
                lot->boundary = {
                    {min_x, min_y}, {split_x, min_y}, {split_x, max_y}, {min_x, max_y}
                };
                new_lot.boundary = {
                    {split_x, min_y}, {max_x, min_y}, {max_x, max_y}, {split_x, max_y}
                };
            } else {
                const double height = max_y - min_y;
                const double split_y = (height <= 2.0)
                    ? (min_y + max_y) * 0.5
                    : std::clamp(context.world_pos.y, min_y + 1.0, max_y - 1.0);
                lot->boundary = {
                    {min_x, min_y}, {max_x, min_y}, {max_x, split_y}, {min_x, split_y}
                };
                new_lot.boundary = {
                    {min_x, split_y}, {max_x, split_y}, {max_x, max_y}, {min_x, max_y}
                };
            }

            lot->centroid = PolygonCentroid(lot->boundary);
            new_lot.centroid = PolygonCentroid(new_lot.boundary);
            auto rect_area = [](const std::vector<RogueCity::Core::Vec2>& boundary) {
                if (boundary.size() < 4) {
                    return 0.0f;
                }
                const double w = std::abs(boundary[1].x - boundary[0].x);
                const double h = std::abs(boundary[3].y - boundary[0].y);
                return static_cast<float>(w * h);
            };
            lot->area = rect_area(lot->boundary);
            new_lot.area = rect_area(new_lot.boundary);
            context.gs.lots.add(std::move(new_lot));
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Lot);
            return true;
        }
    }

    if (add_click &&
        context.gs.tool_runtime.lot_subtool == LotSubtool::Align) {
        LotToken* lot = ResolveEditableLotTarget(context, true);
        if (lot != nullptr) {
            RogueCity::Core::Vec2 center = lot->boundary.empty() ? lot->centroid : PolygonCentroid(lot->boundary);
            SnapToGrid(
                center,
                context.gs.district_boundary_editor.snap_to_grid,
                context.gs.district_boundary_editor.snap_size);

            double half_extent = context.geometry_policy.lot_placement_half_extent;
            if (lot->area > 1.0f) {
                half_extent = std::max(6.0, std::sqrt(static_cast<double>(lot->area)) * 0.5);
            }
            lot->boundary = MakeSquareBoundary(center, half_extent);
            lot->centroid = center;
            lot->area = static_cast<float>((half_extent * 2.0) * (half_extent * 2.0));
            MarkDirtyForSelectionKind(context.gs, VpEntityKind::Lot);
            return true;
        }
    }

    return false;
}

} // namespace RC_UI::Viewport::Handlers
