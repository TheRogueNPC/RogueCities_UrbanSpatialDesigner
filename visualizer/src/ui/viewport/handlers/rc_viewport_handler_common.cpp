#include "ui/viewport/handlers/rc_viewport_handler_common.h"

#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace RC_UI::Viewport::Handlers {

namespace {

using BoostPoint = boost::geometry::model::d2::point_xy<double>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
using BoostSegment = boost::geometry::model::segment<BoostPoint>;
using RogueCity::Core::Bounds;
using RogueCity::Core::BuildingSite;
using RogueCity::Core::District;
using RogueCity::Core::Editor::RenderSpatialGrid;
using RogueCity::Core::Editor::SelectionItem;
using RogueCity::Core::Editor::VpEntityKind;
using RogueCity::Core::LotToken;
using RogueCity::Core::Road;
using RogueCity::Core::Vec2;
using RogueCity::Core::WaterBody;

bool BuildBoostPolygon(
    const std::vector<RogueCity::Core::Vec2>& border,
    BoostPolygon& out_poly) {
    if (border.size() < 3) {
        return false;
    }

    auto& outer = out_poly.outer();
    outer.clear();
    outer.reserve(border.size() + 1);
    for (const auto& p : border) {
        outer.emplace_back(p.x, p.y);
    }
    if (!border.front().equals(border.back())) {
        outer.emplace_back(border.front().x, border.front().y);
    }

    boost::geometry::correct(out_poly);
    return out_poly.outer().size() >= 4;
}

bool ProbeContainsPoint(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id,
    const RogueCity::Core::Vec2& world_pos,
    double world_radius,
    bool prefer_manhattan,
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
            const double dx = std::abs(world_pos.x - lot.centroid.x);
            const double dy = std::abs(world_pos.y - lot.centroid.y);
            const double euclid = std::sqrt(dx * dx + dy * dy);
            const double manhattan = dx + dy;
            const double d = prefer_manhattan ? (0.7 * euclid + 0.3 * manhattan) : euclid;
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
            const double dx = std::abs(world_pos.x - building.position.x);
            const double dy = std::abs(world_pos.y - building.position.y);
            const double euclid = std::sqrt(dx * dx + dy * dy);
            const double manhattan = dx + dy;
            const double d = prefer_manhattan ? (0.65 * euclid + 0.35 * manhattan) : euclid;
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

[[nodiscard]] bool ProbeContainsRoad(
    const Road& road,
    const Vec2& world_pos,
    double world_radius,
    int& out_priority,
    double& out_distance) {
    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    if (road.points.size() < 2) {
        return false;
    }
    double min_distance = std::numeric_limits<double>::max();
    for (size_t i = 1; i < road.points.size(); ++i) {
        min_distance = std::min(min_distance, DistanceToSegment(world_pos, road.points[i - 1], road.points[i]));
    }
    if (min_distance > world_radius * 1.25) {
        return false;
    }
    out_priority = 3;
    out_distance = min_distance;
    return true;
}

[[nodiscard]] bool ProbeContainsDistrict(
    const District& district,
    const Vec2& world_pos,
    int& out_priority,
    double& out_distance) {
    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    if (district.border.empty() || !PointInPolygon(world_pos, district.border)) {
        return false;
    }
    out_priority = 2;
    out_distance = world_pos.distanceTo(PolygonCentroid(district.border));
    return true;
}

[[nodiscard]] bool ProbeContainsLot(
    const LotToken& lot,
    const Vec2& world_pos,
    double world_radius,
    bool prefer_manhattan,
    int& out_priority,
    double& out_distance) {
    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    if (!lot.boundary.empty() && PointInPolygon(world_pos, lot.boundary)) {
        out_priority = 4;
        out_distance = world_pos.distanceTo(lot.centroid);
        return true;
    }
    const double dx = std::abs(world_pos.x - lot.centroid.x);
    const double dy = std::abs(world_pos.y - lot.centroid.y);
    const double euclid = std::sqrt(dx * dx + dy * dy);
    const double manhattan = dx + dy;
    const double d = prefer_manhattan ? (0.7 * euclid + 0.3 * manhattan) : euclid;
    if (d > world_radius * 2.0) {
        return false;
    }
    out_priority = 2;
    out_distance = d;
    return true;
}

[[nodiscard]] bool ProbeContainsBuilding(
    const BuildingSite& building,
    const Vec2& world_pos,
    double world_radius,
    bool prefer_manhattan,
    int& out_priority,
    double& out_distance) {
    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    const double dx = std::abs(world_pos.x - building.position.x);
    const double dy = std::abs(world_pos.y - building.position.y);
    const double euclid = std::sqrt(dx * dx + dy * dy);
    const double manhattan = dx + dy;
    const double d = prefer_manhattan ? (0.65 * euclid + 0.35 * manhattan) : euclid;
    if (d > world_radius * 1.75) {
        return false;
    }
    out_priority = 5;
    out_distance = d;
    return true;
}

[[nodiscard]] bool ProbeContainsWater(
    const WaterBody& water,
    const Vec2& world_pos,
    double world_radius,
    int& out_priority,
    double& out_distance) {
    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();
    if (water.boundary.size() < 2) {
        return false;
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
    if (min_distance > world_radius * 1.5) {
        return false;
    }
    out_priority = 3;
    out_distance = min_distance;
    return true;
}

template <typename TCallback>
void ForEachCellInWorldRadius(
    const RenderSpatialGrid& spatial,
    const Vec2& world_pos,
    double world_radius,
    TCallback&& callback) {
    if (!spatial.IsValid()) {
        return;
    }

    const int max_x = std::max(0, static_cast<int>(spatial.width) - 1);
    const int max_y = std::max(0, static_cast<int>(spatial.height) - 1);
    if (max_x < 0 || max_y < 0) {
        return;
    }

    int center_x = 0;
    int center_y = 0;
    if (!spatial.WorldToCell(world_pos, center_x, center_y)) {
        center_x = std::clamp(
            static_cast<int>(std::floor((world_pos.x - spatial.world_bounds.min.x) / std::max(1e-9, spatial.cell_size_x))),
            0,
            max_x);
        center_y = std::clamp(
            static_cast<int>(std::floor((world_pos.y - spatial.world_bounds.min.y) / std::max(1e-9, spatial.cell_size_y))),
            0,
            max_y);
    }

    const int radius_x = std::max(
        1,
        static_cast<int>(std::ceil(world_radius / std::max(1e-9, spatial.cell_size_x))));
    const int radius_y = std::max(
        1,
        static_cast<int>(std::ceil(world_radius / std::max(1e-9, spatial.cell_size_y))));

    const int start_x = std::clamp(center_x - radius_x, 0, max_x);
    const int end_x = std::clamp(center_x + radius_x, 0, max_x);
    const int start_y = std::clamp(center_y - radius_y, 0, max_y);
    const int end_y = std::clamp(center_y + radius_y, 0, max_y);
    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            callback(spatial.ToCellIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y)));
        }
    }
}

