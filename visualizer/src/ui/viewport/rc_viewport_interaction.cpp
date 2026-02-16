#include "ui/viewport/rc_viewport_interaction.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <unordered_set>
#include <vector>

namespace RC_UI::Viewport {
namespace {

void RequestDefaultContextCommandMenu(
    const RogueCity::Core::Editor::EditorConfig& config,
    const ImVec2& screen_pos,
    const CommandMenuStateBundle& state_bundle) {
    using RogueCity::Core::Editor::ViewportCommandMode;
    switch (config.viewport_context_default_mode) {
        case ViewportCommandMode::SmartList:
            if (state_bundle.smart_menu != nullptr) {
                RC_UI::Commands::RequestOpenSmartMenu(*state_bundle.smart_menu, screen_pos);
            }
            break;
        case ViewportCommandMode::Pie:
            if (state_bundle.pie_menu != nullptr) {
                RC_UI::Commands::RequestOpenPieMenu(*state_bundle.pie_menu, screen_pos);
            }
            break;
        case ViewportCommandMode::Palette:
        default:
            if (state_bundle.command_palette != nullptr) {
                RC_UI::Commands::RequestOpenCommandPalette(*state_bundle.command_palette);
            }
            break;
    }
}

uint64_t SelectionKey(const RogueCity::Core::Editor::SelectionItem& item) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) | static_cast<uint64_t>(item.id);
}

double DistanceToSegment(const RogueCity::Core::Vec2& p, const RogueCity::Core::Vec2& a, const RogueCity::Core::Vec2& b) {
    const RogueCity::Core::Vec2 ab = b - a;
    const double ab_len_sq = ab.lengthSquared();
    if (ab_len_sq <= 1e-9) {
        return p.distanceTo(a);
    }
    const double t = std::clamp((p - a).dot(ab) / ab_len_sq, 0.0, 1.0);
    const RogueCity::Core::Vec2 proj = a + ab * t;
    return p.distanceTo(proj);
}

