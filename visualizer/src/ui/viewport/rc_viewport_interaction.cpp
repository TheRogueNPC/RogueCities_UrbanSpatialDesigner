#include "ui/viewport/rc_viewport_interaction.h"
#include "ui/viewport/handlers/rc_viewport_non_axiom_pipeline.h"

#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"
#include "ui/tools/rc_tool_geometry_policy.h"
#include "ui/tools/rc_tool_interaction_metrics.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>
#include <unordered_set>
#include <vector>

namespace RC_UI::Viewport {

const char* InteractionOutcomeString(InteractionOutcome outcome) {
    switch (outcome) {
        case InteractionOutcome::None: return "None";
        case InteractionOutcome::Mutation: return "Mutation";
        case InteractionOutcome::Selection: return "Selection";
        case InteractionOutcome::GizmoInteraction: return "GizmoInteraction";
        case InteractionOutcome::ActivateOnly: return "ActivateOnly";
        case InteractionOutcome::BlockedByInputGate: return "BlockedByInputGate";
        case InteractionOutcome::NoEligibleTarget: return "NoEligibleTarget";
        default: return "Unknown";
    }
}

namespace {

using BoostPoint = boost::geometry::model::d2::point_xy<double>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
using BoostSegment = boost::geometry::model::segment<BoostPoint>;

void RequestDefaultContextCommandMenu(
    const RogueCity::Core::Editor::EditorConfig& config,
    const ImVec2& screen_pos,
    const CommandMenuStateBundle& state_bundle,
    std::optional<RC_UI::ToolLibrary> preferred_library = std::nullopt) {
    using RogueCity::Core::Editor::ViewportCommandMode;
    switch (config.viewport_context_default_mode) {
        case ViewportCommandMode::SmartList:
            if (state_bundle.smart_menu != nullptr) {
                RC_UI::Commands::RequestOpenSmartMenu(*state_bundle.smart_menu, screen_pos, preferred_library);
            }
            break;
        case ViewportCommandMode::Pie:
            if (state_bundle.pie_menu != nullptr) {
                RC_UI::Commands::RequestOpenPieMenu(*state_bundle.pie_menu, screen_pos, preferred_library);
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
    const BoostPoint point(p.x, p.y);
    const BoostSegment segment(BoostPoint(a.x, a.y), BoostPoint(b.x, b.y));
    return boost::geometry::distance(point, segment);
}

bool PointInPolygon(const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    BoostPolygon boost_polygon{};
    auto& outer = boost_polygon.outer();
    outer.clear();
    outer.reserve(polygon.size() + 1);
    for (const auto& p : polygon) {
        outer.emplace_back(p.x, p.y);
    }
    if (!polygon.front().equals(polygon.back())) {
        outer.emplace_back(polygon.front().x, polygon.front().y);
    }
    boost::geometry::correct(boost_polygon);
    if (boost_polygon.outer().size() < 4) {
        return false;
    }
    return boost::geometry::covered_by(BoostPoint(point.x, point.y), boost_polygon);
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
    const RC_UI::Tools::ToolInteractionMetrics& interaction_metrics) {
    const double world_radius = interaction_metrics.world_pick_radius;
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

RogueCity::Core::LotToken* FindLotMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& lot : gs.lots) {
        if (lot.id == id) {
            return &lot;
        }
    }
    return nullptr;
}

RogueCity::Core::BuildingSite* FindBuildingMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& building : gs.buildings) {
        if (building.id == id) {
            return &building;
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

void SnapToGrid(RogueCity::Core::Vec2& point, bool enabled, float snap_size) {
    if (!enabled || snap_size <= 0.0f) {
        return;
    }
    const double snap = snap_size;
    point.x = std::round(point.x / snap) * snap;
    point.y = std::round(point.y / snap) * snap;
}

void ApplyAxisAlign(RogueCity::Core::Vec2& point, const RogueCity::Core::Vec2& reference) {
    const double dx = std::abs(point.x - reference.x);
    const double dy = std::abs(point.y - reference.y);
    if (dx < dy) {
        point.x = reference.x;
    } else {
        point.y = reference.y;
    }
}

bool SimplifyPolyline(std::vector<RogueCity::Core::Vec2>& points, bool closed) {
    if (points.size() < 5) {
        return false;
    }

    std::vector<RogueCity::Core::Vec2> simplified;
    simplified.reserve(points.size());
    if (!closed) {
        simplified.push_back(points.front());
    }
    for (size_t i = closed ? 0u : 1u; i + 1 < points.size(); i += 2u) {
        simplified.push_back(points[i]);
    }
    if (!closed) {
        simplified.push_back(points.back());
    }

    if (simplified.size() < 2) {
        return false;
    }
    points = std::move(simplified);
    return true;
}

void CycleBuildingType(RogueCity::Core::BuildingType& type) {
    using RogueCity::Core::BuildingType;
    switch (type) {
    case BuildingType::None: type = BuildingType::Residential; break;
    case BuildingType::Residential: type = BuildingType::Rowhome; break;
    case BuildingType::Rowhome: type = BuildingType::Retail; break;
    case BuildingType::Retail: type = BuildingType::MixedUse; break;
    case BuildingType::MixedUse: type = BuildingType::Industrial; break;
    case BuildingType::Industrial: type = BuildingType::Civic; break;
    case BuildingType::Civic: type = BuildingType::Luxury; break;
    case BuildingType::Luxury: type = BuildingType::Utility; break;
    case BuildingType::Utility: type = BuildingType::Residential; break;
    }
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
    const RC_UI::Tools::ToolInteractionMetrics& interaction_metrics,
    const RC_UI::Tools::ToolGeometryPolicy& geometry_policy,
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
    using RogueCity::Core::Editor::WaterSubtool;
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
        const double pick_radius = interaction_metrics.world_vertex_pick_radius;

        if (add_click && gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Pen) {
            if (selected_water == nullptr) {
                WaterBody water{};
                water.id = NextWaterId(gs);
                water.type = WaterType::River;
                water.depth = static_cast<float>(geometry_policy.water_default_depth);
                water.generate_shore = true;
                water.is_user_placed = true;
                water.generation_tag = RogueCity::Core::GenerationTag::M_user;
                water.generation_locked = true;
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
            if (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::ConvertAnchor &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift &&
                selected_water->boundary.size() >= 3) {
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
                    const size_t prev_idx = (best_idx == 0) ? selected_water->boundary.size() - 1 : best_idx - 1;
                    const size_t next_idx = (best_idx + 1) % selected_water->boundary.size();
                    selected_water->boundary[best_idx] =
                        (selected_water->boundary[prev_idx] + selected_water->boundary[next_idx]) * 0.5;
                    MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                    return true;
                }
            }

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
                    if (best_edge <= pick_radius * geometry_policy.edge_insert_multiplier) {
                        selected_water->boundary.insert(
                            selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1),
                            world_pos);
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                        return true;
                    }
                }
            }

            if (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::JoinSplit &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift) {
                if (io.KeyCtrl) {
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
                    if (best_edge <= pick_radius * geometry_policy.edge_insert_multiplier) {
                        const RogueCity::Core::Vec2 midpoint =
                            (selected_water->boundary[edge_idx] + selected_water->boundary[edge_idx + 1]) * 0.5;
                        selected_water->boundary.insert(
                            selected_water->boundary.begin() + static_cast<std::ptrdiff_t>(edge_idx + 1),
                            midpoint);
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                        return true;
                    }
                } else if (selected_water->boundary.size() >= 5) {
                    double best = std::numeric_limits<double>::max();
                    size_t best_idx = 1;
                    for (size_t i = 1; i + 1 < selected_water->boundary.size(); ++i) {
                        const double d = selected_water->boundary[i].distanceTo(world_pos);
                        if (d < best) {
                            best = d;
                            best_idx = i;
                        }
                    }
                    if (best <= pick_radius) {
                        WaterBody split_water{};
                        split_water.id = NextWaterId(gs);
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
                        gs.waterbodies.add(std::move(split_water));
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                        return true;
                    }
                }
            }

            if (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Simplify &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift) {
                const bool closed_shape = selected_water->type != WaterType::River;
                if (SimplifyPolyline(selected_water->boundary, closed_shape)) {
                    MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                    return true;
                }
            }

            const bool allow_water_vertex_drag =
                (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::Selection ||
                 gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::DirectSelect ||
                 gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::HandleTangents ||
                 gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::SnapAlign ||
                 gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
                 gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
                 gs.tool_runtime.water_subtool == WaterSubtool::Flow);
            if (!interaction_state.water_vertex_drag.active &&
                allow_water_vertex_drag &&
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

            if (interaction_state.water_vertex_drag.active &&
                interaction_state.water_vertex_drag.water_id == selected_water->id) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (interaction_state.water_vertex_drag.vertex_index < selected_water->boundary.size()) {
                        RogueCity::Core::Vec2 snapped = world_pos;
                        SnapToGrid(snapped, gs.district_boundary_editor.snap_to_grid, gs.district_boundary_editor.snap_size);

                        const size_t anchor_index = interaction_state.water_vertex_drag.vertex_index;
                        const RogueCity::Core::Vec2 anchor_before = selected_water->boundary[anchor_index];
                        if (gs.tool_runtime.water_spline_subtool == WaterSplineSubtool::SnapAlign) {
                            ApplyAxisAlign(snapped, anchor_before);
                        }
                        const RogueCity::Core::Vec2 delta = snapped - anchor_before;
                        const bool use_falloff =
                            (gs.tool_runtime.water_subtool == WaterSubtool::Erode ||
                             gs.tool_runtime.water_subtool == WaterSubtool::Contour ||
                             gs.tool_runtime.water_subtool == WaterSubtool::Flow);

                        if (use_falloff && geometry_policy.water_falloff_radius_world > 0.0) {
                            for (size_t i = 0; i < selected_water->boundary.size(); ++i) {
                                const double distance = selected_water->boundary[i].distanceTo(anchor_before);
                                if (distance > geometry_policy.water_falloff_radius_world) {
                                    continue;
                                }
                                double weight = RC_UI::Tools::ComputeFalloffWeight(
                                    distance,
                                    geometry_policy.water_falloff_radius_world);
                                if (gs.tool_runtime.water_subtool == WaterSubtool::Erode) {
                                    weight *= 0.72;
                                } else if (gs.tool_runtime.water_subtool == WaterSubtool::Flow) {
                                    weight *= 0.9;
                                }
                                selected_water->boundary[i] += delta * weight;
                            }
                        } else {
                            selected_water->boundary[anchor_index] = snapped;
                        }
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Water);
                    }
                    return true;
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    interaction_state.water_vertex_drag.active = false;
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
            new_road.generation_tag = RogueCity::Core::GenerationTag::M_user;
            new_road.generation_locked = true;
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
        gs.tool_runtime.district_subtool == DistrictSubtool::Merge &&
        gs.selection.selected_district) {
        const uint32_t primary_id = gs.selection.selected_district->id;
        const auto target = PickFromViewportIndex(gs, world_pos, interaction_metrics);
        if (target.has_value() &&
            target->kind == VpEntityKind::District &&
            target->id != primary_id) {
            RogueCity::Core::Vec2 primary_anchor{};
            RogueCity::Core::Vec2 target_anchor{};
            const bool anchors_ok =
                ResolveSelectionAnchor(gs, VpEntityKind::District, primary_id, primary_anchor) &&
                ResolveSelectionAnchor(gs, VpEntityKind::District, target->id, target_anchor);
            if (anchors_ok && primary_anchor.distanceTo(target_anchor) <= geometry_policy.merge_radius_world) {
                if (RogueCity::Core::District* primary = FindDistrictMutable(gs, primary_id);
                    primary != nullptr) {
                    const uint32_t target_id = target->id;
                    if (RogueCity::Core::District* other = FindDistrictMutable(gs, target_id);
                        other != nullptr && !other->border.empty()) {
                        primary->border.insert(primary->border.end(), other->border.begin(), other->border.end());
                        for (size_t i = 0; i < gs.districts.size(); ++i) {
                            auto handle = gs.districts.createHandleFromData(i);
                            if (handle && handle->id == target_id) {
                                gs.districts.remove(handle);
                                break;
                            }
                        }
                        SetPrimarySelection(gs, VpEntityKind::District, primary_id);
                        MarkDirtyForSelectionKind(gs, VpEntityKind::District);
                        return true;
                    }
                }
            }
        }
    }