template <typename TCallback>
void ForEachCellInBounds(
    const RenderSpatialGrid& spatial,
    const Bounds& bounds,
    TCallback&& callback) {
    if (!spatial.IsValid() || spatial.width == 0u || spatial.height == 0u) {
        return;
    }

    if (bounds.max.x < spatial.world_bounds.min.x ||
        bounds.min.x > spatial.world_bounds.max.x ||
        bounds.max.y < spatial.world_bounds.min.y ||
        bounds.min.y > spatial.world_bounds.max.y) {
        return;
    }

    const int max_x = static_cast<int>(spatial.width) - 1;
    const int max_y = static_cast<int>(spatial.height) - 1;
    const double cell_x = std::max(1e-9, spatial.cell_size_x);
    const double cell_y = std::max(1e-9, spatial.cell_size_y);

    const int start_x = std::clamp(
        static_cast<int>(std::floor((bounds.min.x - spatial.world_bounds.min.x) / cell_x)),
        0,
        max_x);
    const int end_x = std::clamp(
        static_cast<int>(std::floor((bounds.max.x - spatial.world_bounds.min.x) / cell_x)),
        0,
        max_x);
    const int start_y = std::clamp(
        static_cast<int>(std::floor((bounds.min.y - spatial.world_bounds.min.y) / cell_y)),
        0,
        max_y);
    const int end_y = std::clamp(
        static_cast<int>(std::floor((bounds.max.y - spatial.world_bounds.min.y) / cell_y)),
        0,
        max_y);

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            callback(spatial.ToCellIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y)));
        }
    }
}

