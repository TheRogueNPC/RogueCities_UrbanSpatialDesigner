#include "ui/viewport/handlers/rc_viewport_handler_common.h"

#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace RC_UI::Viewport::Handlers {

namespace {

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

} // namespace

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
    bool include_hidden) {
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

} // namespace RC_UI::Viewport::Handlers