    if (editor_state == EditorState::Editing_Districts &&
        add_click &&
        (gs.tool_runtime.district_subtool == DistrictSubtool::Paint ||
         gs.tool_runtime.district_subtool == DistrictSubtool::Zone)) {
        District district{};
        district.id = NextDistrictId(gs);
        district.type = DistrictType::Mixed;
        district.border = MakeSquareBoundary(world_pos, geometry_policy.district_placement_half_extent);
        district.orientation = {1.0, 0.0};
        district.is_user_placed = true;
        district.generation_tag = RogueCity::Core::GenerationTag::M_user;
        district.generation_locked = true;
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
        lot.boundary = MakeSquareBoundary(world_pos, geometry_policy.lot_placement_half_extent);
        lot.area = static_cast<float>(
            (geometry_policy.lot_placement_half_extent * 2.0) *
            (geometry_policy.lot_placement_half_extent * 2.0));
        lot.is_user_placed = true;
        lot.generation_tag = RogueCity::Core::GenerationTag::M_user;
        lot.generation_locked = true;
        gs.lots.add(std::move(lot));
        const uint32_t new_id = NextLotId(gs) - 1u;
        SetPrimarySelection(gs, VpEntityKind::Lot, new_id);
        MarkDirtyForSelectionKind(gs, VpEntityKind::Lot);
        return true;
    }