bool TryPickFromSpatialGrid(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_pos,
    const RC_UI::Tools::ToolInteractionMetrics& interaction_metrics,
    double radius_scale,
    bool prefer_manhattan,
    std::optional<RogueCity::Core::Editor::SelectionItem>& out_pick) {
    out_pick.reset();
    const auto& spatial = gs.render_spatial_grid;
    if (!spatial.IsValid()) {
        return false;
    }

    const double world_radius =
        std::max(0.25, interaction_metrics.world_pick_radius * std::max(0.2, radius_scale));

    int best_priority = -1;
    double best_distance = std::numeric_limits<double>::max();
    std::optional<SelectionItem> best{};
    std::unordered_set<uint64_t> seen_candidates;
    bool scanned_any_cell = false;

    const auto try_update_best =
        [&](VpEntityKind kind, uint32_t id, int priority, double distance) {
            if (priority > best_priority ||
                (priority == best_priority && distance < best_distance)) {
                best_priority = priority;
                best_distance = distance;
                best = SelectionItem{ kind, id };
            }
        };

    const auto try_road = [&](uint32_t handle) {
        if (handle >= gs.roads.indexCount() || !gs.roads.isValidIndex(handle)) {
            return;
        }
        const Road& road = gs.roads[handle];
        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(VpEntityKind::Road)) << 32ull) | road.id;
        if (!seen_candidates.insert(key).second || !gs.IsEntityVisible(VpEntityKind::Road, road.id)) {
            return;
        }
        int priority = 0;
        double distance = 0.0;
        if (ProbeContainsRoad(road, world_pos, world_radius, priority, distance)) {
            try_update_best(VpEntityKind::Road, road.id, priority, distance);
        }
    };

    const auto try_district = [&](uint32_t handle) {
        if (handle >= gs.districts.indexCount() || !gs.districts.isValidIndex(handle)) {
            return;
        }
        const District& district = gs.districts[handle];
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint8_t>(VpEntityKind::District)) << 32ull) |
            district.id;
        if (!seen_candidates.insert(key).second || !gs.IsEntityVisible(VpEntityKind::District, district.id)) {
            return;
        }
        int priority = 0;
        double distance = 0.0;
        if (ProbeContainsDistrict(district, world_pos, priority, distance)) {
            try_update_best(VpEntityKind::District, district.id, priority, distance);
        }
    };

    const auto try_lot = [&](uint32_t handle) {
        if (handle >= gs.lots.indexCount() || !gs.lots.isValidIndex(handle)) {
            return;
        }
        const LotToken& lot = gs.lots[handle];
        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(VpEntityKind::Lot)) << 32ull) | lot.id;
        if (!seen_candidates.insert(key).second || !gs.IsEntityVisible(VpEntityKind::Lot, lot.id)) {
            return;
        }
        int priority = 0;
        double distance = 0.0;
        if (ProbeContainsLot(lot, world_pos, world_radius, prefer_manhattan, priority, distance)) {
            try_update_best(VpEntityKind::Lot, lot.id, priority, distance);
        }
    };

    const auto try_water = [&](uint32_t handle) {
        if (handle >= gs.waterbodies.indexCount() || !gs.waterbodies.isValidIndex(handle)) {
            return;
        }
        const WaterBody& water = gs.waterbodies[handle];
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint8_t>(VpEntityKind::Water)) << 32ull) | water.id;
        if (!seen_candidates.insert(key).second || !gs.IsEntityVisible(VpEntityKind::Water, water.id)) {
            return;
        }
        int priority = 0;
        double distance = 0.0;
        if (ProbeContainsWater(water, world_pos, world_radius, priority, distance)) {
            try_update_best(VpEntityKind::Water, water.id, priority, distance);
        }
    };

    const auto try_building = [&](uint32_t handle) {
        if (handle >= gs.buildings.size()) {
            return;
        }
        const BuildingSite& building = gs.buildings.getData()[handle];
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint8_t>(VpEntityKind::Building)) << 32ull) |
            building.id;
        if (!seen_candidates.insert(key).second || !gs.IsEntityVisible(VpEntityKind::Building, building.id)) {
            return;
        }
        int priority = 0;
        double distance = 0.0;
        if (ProbeContainsBuilding(
                building,
                world_pos,
                world_radius,
                prefer_manhattan,
                priority,
                distance)) {
            try_update_best(VpEntityKind::Building, building.id, priority, distance);
        }
    };

    ForEachCellInWorldRadius(spatial, world_pos, world_radius, [&](uint32_t cell) {
        scanned_any_cell = true;

        const uint32_t road_begin = spatial.roads.offsets[cell];
        const uint32_t road_end = spatial.roads.offsets[cell + 1u];
        for (uint32_t cursor = road_begin; cursor < road_end; ++cursor) {
            try_road(spatial.roads.handles[cursor]);
        }

        const uint32_t district_begin = spatial.districts.offsets[cell];
        const uint32_t district_end = spatial.districts.offsets[cell + 1u];
        for (uint32_t cursor = district_begin; cursor < district_end; ++cursor) {
            try_district(spatial.districts.handles[cursor]);
        }

        const uint32_t lot_begin = spatial.lots.offsets[cell];
        const uint32_t lot_end = spatial.lots.offsets[cell + 1u];
        for (uint32_t cursor = lot_begin; cursor < lot_end; ++cursor) {
            try_lot(spatial.lots.handles[cursor]);
        }

        const uint32_t water_begin = spatial.water.offsets[cell];
        const uint32_t water_end = spatial.water.offsets[cell + 1u];
        for (uint32_t cursor = water_begin; cursor < water_end; ++cursor) {
            try_water(spatial.water.handles[cursor]);
        }

        const uint32_t building_begin = spatial.buildings.offsets[cell];
        const uint32_t building_end = spatial.buildings.offsets[cell + 1u];
        for (uint32_t cursor = building_begin; cursor < building_end; ++cursor) {
            try_building(spatial.buildings.handles[cursor]);
        }
    });

    if (!scanned_any_cell) {
        return false;
    }

    out_pick = best;
    return true;
}