bool PointInPolygon(const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& pi = polygon[i];
        const auto& pj = polygon[j];
        const bool intersect = ((pi.y > point.y) != (pj.y > point.y)) &&
            (point.x < (pj.x - pi.x) * (point.y - pi.y) / ((pj.y - pi.y) + 1e-12) + pi.x);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

RogueCity::Core::Vec2 PolygonCentroid(const std::vector<RogueCity::Core::Vec2>& points) {
    RogueCity::Core::Vec2 centroid{};
    if (points.empty()) {
        return centroid;
    }
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<double>(points.size());
    return centroid;
}

bool IsSelectableKind(RogueCity::Core::Editor::VpEntityKind kind) {
    using RogueCity::Core::Editor::VpEntityKind;
    return kind == VpEntityKind::Road ||
        kind == VpEntityKind::District ||
        kind == VpEntityKind::Lot ||
        kind == VpEntityKind::Building ||
        kind == VpEntityKind::Water;
}

bool ResolveSelectionAnchor(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id,
    RogueCity::Core::Vec2& out_anchor) {
    using RogueCity::Core::Editor::VpEntityKind;

    switch (kind) {
    case VpEntityKind::Road:
        for (const auto& road : gs.roads) {
            if (road.id == id && !road.points.empty()) {
                out_anchor = road.points[road.points.size() / 2];
                return true;
            }
        }
        return false;
    case VpEntityKind::District:
        for (const auto& district : gs.districts) {
            if (district.id == id && !district.border.empty()) {
                out_anchor = PolygonCentroid(district.border);
                return true;
            }
        }
        return false;
    case VpEntityKind::Lot:
        for (const auto& lot : gs.lots) {
            if (lot.id == id) {
                out_anchor = lot.boundary.empty() ? lot.centroid : PolygonCentroid(lot.boundary);
                return true;
            }
        }
        return false;
    case VpEntityKind::Building:
        for (const auto& building : gs.buildings) {
            if (building.id == id) {
                out_anchor = building.position;
                return true;
            }
        }
        return false;
    case VpEntityKind::Water:
        for (const auto& water : gs.waterbodies) {
            if (water.id == id && !water.boundary.empty()) {
                out_anchor = PolygonCentroid(water.boundary);
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

bool ProbeContainsPoint(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id,
    const RogueCity::Core::Vec2& world_pos,
    double world_radius,
    int& out_priority,
    double& out_distance) {
    using RogueCity::Core::Editor::VpEntityKind;

    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    if (!IsSelectableKind(kind)) {
        return false;
    }

    if (kind == VpEntityKind::Road) {
        for (const auto& road : gs.roads) {
            if (road.id != id || road.points.size() < 2) {
                continue;
            }
            double min_distance = std::numeric_limits<double>::max();
            for (size_t i = 1; i < road.points.size(); ++i) {
                min_distance = std::min(min_distance, DistanceToSegment(world_pos, road.points[i - 1], road.points[i]));
            }
            if (min_distance <= world_radius * 1.25) {
                out_priority = 3;
                out_distance = min_distance;
                return true;
            }
            return false;
        }
        return false;
    }

    if (kind == VpEntityKind::District) {
        for (const auto& district : gs.districts) {
            if (district.id != id || district.border.empty()) {
                continue;
            }
            if (!PointInPolygon(world_pos, district.border)) {
                return false;
            }
            out_priority = 2;
            out_distance = world_pos.distanceTo(PolygonCentroid(district.border));
            return true;
        }
        return false;
    }

    if (kind == VpEntityKind::Lot) {
        for (const auto& lot : gs.lots) {
            if (lot.id != id) {
                continue;
            }
            if (!lot.boundary.empty() && PointInPolygon(world_pos, lot.boundary)) {
                out_priority = 4;
                out_distance = world_pos.distanceTo(lot.centroid);
                return true;
            }
            const double d = world_pos.distanceTo(lot.centroid);
            if (d <= world_radius * 2.0) {
                out_priority = 2;
                out_distance = d;
                return true;
            }
            return false;
        }
        return false;
    }

    if (kind == VpEntityKind::Building) {
        for (const auto& building : gs.buildings) {
            if (building.id != id) {
                continue;
            }
            const double d = world_pos.distanceTo(building.position);
            if (d <= world_radius * 1.75) {
                out_priority = 5;
                out_distance = d;
                return true;
            }
            return false;
        }
        return false;
    }

    for (const auto& water : gs.waterbodies) {
        if (water.id != id || water.boundary.size() < 2) {
            continue;
        }
        double min_distance = std::numeric_limits<double>::max();
        for (size_t i = 1; i < water.boundary.size(); ++i) {
            min_distance = std::min(min_distance, DistanceToSegment(world_pos, water.boundary[i - 1], water.boundary[i]));
        }
        if (water.boundary.size() >= 3 && PointInPolygon(world_pos, water.boundary)) {
            out_priority = 4;
            out_distance = 0.0;
            return true;
        }
        if (min_distance <= world_radius * 1.5) {
            out_priority = 3;
            out_distance = min_distance;
            return true;
        }
        return false;
    }
    return false;
}

std::optional<RogueCity::Core::Editor::SelectionItem> PickFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_pos,
    float zoom) {
    const double world_radius = std::max(4.0, 12.0 / std::max(0.1f, zoom));
    int best_priority = -1;
    double best_distance = std::numeric_limits<double>::max();
    std::optional<RogueCity::Core::Editor::SelectionItem> best{};

    for (const auto& probe : gs.viewport_index) {
        if (!IsSelectableKind(probe.kind)) {
            continue;
        }
        if (!gs.IsEntityVisible(probe.kind, probe.id)) {
            continue;
        }
        int priority = 0;
        double distance = 0.0;
        if (!ProbeContainsPoint(gs, probe.kind, probe.id, world_pos, world_radius, priority, distance)) {
            continue;
        }
        if (priority > best_priority || (priority == best_priority && distance < best_distance)) {
            best_priority = priority;
            best_distance = distance;
            best = RogueCity::Core::Editor::SelectionItem{ probe.kind, probe.id };
        }
    }

    return best;
}

std::vector<RogueCity::Core::Editor::SelectionItem> QueryRegionFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState& gs,
    const std::function<bool(const RogueCity::Core::Vec2&)>& include_point,
    bool include_hidden = false) {
    std::vector<RogueCity::Core::Editor::SelectionItem> results;
    std::unordered_set<uint64_t> dedupe;
    results.reserve(gs.viewport_index.size());

    for (const auto& probe : gs.viewport_index) {
        if (!IsSelectableKind(probe.kind)) {
            continue;
        }
        if (!include_hidden && !gs.IsEntityVisible(probe.kind, probe.id)) {
            continue;
        }

        RogueCity::Core::Vec2 anchor{};
        if (!ResolveSelectionAnchor(gs, probe.kind, probe.id, anchor)) {
            continue;
        }
        if (!include_point(anchor)) {
            continue;
        }

        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(probe.kind)) << 32) | probe.id;
        if (!dedupe.insert(key).second) {
            continue;
        }
        results.push_back({ probe.kind, probe.id });
    }

    return results;
}

RogueCity::Core::Vec2 ComputeSelectionPivot(const RogueCity::Core::Editor::GlobalState& gs) {
    RogueCity::Core::Vec2 pivot{};
    size_t count = 0;
    for (const auto& item : gs.selection_manager.Items()) {
        RogueCity::Core::Vec2 anchor{};
        if (!ResolveSelectionAnchor(gs, item.kind, item.id, anchor)) {
            continue;
        }
        pivot += anchor;
        ++count;
    }
    if (count == 0) {
        return pivot;
    }
    pivot /= static_cast<double>(count);
    return pivot;
}

void MarkDirtyForSelectionKind(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind) {
    using RogueCity::Core::Editor::DirtyLayer;
    using RogueCity::Core::Editor::VpEntityKind;
    switch (kind) {
    case VpEntityKind::Road:
        gs.dirty_layers.MarkDirty(DirtyLayer::Roads);
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::District:
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::Lot:
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::Building:
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::Water:
        break;
    default:
        break;
    }
    gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
}

RogueCity::Core::Road* FindRoadMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& road : gs.roads) {
        if (road.id == id) {
            return &road;
        }
    }
    return nullptr;
}

RogueCity::Core::District* FindDistrictMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& district : gs.districts) {
        if (district.id == id) {
            return &district;
        }
    }
    return nullptr;
}