    if (editor_state == EditorState::Editing_Lots &&
        add_click &&
        gs.tool_runtime.lot_subtool == LotSubtool::Merge &&
        gs.selection.selected_lot) {
        const uint32_t primary_id = gs.selection.selected_lot->id;
        const auto target = PickFromViewportIndex(gs, world_pos, interaction_metrics);
        if (target.has_value() &&
            target->kind == VpEntityKind::Lot &&
            target->id != primary_id) {
            RogueCity::Core::Vec2 primary_anchor{};
            RogueCity::Core::Vec2 target_anchor{};
            const bool anchors_ok =
                ResolveSelectionAnchor(gs, VpEntityKind::Lot, primary_id, primary_anchor) &&
                ResolveSelectionAnchor(gs, VpEntityKind::Lot, target->id, target_anchor);
            if (anchors_ok && primary_anchor.distanceTo(target_anchor) <= geometry_policy.merge_radius_world) {
                const uint32_t target_id = target->id;
                for (size_t i = 0; i < gs.lots.size(); ++i) {
                    auto h_primary = gs.lots.createHandleFromData(i);
                    if (!h_primary || h_primary->id != primary_id) {
                        continue;
                    }
                    for (size_t j = 0; j < gs.lots.size(); ++j) {
                        auto h_other = gs.lots.createHandleFromData(j);
                        if (!h_other || h_other->id != target_id) {
                            continue;
                        }
                        h_primary->boundary.insert(h_primary->boundary.end(), h_other->boundary.begin(), h_other->boundary.end());
                        h_primary->area += h_other->area;
                        h_primary->centroid = (h_primary->centroid + h_other->centroid) * 0.5;
                        gs.lots.remove(h_other);
                        SetPrimarySelection(gs, VpEntityKind::Lot, primary_id);
                        MarkDirtyForSelectionKind(gs, VpEntityKind::Lot);
                        return true;
                    }
                    break;
                }
            }
        }
    }