bool TryQueryRegionFromSpatialGrid(
    const RogueCity::Core::Editor::GlobalState& gs,
    const std::function<bool(const RogueCity::Core::Vec2&)>& include_point,
    bool include_hidden,
    const Bounds* region_bounds,
    std::vector<RogueCity::Core::Editor::SelectionItem>& out_results) {
    out_results.clear();
    const auto& spatial = gs.render_spatial_grid;
    if (!spatial.IsValid()) {
        return false;
    }

    std::vector<uint8_t> road_candidates(gs.roads.indexCount(), 0u);
    std::vector<uint8_t> district_candidates(gs.districts.indexCount(), 0u);
    std::vector<uint8_t> lot_candidates(gs.lots.indexCount(), 0u);
    std::vector<uint8_t> water_candidates(gs.waterbodies.indexCount(), 0u);
    std::vector<uint8_t> building_candidates(gs.buildings.size(), 0u);

    const auto mark_candidates_for_cell = [&](uint32_t cell) {
        const uint32_t road_begin = spatial.roads.offsets[cell];
        const uint32_t road_end = spatial.roads.offsets[cell + 1u];
        for (uint32_t cursor = road_begin; cursor < road_end; ++cursor) {
            const uint32_t handle = spatial.roads.handles[cursor];
            if (handle < road_candidates.size()) {
                road_candidates[handle] = 1u;
            }
        }

        const uint32_t district_begin = spatial.districts.offsets[cell];
        const uint32_t district_end = spatial.districts.offsets[cell + 1u];
        for (uint32_t cursor = district_begin; cursor < district_end; ++cursor) {
            const uint32_t handle = spatial.districts.handles[cursor];
            if (handle < district_candidates.size()) {
                district_candidates[handle] = 1u;
            }
        }

        const uint32_t lot_begin = spatial.lots.offsets[cell];
        const uint32_t lot_end = spatial.lots.offsets[cell + 1u];
        for (uint32_t cursor = lot_begin; cursor < lot_end; ++cursor) {
            const uint32_t handle = spatial.lots.handles[cursor];
            if (handle < lot_candidates.size()) {
                lot_candidates[handle] = 1u;
            }
        }

        const uint32_t water_begin = spatial.water.offsets[cell];
        const uint32_t water_end = spatial.water.offsets[cell + 1u];
        for (uint32_t cursor = water_begin; cursor < water_end; ++cursor) {
            const uint32_t handle = spatial.water.handles[cursor];
            if (handle < water_candidates.size()) {
                water_candidates[handle] = 1u;
            }
        }

        const uint32_t building_begin = spatial.buildings.offsets[cell];
        const uint32_t building_end = spatial.buildings.offsets[cell + 1u];
        for (uint32_t cursor = building_begin; cursor < building_end; ++cursor) {
            const uint32_t handle = spatial.buildings.handles[cursor];
            if (handle < building_candidates.size()) {
                building_candidates[handle] = 1u;
            }
        }
    };

    if (region_bounds != nullptr) {
        ForEachCellInBounds(spatial, *region_bounds, mark_candidates_for_cell);
    } else {
        for (uint32_t cell = 0; cell < spatial.CellCount(); ++cell) {
            mark_candidates_for_cell(cell);
        }
    }

    std::unordered_set<uint64_t> dedupe;
    dedupe.reserve(gs.viewport_index.size() + 8u);

    const auto try_add = [&](VpEntityKind kind, uint32_t id, const Vec2& anchor) {
        if (!include_hidden && !gs.IsEntityVisible(kind, id)) {
            return;
        }
        if (!include_point(anchor)) {
            return;
        }
        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(kind)) << 32ull) | id;
        if (!dedupe.insert(key).second) {
            return;
        }
        out_results.push_back(SelectionItem{ kind, id });
    };

    for (uint32_t handle = 0; handle < static_cast<uint32_t>(road_candidates.size()); ++handle) {
        if (road_candidates[handle] == 0u || !gs.roads.isValidIndex(handle)) {
            continue;
        }
        const Road& road = gs.roads[handle];
        if (road.points.empty()) {
            continue;
        }
        try_add(VpEntityKind::Road, road.id, road.points[road.points.size() / 2u]);
    }

    for (uint32_t handle = 0; handle < static_cast<uint32_t>(district_candidates.size()); ++handle) {
        if (district_candidates[handle] == 0u || !gs.districts.isValidIndex(handle)) {
            continue;
        }
        const District& district = gs.districts[handle];
        if (district.border.empty()) {
            continue;
        }
        try_add(VpEntityKind::District, district.id, PolygonCentroid(district.border));
    }

    for (uint32_t handle = 0; handle < static_cast<uint32_t>(lot_candidates.size()); ++handle) {
        if (lot_candidates[handle] == 0u || !gs.lots.isValidIndex(handle)) {
            continue;
        }
        const LotToken& lot = gs.lots[handle];
        try_add(
            VpEntityKind::Lot,
            lot.id,
            lot.boundary.empty() ? lot.centroid : PolygonCentroid(lot.boundary));
    }

    for (uint32_t handle = 0; handle < static_cast<uint32_t>(building_candidates.size()); ++handle) {
        if (building_candidates[handle] == 0u) {
            continue;
        }
        const BuildingSite& building = gs.buildings.getData()[handle];
        try_add(VpEntityKind::Building, building.id, building.position);
    }

    for (uint32_t handle = 0; handle < static_cast<uint32_t>(water_candidates.size()); ++handle) {
        if (water_candidates[handle] == 0u || !gs.waterbodies.isValidIndex(handle)) {
            continue;
        }
        const WaterBody& water = gs.waterbodies[handle];
        if (water.boundary.empty()) {
            continue;
        }
        try_add(VpEntityKind::Water, water.id, PolygonCentroid(water.boundary));
    }

    return true;
}

} // namespace