RogueCity::Core::WaterBody* FindWaterMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& water : gs.waterbodies) {
        if (water.id == id) {
            return &water;
        }
    }
    return nullptr;
}

uint32_t NextRoadId(const RogueCity::Core::Editor::GlobalState& gs) {
    uint32_t max_id = 0;
    for (const auto& road : gs.roads) {
        max_id = std::max(max_id, road.id);
    }
    return max_id + 1u;
}

uint32_t NextDistrictId(const RogueCity::Core::Editor::GlobalState& gs) {
    uint32_t max_id = 0;
    for (const auto& district : gs.districts) {
        max_id = std::max(max_id, district.id);
    }
    return max_id + 1u;
}

uint32_t NextLotId(const RogueCity::Core::Editor::GlobalState& gs) {
    uint32_t max_id = 0;
    for (const auto& lot : gs.lots) {
        max_id = std::max(max_id, lot.id);
    }
    return max_id + 1u;
}

uint32_t NextBuildingId(const RogueCity::Core::Editor::GlobalState& gs) {
    uint32_t max_id = 0;
    for (const auto& building : gs.buildings) {
        max_id = std::max(max_id, building.id);
    }
    return max_id + 1u;
}

uint32_t NextWaterId(const RogueCity::Core::Editor::GlobalState& gs) {
    uint32_t max_id = 0;
    for (const auto& water : gs.waterbodies) {
        max_id = std::max(max_id, water.id);
    }
    return max_id + 1u;
}

std::vector<RogueCity::Core::Vec2> MakeSquareBoundary(const RogueCity::Core::Vec2& center, double half_extent) {
    return {
        {center.x - half_extent, center.y - half_extent},
        {center.x + half_extent, center.y - half_extent},
        {center.x + half_extent, center.y + half_extent},
        {center.x - half_extent, center.y + half_extent}
    };
}

void SetPrimarySelection(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id) {
    gs.selection_manager.Select(kind, id);
    RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
}

void ApplyToolPreferredGizmoOperation(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::EditorState editor_state) {
    using RogueCity::Core::Editor::BuildingSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::GizmoOperation;
    using RogueCity::Core::Editor::LotSubtool;

    if (editor_state == EditorState::Editing_Buildings) {
        if (gs.tool_runtime.building_subtool == BuildingSubtool::Rotate) {
            gs.gizmo.operation = GizmoOperation::Rotate;
        } else if (gs.tool_runtime.building_subtool == BuildingSubtool::Scale) {
            gs.gizmo.operation = GizmoOperation::Scale;
        } else {
            gs.gizmo.operation = GizmoOperation::Translate;
        }
    } else if (editor_state == EditorState::Editing_Lots) {
        if (gs.tool_runtime.lot_subtool == LotSubtool::Align) {
            gs.gizmo.operation = GizmoOperation::Rotate;
        } else {
            gs.gizmo.operation = GizmoOperation::Translate;
        }
    }
}