    if (editor_state == EditorState::Editing_Lots &&
        add_click &&
        gs.tool_runtime.lot_subtool == LotSubtool::Slice &&
        gs.selection.selected_lot) {
        LotToken* lot = FindLotMutable(gs, gs.selection.selected_lot->id);
        if (lot != nullptr) {
            RogueCity::Core::Vec2 center = lot->centroid;
            if (!lot->boundary.empty()) {
                center = PolygonCentroid(lot->boundary);
            }

            double min_x = center.x - geometry_policy.lot_placement_half_extent;
            double max_x = center.x + geometry_policy.lot_placement_half_extent;
            double min_y = center.y - geometry_policy.lot_placement_half_extent;
            double max_y = center.y + geometry_policy.lot_placement_half_extent;
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

            const bool vertical_split = std::abs(world_pos.x - center.x) >= std::abs(world_pos.y - center.y);
            LotToken new_lot = *lot;
            new_lot.id = NextLotId(gs);
            new_lot.is_user_placed = true;
            new_lot.generation_tag = RogueCity::Core::GenerationTag::M_user;
            new_lot.generation_locked = true;

            if (vertical_split) {
                const double width = max_x - min_x;
                const double split_x = (width <= 2.0)
                    ? (min_x + max_x) * 0.5
                    : std::clamp(world_pos.x, min_x + 1.0, max_x - 1.0);
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
                    : std::clamp(world_pos.y, min_y + 1.0, max_y - 1.0);
                lot->boundary = {
                    {min_x, min_y}, {max_x, min_y}, {max_x, split_y}, {min_x, split_y}
                };
                new_lot.boundary = {
                    {min_x, split_y}, {max_x, split_y}, {max_x, max_y}, {min_x, max_y}
                };
            }

            lot->centroid = PolygonCentroid(lot->boundary);
            new_lot.centroid = PolygonCentroid(new_lot.boundary);
            auto rect_area = [](const std::vector<RogueCity::Core::Vec2>& b) {
                if (b.size() < 4) return 0.0f;
                const double w = std::abs(b[1].x - b[0].x);
                const double h = std::abs(b[3].y - b[0].y);
                return static_cast<float>(w * h);
            };
            lot->area = rect_area(lot->boundary);
            new_lot.area = rect_area(new_lot.boundary);
            gs.lots.add(std::move(new_lot));
            MarkDirtyForSelectionKind(gs, VpEntityKind::Lot);
            return true;
        }
    }