uint64_t SelectionKey(const RogueCity::Core::Editor::SelectionItem& item) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) | static_cast<uint64_t>(item.id);
}

double DistanceToSegment(const RogueCity::Core::Vec2& p, const RogueCity::Core::Vec2& a, const RogueCity::Core::Vec2& b) {
    const BoostPoint point(p.x, p.y);
    const BoostSegment segment(BoostPoint(a.x, a.y), BoostPoint(b.x, b.y));
    return boost::geometry::distance(point, segment);
}

bool PointInPolygon(const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
    BoostPolygon boost_polygon{};
    if (!BuildBoostPolygon(polygon, boost_polygon)) {
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

std::optional<RogueCity::Core::Editor::SelectionItem> PickFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_pos,
    const RC_UI::Tools::ToolInteractionMetrics& interaction_metrics,
    double radius_scale,
    bool prefer_manhattan) {
    std::optional<RogueCity::Core::Editor::SelectionItem> spatial_pick{};
    if (TryPickFromSpatialGrid(
            gs,
            world_pos,
            interaction_metrics,
            radius_scale,
            prefer_manhattan,
            spatial_pick)) {
        return spatial_pick;
    }

    const double world_radius = std::max(0.25, interaction_metrics.world_pick_radius * std::max(0.2, radius_scale));
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
        if (!ProbeContainsPoint(gs, probe.kind, probe.id, world_pos, world_radius, prefer_manhattan, priority, distance)) {
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
    bool include_hidden,
    const Bounds* region_bounds) {
    std::vector<RogueCity::Core::Editor::SelectionItem> spatial_results{};
    if (TryQueryRegionFromSpatialGrid(
            gs, include_point, include_hidden, region_bounds, spatial_results)) {
        return spatial_results;
    }

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

    const auto* primary = gs.selection_manager.Primary();
    if (primary != nullptr && primary->kind == kind) {
        PromoteEntityToUserLocked(gs, primary->kind, primary->id);
    } else {
        switch (kind) {
        case VpEntityKind::Road:
            if (gs.selection.selected_road) {
                PromoteEntityToUserLocked(gs, kind, gs.selection.selected_road->id);
            }
            break;
        case VpEntityKind::District:
            if (gs.selection.selected_district) {
                PromoteEntityToUserLocked(gs, kind, gs.selection.selected_district->id);
            }
            break;
        case VpEntityKind::Lot:
            if (gs.selection.selected_lot) {
                PromoteEntityToUserLocked(gs, kind, gs.selection.selected_lot->id);
            }
            break;
        case VpEntityKind::Building:
            if (gs.selection.selected_building) {
                PromoteEntityToUserLocked(gs, kind, gs.selection.selected_building->id);
            }
            break;
        default:
            break;
        }
    }

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

void PromoteEntityToUserLocked(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id) {
    using RogueCity::Core::Editor::VpEntityKind;
    switch (kind) {
    case VpEntityKind::Road:
        if (auto* road = FindRoadMutable(gs, id); road != nullptr) {
            road->is_user_created = true;
            road->generation_tag = RogueCity::Core::GenerationTag::M_user;
            road->generation_locked = true;
        }
        break;
    case VpEntityKind::District:
        if (auto* district = FindDistrictMutable(gs, id); district != nullptr) {
            district->is_user_placed = true;
            district->generation_tag = RogueCity::Core::GenerationTag::M_user;
            district->generation_locked = true;
        }
        break;
    case VpEntityKind::Lot:
        if (auto* lot = FindLotMutable(gs, id); lot != nullptr) {
            lot->is_user_placed = true;
            lot->generation_tag = RogueCity::Core::GenerationTag::M_user;
            lot->generation_locked = true;
        }
        break;
    case VpEntityKind::Building:
        if (auto* building = FindBuildingMutable(gs, id); building != nullptr) {
            building->is_user_placed = true;
            building->generation_tag = RogueCity::Core::GenerationTag::M_user;
            building->generation_locked = true;
        }
        break;
    case VpEntityKind::Water:
        if (auto* water = FindWaterMutable(gs, id); water != nullptr) {
            water->is_user_placed = true;
            water->generation_tag = RogueCity::Core::GenerationTag::M_user;
            water->generation_locked = true;
        }
        break;
    default:
        break;
    }
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