bool HandleDomainPlacementActions(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::EditorState editor_state,
    const RogueCity::Core::Vec2& world_pos,
    NonAxiomInteractionState& interaction_state) {
    using RogueCity::Core::BuildingSite;
    using RogueCity::Core::BuildingType;
    using RogueCity::Core::District;
    using RogueCity::Core::DistrictType;
    using RogueCity::Core::Editor::BuildingSubtool;
    using RogueCity::Core::Editor::DistrictSubtool;
    using RogueCity::Core::Editor::EditorState;
    using RogueCity::Core::Editor::LotSubtool;
    using RogueCity::Core::Editor::RoadSplineSubtool;
    using RogueCity::Core::Editor::RoadSubtool;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::Editor::WaterSplineSubtool;
    using RogueCity::Core::LotToken;
    using RogueCity::Core::Road;
    using RogueCity::Core::RoadType;
    using RogueCity::Core::WaterBody;
    using RogueCity::Core::WaterType;

    const ImGuiIO& io = ImGui::GetIO();
    const bool add_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt && !io.KeyShift && !io.KeyCtrl;

    if (editor_state == EditorState::Editing_Water) {
        const auto* primary = gs.selection_manager.Primary();
        uint32_t selected_water_id = (primary != nullptr && primary->kind == VpEntityKind::Water) ? primary->id : 0u;
        WaterBody* selected_water = selected_water_id != 0u ? FindWaterMutable(gs, selected_water_id) : nullptr;
        const double pick_radius = std::max(4.0, 10.0 / std::max(0.1f, gs.params.viewport_pan_speed));

        if (add_click && gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Pen) {
            if (selected_water == nullptr) {
                WaterBody water{};
                water.id = NextWaterId(gs);
                water.type = WaterType::River;
                water.depth = 6.0f;
                water.generate_shore = true;
                water.is_user_placed = true;
                water.boundary.push_back(world_pos);
                gs.waterbodies.add(std::move(water));
                SetPrimarySelection(gs, VpEntityKind::Water, NextWaterId(gs) - 1u);
                MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
            } else {
                selected_water->boundary.push_back(world_pos);
                MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
            }
            return true;
        }

        if (selected_water != nullptr && !selected_water->boundary.empty()) {
            if (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::AddRemoveAnchor &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift) {
                if (io.KeyCtrl && selected_water->boundary.size() > 2) {
                    double best = std::numeric_limits<double>::max();
                    size_t best_idx = 0;
                    for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                        const double d = selected_water->boundary[i].distanceTo(world_pos);
                        if (d < best) {
                            best = d;
                            best_idx = i;
                        }
                    }
                    if (best <= pick_radius) {
                        selected_water->boundary.erase(
                            selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(best_idx));
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                        return true;
                    }
                } else {
                    double best_edge = std::numeric_limits<double>::max();
                    size_t edge_idx = 0;
                    for (size_t i = 0; i + 1 < selected_water->boundary.size(); ++i) {
                        const double d = DistanceToSegment(
                            world_pos,
                            selected_water->boundary[i],
                            selected_water->boundary[i + 1]);
                        if (d < best_edge) {
                            best_edge = d;
                            edge_idx = i;
                        }
                    }
                    if (best_edge <= pick_radius * 1.5) {
                        selected_water->boundary.insert(
                            selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1),
                            world_pos);
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                        return true;
                    }
                }
            }

            if (!interaction_state.water_vertex_drag.active &&
                (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Selection ||
                 gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::DirectSelect ||
                 gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt) {
                double best = std::numeric_limits<double>::max();
                size_t best_idx = 0;
                for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                    const double d = selected_water->boundary[i].distanceTo(world_pos);
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
        }
    }

    if (editor_state == EditorState::Editing_Roads && add_click && gs.tool_runtime.road_spline_subtool == RoadSplineSubtool::Pen) {
        auto* road = gs.selection.selected_road ? FindRoadMutable(gs, gs.selection.selected_road->id) : nullptr;
        if (road == nullptr) {
            Road new_road{};
            new_road.id = NextRoadId(gs);
            new_road.type = (gs.tool_runtime.road_subtool == RoadSubtool::Bridge) ? RoadType::M_Major : RoadType::M_Minor;
            new_road.is_user_created = true;
            new_road.points.push_back(world_pos);
            gs.roads.add(std::move(new_road));
            const uint32_t new_id = NextRoadId(gs) - 1u;
            SetPrimarySelection(gs, VpEntityKind::Road, new_id);
            MarkDirtyForSelectionKind(gs, VpEntityKind::Road);
        } else {
            road->points.push_back(world_pos);
            MarkDirtyForSelectionKind(gs, VpEntityKind::Road);
        }
        return true;
    }

    if (editor_state == EditorState::Editing_Districts &&
        add_click &&
        (gs.tool_runtime.district_subtool == DistrictSubtool::Paint ||
         gs.tool_runtime.district_subtool == DistrictSubtool::Zone)) {
        District district{};
        district.id = NextDistrictId(gs);
        district.type = DistrictType::Mixed;
        district.border = MakeSquareBoundary(world_pos, 45.0);
        district.orientation = {1.0, 0.0};
        gs.districts.add(std::move(district));
        const uint32_t new_id = NextDistrictId(gs) - 1u;
        SetPrimarySelection(gs, VpEntityKind::District, new_id);
        MarkDirtyForSelectionKind(gs, VpEntityKind::District);
        return true;
    }

    if (editor_state == EditorState::Editing_Lots && add_click && gs.tool_runtime.lot_subtool == LotSubtool::Plot) {
        LotToken lot{};
        lot.id = NextLotId(gs);
        lot.district_id = gs.selection.selected_district ? gs.selection.selected_district->id : 0u;
        lot.centroid = world_pos;
        lot.boundary = MakeSquareBoundary(world_pos, 16.0);
        lot.area = 32.0f * 32.0f;
        lot.is_user_placed = true;
        gs.lots.add(std::move(lot));
        const uint32_t new_id = NextLotId(gs) - 1u;
        SetPrimarySelection(gs, VpEntityKind::Lot, new_id);
        MarkDirtyForSelectionKind(gs, VpEntityKind::Lot);
        return true;
    }

    if (editor_state == EditorState::Editing_Buildings &&
        add_click &&
        gs.tool_runtime.building_subtool == BuildingSubtool::Place) {
        BuildingSite building{};
        building.id = NextBuildingId(gs);
        building.lot_id = gs.selection.selected_lot ? gs.selection.selected_lot->id : 0u;
        building.district_id = gs.selection.selected_district ? gs.selection.selected_district->id : 0u;
        building.position = world_pos;
        building.uniform_scale = 1.0f;
        building.type = BuildingType::Residential;
        building.is_user_placed = true;
        gs.buildings.push_back(building);
        SetPrimarySelection(gs, VpEntityKind::Building, building.id);
        MarkDirtyForSelectionKind(gs, VpEntityKind::Building);
        return true;
    }

    return false;
}

} // namespace