    if (editor_state == EditorState::Editing_Lots &&
        add_click &&
        gs.tool_runtime.lot_subtool == LotSubtool::Align &&
        gs.selection.selected_lot) {
        LotToken* lot = FindLotMutable(gs, gs.selection.selected_lot->id);
        if (lot != nullptr) {
            RogueCity::Core::Vec2 center = lot->boundary.empty() ? lot->centroid : PolygonCentroid(lot->boundary);
            SnapToGrid(center, gs.district_boundary_editor.snap_to_grid, gs.district_boundary_editor.snap_size);

            double half_extent = geometry_policy.lot_placement_half_extent;
            if (lot->area > 1.0f) {
                half_extent = std::max(6.0, std::sqrt(static_cast<double>(lot->area)) * 0.5);
            }
            lot->boundary = MakeSquareBoundary(center, half_extent);
            lot->centroid = center;
            lot->area = static_cast<float>((half_extent * 2.0) * (half_extent * 2.0));
            MarkDirtyForSelectionKind(gs, VpEntityKind::Lot);
            return true;
        }
    }

    if (editor_state == EditorState::Editing_Buildings &&
        add_click &&
        gs.tool_runtime.building_subtool == BuildingSubtool::Place) {
        BuildingSite building{};
        building.id = NextBuildingId(gs);
        building.lot_id = gs.selection.selected_lot ? gs.selection.selected_lot->id : 0u;
        building.district_id = gs.selection.selected_district ? gs.selection.selected_district->id : 0u;
        building.position = world_pos;
        building.uniform_scale = static_cast<float>(geometry_policy.building_default_scale);
        building.type = BuildingType::Residential;
        building.is_user_placed = true;
        building.generation_tag = RogueCity::Core::GenerationTag::M_user;
        building.generation_locked = true;
        gs.buildings.push_back(building);
        SetPrimarySelection(gs, VpEntityKind::Building, building.id);
        MarkDirtyForSelectionKind(gs, VpEntityKind::Building);
        return true;
    }

    if (editor_state == EditorState::Editing_Buildings &&
        add_click &&
        gs.tool_runtime.building_subtool == BuildingSubtool::Assign) {
        RogueCity::Core::BuildingSite* building = nullptr;
        if (gs.selection.selected_building) {
            building = FindBuildingMutable(gs, gs.selection.selected_building->id);
        }
        if (building == nullptr) {
            const auto picked = PickFromViewportIndex(gs, world_pos, interaction_metrics);
            if (picked.has_value() && picked->kind == VpEntityKind::Building) {
                building = FindBuildingMutable(gs, picked->id);
                if (building != nullptr) {
                    SetPrimarySelection(gs, VpEntityKind::Building, building->id);
                }
            }
        }
        if (building != nullptr) {
            CycleBuildingType(building->type);
            MarkDirtyForSelectionKind(gs, VpEntityKind::Building);
            return true;
        }
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

    struct DomainHoldState {
        float held_seconds{ 0.0f };
        bool opened{ false };
    };
    static std::array<DomainHoldState, 6> s_domain_hold{};

    const auto reset_domain_holds = [&]() {
        for (auto& hold : s_domain_hold) {
            hold.held_seconds = 0.0f;
            hold.opened = false;
        }
    };

    if (!allow_command_hotkeys) {
        reset_domain_holds();
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

    if (params.editor_config->viewport_hotkey_domain_context_enabled) {
        using RC_UI::ToolLibrary;
        static constexpr std::array<std::pair<ImGuiKey, ToolLibrary>, 6> kDomainKeys{{
            {ImGuiKey_A, ToolLibrary::Axiom},
            {ImGuiKey_W, ToolLibrary::Water},
            {ImGuiKey_R, ToolLibrary::Road},
            {ImGuiKey_D, ToolLibrary::District},
            {ImGuiKey_L, ToolLibrary::Lot},
            {ImGuiKey_B, ToolLibrary::Building},
        }};
        constexpr float kHoldOpenSeconds = 0.28f;

        for (size_t i = 0; i < kDomainKeys.size(); ++i) {
            auto& hold = s_domain_hold[i];
            const bool key_down = ImGui::IsKeyDown(kDomainKeys[i].first) &&
                !io.KeyCtrl && !io.KeyShift && !io.KeyAlt;

            if (!key_down) {
                hold.held_seconds = 0.0f;
                hold.opened = false;
                continue;
            }

            hold.held_seconds += io.DeltaTime;
            if (!hold.opened && hold.held_seconds >= kHoldOpenSeconds) {
                RequestDefaultContextCommandMenu(
                    *params.editor_config,
                    params.mouse_pos,
                    state_bundle,
                    kDomainKeys[i].second);
                hold.opened = true;
            }
        }
    } else {
        reset_domain_holds();
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

    const bool orbit = io.KeyAlt &&
        (io.MouseDown[ImGuiMouseButton_Left] || io.MouseDown[ImGuiMouseButton_Middle]);
    const bool pan =
        ((!io.KeyAlt) && io.MouseDown[ImGuiMouseButton_Middle]) ||
        (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right]);
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
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            params.axiom_tool->on_right_click(result.world_pos);
        }
    }

    return result;
}

NonAxiomInteractionResult ProcessNonAxiomViewportInteraction(
    const NonAxiomInteractionParams& params,
    NonAxiomInteractionState* interaction_state) {
    // Contract anchors: BuildToolInteractionMetrics(...), ResolveToolGeometryPolicy(...),
    // and requires_explicit_generation are handled inside the delegated pipeline module.
    return ProcessNonAxiomViewportInteractionPipeline(params, interaction_state);
}

} // namespace RC_UI::Viewport