void ProcessViewportCommandTriggers(
    const CommandInteractionParams& params,
    const CommandMenuStateBundle& state_bundle) {
    if (params.editor_config == nullptr) {
        return;
    }

    const bool allow_viewport_mouse_actions = RC_UI::AllowViewportMouseActions(params.input_gate);
    const bool allow_viewport_key_actions = RC_UI::AllowViewportKeyActions(params.input_gate);
    const ImGuiIO& io = ImGui::GetIO();

    if (allow_viewport_mouse_actions &&
        params.in_viewport &&
        !params.minimap_hovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !io.KeyAlt &&
        !io.KeyShift &&
        !io.KeyCtrl) {
        RequestDefaultContextCommandMenu(*params.editor_config, params.mouse_pos, state_bundle);
    }

    const bool allow_command_hotkeys =
        allow_viewport_key_actions &&
        params.in_viewport &&
        !params.minimap_hovered &&
        !io.WantTextInput &&
        !ImGui::IsAnyItemActive();
    if (!allow_command_hotkeys) {
        return;
    }

    if (params.editor_config->viewport_hotkey_space_enabled && ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (state_bundle.smart_menu != nullptr) {
            RC_UI::Commands::RequestOpenSmartMenu(*state_bundle.smart_menu, params.mouse_pos);
        }
    }

    if ((params.editor_config->viewport_hotkey_slash_enabled && ImGui::IsKeyPressed(ImGuiKey_Slash, false)) ||
        (params.editor_config->viewport_hotkey_grave_enabled && ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false))) {
        if (state_bundle.pie_menu != nullptr) {
            RC_UI::Commands::RequestOpenPieMenu(*state_bundle.pie_menu, params.mouse_pos);
        }
    }

    if (params.editor_config->viewport_hotkey_p_enabled &&
        !io.KeyCtrl &&
        ImGui::IsKeyPressed(ImGuiKey_P, false)) {
        if (state_bundle.command_palette != nullptr) {
            RC_UI::Commands::RequestOpenCommandPalette(*state_bundle.command_palette);
        }
    }
}

AxiomInteractionResult ProcessAxiomViewportInteraction(const AxiomInteractionParams& params) {
    AxiomInteractionResult result{};
    if (!params.axiom_mode ||
        !params.allow_viewport_mouse_actions ||
        params.primary_viewport == nullptr ||
        params.axiom_tool == nullptr) {
        return result;
    }

    ImGuiIO& io = ImGui::GetIO();
    result.active = true;
    result.has_world_pos = true;
    result.world_pos = params.primary_viewport->screen_to_world(params.mouse_pos);

    const bool orbit = io.KeyAlt && io.MouseDown[ImGuiMouseButton_Left];
    const bool pan = io.MouseDown[ImGuiMouseButton_Middle] || (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right]);
    const bool zoom_drag = io.KeyCtrl && io.MouseDown[ImGuiMouseButton_Right];
    result.nav_active = orbit || pan || zoom_drag;

    if (params.allow_viewport_key_actions && params.global_state != nullptr) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
            if (io.KeyShift) {
                params.axiom_tool->redo();
            } else {
                params.axiom_tool->undo();
            }
            params.global_state->dirty_layers.MarkFromAxiomEdit();
        } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
            params.axiom_tool->redo();
            params.global_state->dirty_layers.MarkFromAxiomEdit();
        }
    }

    if (io.MouseWheel != 0.0f) {
        const float z = params.primary_viewport->get_camera_z();
        const float factor = std::pow(1.12f, -io.MouseWheel);
        const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
        params.primary_viewport->set_camera_position(params.primary_viewport->get_camera_xy(), new_z);
    }

    if (orbit) {
        const float yaw = params.primary_viewport->get_camera_yaw();
        params.primary_viewport->set_camera_yaw(yaw + io.MouseDelta.x * 0.0075f);
    }

    if (pan) {
        const float z = params.primary_viewport->get_camera_z();
        const float zoom = 500.0f / std::max(100.0f, z);
        const float yaw = params.primary_viewport->get_camera_yaw();

        const float dx = io.MouseDelta.x / zoom;
        const float dy = io.MouseDelta.y / zoom;

        const float c = std::cos(yaw);
        const float s = std::sin(yaw);
        RogueCity::Core::Vec2 delta_world(dx * c - dy * s, dx * s + dy * c);

        auto cam = params.primary_viewport->get_camera_xy();
        cam.x -= delta_world.x;
        cam.y -= delta_world.y;
        params.primary_viewport->set_camera_position(cam, z);
    }

    if (zoom_drag) {
        const float z = params.primary_viewport->get_camera_z();
        const float factor = std::exp(io.MouseDelta.y * 0.01f);
        const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
        params.primary_viewport->set_camera_position(params.primary_viewport->get_camera_xy(), new_z);
    }

    if (!result.nav_active) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            params.axiom_tool->on_mouse_down(result.world_pos);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            params.axiom_tool->on_mouse_up(result.world_pos);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            params.axiom_tool->on_mouse_move(result.world_pos);
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && io.KeyCtrl) {
            params.axiom_tool->on_right_click(result.world_pos);
        }
    }

    return result;
}

NonAxiomInteractionResult ProcessNonAxiomViewportInteraction(
    const NonAxiomInteractionParams& params,
    NonAxiomInteractionState* interaction_state) {
    NonAxiomInteractionResult result{};
    if (params.axiom_mode ||
        params.primary_viewport == nullptr ||
        params.global_state == nullptr ||
        interaction_state == nullptr) {
        return result;
    }

    auto& gs = *params.global_state;
    result.active = true;
    result.has_world_pos = true;
    result.world_pos = params.primary_viewport->screen_to_world(params.mouse_pos);

    if (!params.allow_viewport_mouse_actions || !params.in_viewport || params.minimap_hovered) {
        gs.hovered_entity.reset();
        return result;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float zoom = params.primary_viewport->world_to_screen_scale(1.0f);

    if (params.allow_viewport_key_actions) {
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
            gs.gizmo.enabled = true;
            gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Translate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            gs.gizmo.enabled = true;
            gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Rotate;
        } else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            gs.gizmo.enabled = true;
            gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Scale;
        } else if (ImGui::IsKeyPressed(ImGuiKey_X)) {
            gs.gizmo.snapping = !gs.gizmo.snapping;
        } else if (ImGui::IsKeyPressed(ImGuiKey_1)) {
            gs.layer_manager.active_layer = 0u;
        } else if (ImGui::IsKeyPressed(ImGuiKey_2)) {
            gs.layer_manager.active_layer = 1u;
        } else if (ImGui::IsKeyPressed(ImGuiKey_3)) {
            gs.layer_manager.active_layer = 2u;
        }
    }

    ApplyToolPreferredGizmoOperation(gs, params.editor_state);

    bool consumed_interaction = HandleDomainPlacementActions(
        gs,
        params.editor_state,
        result.world_pos,
        *interaction_state);

    const auto& selected_items = gs.selection_manager.Items();
    if (!consumed_interaction && gs.gizmo.enabled && !selected_items.empty()) {
        const RogueCity::Core::Vec2 pivot = ComputeSelectionPivot(gs);
        const double pick_radius = std::max(8.0, 14.0 / std::max(0.1f, zoom));
        const double dist_to_pivot = result.world_pos.distanceTo(pivot);

        auto can_start_drag = [&]() {
            using RogueCity::Core::Editor::GizmoOperation;
            switch (gs.gizmo.operation) {
            case GizmoOperation::Translate:
                return dist_to_pivot <= pick_radius * 1.5;
            case GizmoOperation::Rotate:
                return dist_to_pivot >= pick_radius * 1.2 && dist_to_pivot <= pick_radius * 3.0;
            case GizmoOperation::Scale:
                return dist_to_pivot >= pick_radius * 1.0 && dist_to_pivot <= pick_radius * 2.2;
            default:
                return false;
            }
        };

        if (!interaction_state->gizmo_drag.active &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !io.KeyAlt && !io.KeyShift && !io.KeyCtrl &&
            can_start_drag()) {
            interaction_state->gizmo_drag.active = true;
            interaction_state->gizmo_drag.pivot = pivot;
            interaction_state->gizmo_drag.previous_world = result.world_pos;
            consumed_interaction = true;
        }

        if (interaction_state->gizmo_drag.active) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                using RogueCity::App::EditorManipulation::ApplyRotate;
                using RogueCity::App::EditorManipulation::ApplyScale;
                using RogueCity::App::EditorManipulation::ApplyTranslate;
                using RogueCity::Core::Editor::GizmoOperation;

                bool changed = false;
                if (gs.gizmo.operation == GizmoOperation::Translate) {
                    RogueCity::Core::Vec2 delta = result.world_pos - interaction_state->gizmo_drag.previous_world;
                    if (gs.gizmo.snapping && gs.gizmo.translate_snap > 0.0f) {
                        delta.x = std::round(delta.x / gs.gizmo.translate_snap) * gs.gizmo.translate_snap;
                        delta.y = std::round(delta.y / gs.gizmo.translate_snap) * gs.gizmo.translate_snap;
                    }
                    changed = ApplyTranslate(gs, selected_items, delta);
                } else if (gs.gizmo.operation == GizmoOperation::Rotate) {
                    const RogueCity::Core::Vec2 prev = interaction_state->gizmo_drag.previous_world - interaction_state->gizmo_drag.pivot;
                    const RogueCity::Core::Vec2 curr = result.world_pos - interaction_state->gizmo_drag.pivot;
                    const double prev_angle = std::atan2(prev.y, prev.x);
                    const double curr_angle = std::atan2(curr.y, curr.x);
                    double delta_angle = curr_angle - prev_angle;
                    if (gs.gizmo.snapping && gs.gizmo.rotate_snap_degrees > 0.0f) {
                        const double step = gs.gizmo.rotate_snap_degrees * std::numbers::pi / 180.0;
                        delta_angle = std::round(delta_angle / step) * step;
                    }
                    changed = ApplyRotate(gs, selected_items, interaction_state->gizmo_drag.pivot, delta_angle);
                } else if (gs.gizmo.operation == GizmoOperation::Scale) {
                    const double prev_dist = std::max(1e-5, interaction_state->gizmo_drag.previous_world.distanceTo(interaction_state->gizmo_drag.pivot));
                    const double curr_dist = std::max(1e-5, result.world_pos.distanceTo(interaction_state->gizmo_drag.pivot));
                    double factor = curr_dist / prev_dist;
                    if (gs.gizmo.snapping && gs.gizmo.scale_snap > 0.0f) {
                        factor = 1.0 + std::round((factor - 1.0) / gs.gizmo.scale_snap) * gs.gizmo.scale_snap;
                    }
                    changed = ApplyScale(gs, selected_items, interaction_state->gizmo_drag.pivot, factor);
                }

                if (changed) {
                    std::unordered_set<uint64_t> seen;
                    seen.reserve(selected_items.size());
                    for (const auto& item : selected_items) {
                        if (!seen.insert(SelectionKey(item)).second) {
                            continue;
                        }
                        MarkDirtyForSelectionKind(gs, item.kind);
                    }
                }
                interaction_state->gizmo_drag.previous_world = result.world_pos;
                consumed_interaction = true;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                interaction_state->gizmo_drag.active = false;
                consumed_interaction = true;
            }
        }
    }

    if (!consumed_interaction &&
        params.editor_state == RogueCity::Core::Editor::EditorState::Editing_Roads &&
        gs.selection.selected_road) {
        const uint32_t road_id = gs.selection.selected_road->id;
        RogueCity::Core::Road* road = FindRoadMutable(gs, road_id);
        if (road != nullptr && road->points.size() >= 2) {
            const double pick_radius = std::max(5.0, 9.0 / std::max(0.1f, zoom));
            if (!interaction_state->road_vertex_drag.active &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt &&
                (gs.tool_runtime.road_spline_subtool == RogueCity::Core::Editor::RoadSplineSubtool::Selection ||
                 gs.tool_runtime.road_spline_subtool == RogueCity::Core::Editor::RoadSplineSubtool::DirectSelect ||
                 gs.tool_runtime.road_spline_subtool == RogueCity::Core::Editor::RoadSplineSubtool::HandleTangents)) {
                double best = std::numeric_limits<double>::max();
                size_t best_idx = 0;
                for (size_t i = 0; i < road->points.size(); ++i) {
                    const double d = road->points[i].distanceTo(result.world_pos);
                    if (d < best) {
                        best = d;
                        best_idx = i;
                    }
                }
                if (best <= pick_radius) {
                    interaction_state->road_vertex_drag.active = true;
                    interaction_state->road_vertex_drag.road_id = road_id;
                    interaction_state->road_vertex_drag.vertex_index = best_idx;
                    consumed_interaction = true;
                }
            }

            if (!consumed_interaction &&
                gs.tool_runtime.road_spline_subtool == RogueCity::Core::Editor::RoadSplineSubtool::AddRemoveAnchor &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift) {
                if (io.KeyCtrl && road->points.size() > 2) {
                    double best = std::numeric_limits<double>::max();
                    size_t best_idx = 0;
                    for (size_t i = 0; i < road->points.size(); ++i) {
                        const double d = road->points[i].distanceTo(result.world_pos);
                        if (d < best) {
                            best = d;
                            best_idx = i;
                        }
                    }
                    if (best <= pick_radius) {
                        road->points.erase(road->points.begin() + static_cast<std::ptrdiff_t>(best_idx));
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::Road);
                        consumed_interaction = true;
                    }
                } else {
                    double best_edge = std::numeric_limits<double>::max();
                    size_t edge_idx = 0;
                    for (size_t i = 0; i + 1 < road->points.size(); ++i) {
                        const double d = DistanceToSegment(result.world_pos, road->points[i], road->points[i + 1]);
                        if (d < best_edge) {
                            best_edge = d;
                            edge_idx = i;
                        }
                    }
                    if (best_edge <= pick_radius * 1.5) {
                        road->points.insert(road->points.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1), result.world_pos);
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::Road);
                        consumed_interaction = true;
                    }
                }
            }

            if (interaction_state->road_vertex_drag.active) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    RogueCity::Core::Vec2 snapped = result.world_pos;
                    if (gs.district_boundary_editor.snap_to_grid && gs.district_boundary_editor.snap_size > 0.0f) {
                        const double snap = gs.district_boundary_editor.snap_size;
                        snapped.x = std::round(snapped.x / snap) * snap;
                        snapped.y = std::round(snapped.y / snap) * snap;
                    }
                    RogueCity::App::EditorManipulation::MoveRoadVertex(*road, interaction_state->road_vertex_drag.vertex_index, snapped);
                    MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::Road);
                    consumed_interaction = true;
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    interaction_state->road_vertex_drag.active = false;
                    consumed_interaction = true;
                }
            }
        }
    }

    if (!consumed_interaction &&
        params.editor_state == RogueCity::Core::Editor::EditorState::Editing_Districts &&
        gs.selection.selected_district) {
        const uint32_t district_id = gs.selection.selected_district->id;
        RogueCity::Core::District* district = FindDistrictMutable(gs, district_id);
        if (district != nullptr && district->border.size() >= 3) {
            const double pick_radius = std::max(5.0, 9.0 / std::max(0.1f, zoom));

            if (!interaction_state->district_boundary_drag.active &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt) {
                double best = std::numeric_limits<double>::max();
                size_t best_idx = 0;
                for (size_t i = 0; i < district->border.size(); ++i) {
                    const double d = district->border[i].distanceTo(result.world_pos);
                    if (d < best) {
                        best = d;
                        best_idx = i;
                    }
                }

                const bool remove_vertex =
                    gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Merge ||
                    gs.district_boundary_editor.delete_mode;
                const bool insert_vertex =
                    gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Split ||
                    gs.district_boundary_editor.insert_mode;

                if (remove_vertex && best <= pick_radius) {
                    RogueCity::App::EditorManipulation::RemoveDistrictVertex(*district, best_idx);
                    MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                    consumed_interaction = true;
                } else if (insert_vertex) {
                    double best_edge = std::numeric_limits<double>::max();
                    size_t edge_idx = 0;
                    for (size_t i = 0; i < district->border.size(); ++i) {
                        const auto& a = district->border[i];
                        const auto& b = district->border[(i + 1) % district->border.size()];
                        const double d = DistanceToSegment(result.world_pos, a, b);
                        if (d < best_edge) {
                            best_edge = d;
                            edge_idx = i;
                        }
                    }
                    if (best_edge <= pick_radius * 1.5) {
                        RogueCity::App::EditorManipulation::InsertDistrictVertex(*district, edge_idx, result.world_pos);
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                        consumed_interaction = true;
                    }
                } else if (best <= pick_radius) {
                    interaction_state->district_boundary_drag.active = true;
                    interaction_state->district_boundary_drag.district_id = district_id;
                    interaction_state->district_boundary_drag.vertex_index = best_idx;
                    consumed_interaction = true;
                }
            }

            if (interaction_state->district_boundary_drag.active) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    RogueCity::Core::Vec2 snapped = result.world_pos;
                    if (gs.district_boundary_editor.snap_to_grid && gs.district_boundary_editor.snap_size > 0.0f) {
                        const double snap = gs.district_boundary_editor.snap_size;
                        snapped.x = std::round(snapped.x / snap) * snap;
                        snapped.y = std::round(snapped.y / snap) * snap;
                    }
                    if (interaction_state->district_boundary_drag.vertex_index < district->border.size()) {
                        district->border[interaction_state->district_boundary_drag.vertex_index] = snapped;
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                    }
                    consumed_interaction = true;
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    interaction_state->district_boundary_drag.active = false;
                    consumed_interaction = true;
                }
            }
        }
    }

    const bool start_box = io.KeyAlt && io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool start_lasso = io.KeyAlt && !io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    if (!consumed_interaction && start_box) {
        interaction_state->selection_drag.box_active = true;
        interaction_state->selection_drag.lasso_active = false;
        interaction_state->selection_drag.box_start = result.world_pos;
        interaction_state->selection_drag.box_end = result.world_pos;
        gs.hovered_entity.reset();
        consumed_interaction = true;
    } else if (!consumed_interaction && start_lasso) {
        interaction_state->selection_drag.lasso_active = true;
        interaction_state->selection_drag.box_active = false;
        interaction_state->selection_drag.lasso_points.clear();
        interaction_state->selection_drag.lasso_points.push_back(result.world_pos);
        gs.hovered_entity.reset();
        consumed_interaction = true;
    }

    if (!consumed_interaction && interaction_state->selection_drag.box_active) {
        interaction_state->selection_drag.box_end = result.world_pos;
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            const double min_x = std::min(interaction_state->selection_drag.box_start.x, interaction_state->selection_drag.box_end.x);
            const double max_x = std::max(interaction_state->selection_drag.box_start.x, interaction_state->selection_drag.box_end.x);
            const double min_y = std::min(interaction_state->selection_drag.box_start.y, interaction_state->selection_drag.box_end.y);
            const double max_y = std::max(interaction_state->selection_drag.box_start.y, interaction_state->selection_drag.box_end.y);

            auto region_items = QueryRegionFromViewportIndex(
                gs,
                [=](const RogueCity::Core::Vec2& p) {
                    return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
                },
                true);
            gs.selection_manager.SetItems(std::move(region_items));
            RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
            gs.hovered_entity.reset();
            interaction_state->selection_drag.box_active = false;
        }
    } else if (!consumed_interaction && interaction_state->selection_drag.lasso_active) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            if (interaction_state->selection_drag.lasso_points.empty() ||
                interaction_state->selection_drag.lasso_points.back().distanceTo(result.world_pos) > 3.0) {
                interaction_state->selection_drag.lasso_points.push_back(result.world_pos);
            }
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            auto region_items = QueryRegionFromViewportIndex(
                gs,
                [&](const RogueCity::Core::Vec2& p) {
                    return PointInPolygon(p, interaction_state->selection_drag.lasso_points);
                });
            gs.selection_manager.SetItems(std::move(region_items));
            RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
            gs.hovered_entity.reset();
            interaction_state->selection_drag.lasso_active = false;
            interaction_state->selection_drag.lasso_points.clear();
        }
    } else if (!consumed_interaction) {
        const auto hovered = PickFromViewportIndex(gs, result.world_pos, zoom);
        gs.hovered_entity = hovered;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (hovered.has_value()) {
                if (io.KeyShift) {
                    gs.selection_manager.Add(hovered->kind, hovered->id);
                } else if (io.KeyCtrl) {
                    gs.selection_manager.Toggle(hovered->kind, hovered->id);
                } else {
                    gs.selection_manager.Select(hovered->kind, hovered->id);
                }
                RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
            } else if (!io.KeyShift && !io.KeyCtrl) {
                gs.selection_manager.Clear();
                RogueCity::Core::Editor::ClearPrimarySelection(gs.selection);
                gs.hovered_entity.reset();
            }
        }
    }

    return result;
}

} // namespace RC_UI::Viewport
