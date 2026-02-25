// FILE: ViewportIndexBuilder.cpp
// PURPOSE: Build the viewport index from GlobalState entities.
#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/ViewportIndex.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <vector>

namespace RogueCity::App {

namespace {

using RogueCity::Core::Bounds;
using RogueCity::Core::BuildingSite;
using RogueCity::Core::District;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::RenderSpatialGrid;
using RogueCity::Core::Editor::RenderSpatialLayer;
using RogueCity::Core::Editor::VpEntityKind;
using RogueCity::Core::Editor::VpProbeData;
using RogueCity::Core::LotToken;
using RogueCity::Core::Road;
using RogueCity::Core::Vec2;
using RogueCity::Core::WaterBody;
using RogueCity::Core::Editor::SetViewportLabel;

constexpr double kMinWorldSpanMeters = 1.0;
constexpr uint32_t kMinGridAxis = 64u;
constexpr uint32_t kMaxGridAxis = 384u;
constexpr double kTargetEntriesPerCell = 20.0;

static void SetLabelWithId(VpProbeData& probe, const char* prefix, uint32_t id) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%s %u", prefix, id);
    SetViewportLabel(probe, buffer);
}

[[nodiscard]] bool IsFinite(double v) {
    return std::isfinite(v);
}

[[nodiscard]] bool HasUsableBounds(const Bounds& bounds) {
    return IsFinite(bounds.min.x) && IsFinite(bounds.min.y) &&
        IsFinite(bounds.max.x) && IsFinite(bounds.max.y) &&
        (bounds.max.x - bounds.min.x) > 1e-6 &&
        (bounds.max.y - bounds.min.y) > 1e-6;
}

[[nodiscard]] Bounds MakeNormalizedBounds(Bounds bounds) {
    if (bounds.min.x > bounds.max.x) {
        std::swap(bounds.min.x, bounds.max.x);
    }
    if (bounds.min.y > bounds.max.y) {
        std::swap(bounds.min.y, bounds.max.y);
    }

    const double width = bounds.max.x - bounds.min.x;
    const double height = bounds.max.y - bounds.min.y;

    if (width < kMinWorldSpanMeters) {
        const double cx = (bounds.min.x + bounds.max.x) * 0.5;
        bounds.min.x = cx - kMinWorldSpanMeters * 0.5;
        bounds.max.x = cx + kMinWorldSpanMeters * 0.5;
    }
    if (height < kMinWorldSpanMeters) {
        const double cy = (bounds.min.y + bounds.max.y) * 0.5;
        bounds.min.y = cy - kMinWorldSpanMeters * 0.5;
        bounds.max.y = cy + kMinWorldSpanMeters * 0.5;
    }

    return bounds;
}

void ExpandBounds(Bounds& bounds, const Vec2& point, bool& has_point) {
    if (!IsFinite(point.x) || !IsFinite(point.y)) {
        return;
    }
    if (!has_point) {
        bounds.min = point;
        bounds.max = point;
        has_point = true;
        return;
    }
    bounds.min.x = std::min(bounds.min.x, point.x);
    bounds.min.y = std::min(bounds.min.y, point.y);
    bounds.max.x = std::max(bounds.max.x, point.x);
    bounds.max.y = std::max(bounds.max.y, point.y);
}

template <typename TPointRange>
void ExpandBoundsFromPoints(Bounds& bounds, const TPointRange& points, bool& has_point) {
    for (const auto& point : points) {
        ExpandBounds(bounds, point, has_point);
    }
}

[[nodiscard]] Bounds ComputeEntityBounds(const GlobalState& gs, bool& out_has_bounds) {
    Bounds bounds{};
    bool has_point = false;

    for (const auto& road : gs.roads) {
        ExpandBoundsFromPoints(bounds, road.points, has_point);
    }
    for (const auto& district : gs.districts) {
        ExpandBoundsFromPoints(bounds, district.border, has_point);
    }
    for (const auto& lot : gs.lots) {
        if (!lot.boundary.empty()) {
            ExpandBoundsFromPoints(bounds, lot.boundary, has_point);
        } else {
            ExpandBounds(bounds, lot.centroid, has_point);
        }
    }
    for (const auto& water : gs.waterbodies) {
        ExpandBoundsFromPoints(bounds, water.boundary, has_point);
    }
    for (const auto& building : gs.buildings) {
        ExpandBounds(bounds, building.position, has_point);
    }

    out_has_bounds = has_point;
    return has_point ? MakeNormalizedBounds(bounds) : bounds;
}

[[nodiscard]] Bounds ComputePreferredWorldBounds(const GlobalState& gs) {
    if (!gs.city_boundary.empty()) {
        Bounds city_bounds{};
        bool has_city = false;
        ExpandBoundsFromPoints(city_bounds, gs.city_boundary, has_city);
        if (has_city) {
            return MakeNormalizedBounds(city_bounds);
        }
    }

    if (HasUsableBounds(gs.texture_space_bounds)) {
        return MakeNormalizedBounds(gs.texture_space_bounds);
    }

    bool has_entities = false;
    const Bounds entity_bounds = ComputeEntityBounds(gs, has_entities);
    if (has_entities) {
        return entity_bounds;
    }

    return MakeNormalizedBounds(
        RogueCity::Core::Editor::ComputeWorldBounds(gs.city_texture_size, gs.city_meters_per_pixel));
}

[[nodiscard]] bool HasRenderableEntities(const GlobalState& gs) {
    return gs.roads.size() > 0 || gs.districts.size() > 0 || gs.lots.size() > 0 ||
        gs.waterbodies.size() > 0 || gs.buildings.size() > 0;
}

[[nodiscard]] uint32_t ClampAxis(double value) {
    return static_cast<uint32_t>(
        std::clamp<int>(
            static_cast<int>(std::lround(value)),
            static_cast<int>(kMinGridAxis),
            static_cast<int>(kMaxGridAxis)));
}

void InitializeSpatialGrid(RenderSpatialGrid& grid, const Bounds& world_bounds, size_t estimated_entries) {
    grid.world_bounds = MakeNormalizedBounds(world_bounds);

    const double world_w = std::max(kMinWorldSpanMeters, grid.world_bounds.max.x - grid.world_bounds.min.x);
    const double world_h = std::max(kMinWorldSpanMeters, grid.world_bounds.max.y - grid.world_bounds.min.y);

    const double target_cells = std::max(1.0, std::ceil(static_cast<double>(estimated_entries) / kTargetEntriesPerCell));
    const double aspect = std::clamp(world_w / world_h, 0.1, 10.0);

    grid.width = ClampAxis(std::sqrt(target_cells * aspect));
    grid.height = ClampAxis(target_cells / std::max(1.0, static_cast<double>(grid.width)));

    grid.cell_size_x = world_w / static_cast<double>(grid.width);
    grid.cell_size_y = world_h / static_cast<double>(grid.height);
    grid.valid = true;

    const size_t cell_count = static_cast<size_t>(grid.CellCount());
    grid.roads.offsets.assign(cell_count + 1u, 0u);
    grid.districts.offsets.assign(cell_count + 1u, 0u);
    grid.lots.offsets.assign(cell_count + 1u, 0u);
    grid.water.offsets.assign(cell_count + 1u, 0u);
    grid.buildings.offsets.assign(cell_count + 1u, 0u);
    grid.roads.handles.clear();
    grid.districts.handles.clear();
    grid.lots.handles.clear();
    grid.water.handles.clear();
    grid.buildings.handles.clear();
}

[[nodiscard]] int ClampCellX(const RenderSpatialGrid& grid, int x) {
    return std::clamp(x, 0, static_cast<int>(grid.width) - 1);
}

[[nodiscard]] int ClampCellY(const RenderSpatialGrid& grid, int y) {
    return std::clamp(y, 0, static_cast<int>(grid.height) - 1);
}

[[nodiscard]] int WorldToCellXClamped(const RenderSpatialGrid& grid, double x) {
    const double rx = (x - grid.world_bounds.min.x) / grid.cell_size_x;
    return ClampCellX(grid, static_cast<int>(std::floor(rx)));
}

[[nodiscard]] int WorldToCellYClamped(const RenderSpatialGrid& grid, double y) {
    const double ry = (y - grid.world_bounds.min.y) / grid.cell_size_y;
    return ClampCellY(grid, static_cast<int>(std::floor(ry)));
}

[[nodiscard]] bool BoundsIntersectsGridWorld(const RenderSpatialGrid& grid, const Bounds& bounds) {
    return !(bounds.max.x < grid.world_bounds.min.x ||
        bounds.min.x > grid.world_bounds.max.x ||
        bounds.max.y < grid.world_bounds.min.y ||
        bounds.min.y > grid.world_bounds.max.y);
}

template <typename TCallback>
void ForEachCellCoveredByBounds(const RenderSpatialGrid& grid, const Bounds& bounds, TCallback&& callback) {
    if (!grid.IsValid() || !BoundsIntersectsGridWorld(grid, bounds)) {
        return;
    }

    const int min_x = WorldToCellXClamped(grid, bounds.min.x);
    const int max_x = WorldToCellXClamped(grid, bounds.max.x);
    const int min_y = WorldToCellYClamped(grid, bounds.min.y);
    const int max_y = WorldToCellYClamped(grid, bounds.max.y);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            callback(grid.ToCellIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y)));
        }
    }
}

[[nodiscard]] Bounds SegmentBounds(const Vec2& a, const Vec2& b) {
    Bounds bounds{};
    bounds.min.x = std::min(a.x, b.x);
    bounds.min.y = std::min(a.y, b.y);
    bounds.max.x = std::max(a.x, b.x);
    bounds.max.y = std::max(a.y, b.y);
    return bounds;
}

[[nodiscard]] Bounds PointBounds(const Vec2& p) {
    return Bounds{ p, p };
}

[[nodiscard]] Bounds PolygonBounds(const std::vector<Vec2>& points, bool& has_bounds) {
    Bounds bounds{};
    has_bounds = false;
    ExpandBoundsFromPoints(bounds, points, has_bounds);
    return has_bounds ? MakeNormalizedBounds(bounds) : bounds;
}

template <typename TCountVector>
void FinalizeLayerOffsets(TCountVector& counts, RenderSpatialLayer& layer) {
    const size_t cell_count = counts.size();
    layer.offsets.assign(cell_count + 1u, 0u);
    for (size_t i = 0; i < cell_count; ++i) {
        layer.offsets[i + 1u] = layer.offsets[i] + counts[i];
    }
    layer.handles.assign(layer.offsets.back(), 0u);
}

} // namespace

void ViewportIndexBuilder::Build(GlobalState& gs) {
    auto& index = gs.viewport_index;
    index.clear();
    const size_t expected =
        gs.districts.size() +
        gs.lots.size() +
        gs.buildings.size() +
        gs.roads.size() +
        gs.waterbodies.size();
    index.reserve(expected);

    std::unordered_map<uint32_t, std::vector<const LotToken*>> lots_by_district;
    std::unordered_map<uint32_t, std::vector<const BuildingSite*>> buildings_by_lot;

    lots_by_district.reserve(gs.lots.size());
    buildings_by_lot.reserve(gs.buildings.size());

    for (const auto& lot : gs.lots) {
        lots_by_district[lot.district_id].push_back(&lot);
    }

    for (const auto& building : gs.buildings) {
        buildings_by_lot[building.lot_id].push_back(&building);
    }

    // Districts -> Lots -> Buildings (hierarchical spans)
    for (const auto& district : gs.districts) {
        const uint32_t district_index = static_cast<uint32_t>(index.size());
        VpProbeData district_probe{};
        district_probe.kind = VpEntityKind::District;
        district_probe.id = district.id;
        district_probe.district_id = district.id;
        district_probe.zone_mask = static_cast<uint8_t>(district.type);
        district_probe.layer_id = gs.GetEntityLayer(VpEntityKind::District, district.id);
        SetLabelWithId(district_probe, "District", district.id);

        index.push_back(district_probe);

        const auto lot_it = lots_by_district.find(district.id);
        if (lot_it == lots_by_district.end()) {
            continue;
        }

        const uint32_t first_child = static_cast<uint32_t>(index.size());
        uint16_t child_count = 0;
        struct LotRecord {
            uint32_t lot_index{ 0 };
            uint32_t lot_id{ 0 };
        };
        std::vector<LotRecord> lot_records;
        lot_records.reserve(lot_it->second.size());

        for (const auto* lot : lot_it->second) {
            const uint32_t lot_index = static_cast<uint32_t>(index.size());
            VpProbeData lot_probe{};
            lot_probe.kind = VpEntityKind::Lot;
            lot_probe.id = lot->id;
            lot_probe.parent = district_index;
            lot_probe.district_id = lot->district_id;
            lot_probe.road_id = static_cast<uint32_t>(lot->primary_road);
            lot_probe.aesp = { {lot->access, lot->exposure, lot->serviceability, lot->privacy} };
            lot_probe.zone_mask = static_cast<uint8_t>(lot->lot_type);
            lot_probe.layer_id = gs.GetEntityLayer(VpEntityKind::Lot, lot->id);
            SetLabelWithId(lot_probe, "Lot", lot->id);

            index.push_back(lot_probe);
            child_count += 1;
            lot_records.push_back({ lot_index, lot->id });
        }

        if (child_count > 0) {
            index[district_index].first_child = first_child;
            index[district_index].child_count = child_count;
        }

        for (const auto& lot_record : lot_records) {
            const auto building_it = buildings_by_lot.find(lot_record.lot_id);
            if (building_it == buildings_by_lot.end()) {
                continue;
            }

            const uint32_t building_first = static_cast<uint32_t>(index.size());
            uint16_t building_count = 0;

            for (const auto* building : building_it->second) {
                VpProbeData building_probe{};
                building_probe.kind = VpEntityKind::Building;
                building_probe.id = building->id;
                building_probe.parent = lot_record.lot_index;
                building_probe.district_id = building->district_id;
                building_probe.zone_mask = static_cast<uint8_t>(building->type);
                building_probe.layer_id = gs.GetEntityLayer(VpEntityKind::Building, building->id);
                SetLabelWithId(building_probe, "Building", building->id);
                index.push_back(building_probe);
                building_count += 1;
            }

            if (building_count > 0) {
                index[lot_record.lot_index].first_child = building_first;
                index[lot_record.lot_index].child_count = building_count;
            }
        }
    }

    // Roads (top-level entries)
    for (const auto& road : gs.roads) {
        VpProbeData road_probe{};
        road_probe.kind = VpEntityKind::Road;
        road_probe.id = road.id;
        road_probe.road_id = road.id;
        road_probe.road_hierarchy = static_cast<uint8_t>(road.type);
        road_probe.layer_id = gs.GetEntityLayer(VpEntityKind::Road, road.id);
        SetLabelWithId(road_probe, "Road", road.id);
        index.push_back(road_probe);
    }

    // Water bodies (top-level entries)
    for (const auto& water : gs.waterbodies) {
        VpProbeData water_probe{};
        water_probe.kind = VpEntityKind::Water;
        water_probe.id = water.id;
        water_probe.layer_id = gs.GetEntityLayer(VpEntityKind::Water, water.id);
        SetLabelWithId(water_probe, "Water", water.id);
        index.push_back(water_probe);
    }

    RogueCity::Core::Editor::RebuildStableIDMapping(index);

    gs.road_handle_by_id.clear();
    gs.district_handle_by_id.clear();
    gs.lot_handle_by_id.clear();
    gs.water_handle_by_id.clear();
    gs.building_handle_by_id.clear();

    if (!HasRenderableEntities(gs)) {
        gs.render_spatial_grid.Clear();
        gs.render_spatial_grid.build_version += 1u;
        return;
    }

    RenderSpatialGrid& grid = gs.render_spatial_grid;
    grid.Clear();

    const Bounds world_bounds = ComputePreferredWorldBounds(gs);
    InitializeSpatialGrid(grid, world_bounds, expected);

    const size_t cell_count = static_cast<size_t>(grid.CellCount());
    std::vector<uint32_t> road_counts(cell_count, 0u);
    std::vector<uint32_t> district_counts(cell_count, 0u);
    std::vector<uint32_t> lot_counts(cell_count, 0u);
    std::vector<uint32_t> water_counts(cell_count, 0u);
    std::vector<uint32_t> building_counts(cell_count, 0u);

    gs.road_handle_by_id.reserve(gs.roads.size());
    gs.district_handle_by_id.reserve(gs.districts.size());
    gs.lot_handle_by_id.reserve(gs.lots.size());
    gs.water_handle_by_id.reserve(gs.waterbodies.size());
    gs.building_handle_by_id.reserve(gs.buildings.size());

    // Build-time dedupe to keep each road present once per cell, even when many segments overlap.
    std::vector<uint32_t> road_cell_stamp(cell_count, 0u);
    uint32_t road_stamp = 1u;

    auto next_road_stamp = [&]() {
        road_stamp += 1u;
        if (road_stamp == 0u) {
            std::fill(road_cell_stamp.begin(), road_cell_stamp.end(), 0u);
            road_stamp = 1u;
        }
    };

    size_t road_data_index = 0;
    for (const auto& road : gs.roads) {
        const uint32_t handle = static_cast<uint32_t>(gs.roads.createHandleFromData(road_data_index).getIndex());
        gs.road_handle_by_id[road.id] = handle;
        next_road_stamp();

        if (road.points.empty()) {
            ++road_data_index;
            continue;
        }

        if (road.points.size() == 1) {
            ForEachCellCoveredByBounds(grid, PointBounds(road.points.front()), [&](uint32_t cell) {
                if (road_cell_stamp[cell] == road_stamp) {
                    return;
                }
                road_cell_stamp[cell] = road_stamp;
                road_counts[cell] += 1u;
            });
            ++road_data_index;
            continue;
        }

        for (size_t i = 1; i < road.points.size(); ++i) {
            const Bounds segment_bounds = SegmentBounds(road.points[i - 1], road.points[i]);
            ForEachCellCoveredByBounds(grid, segment_bounds, [&](uint32_t cell) {
                if (road_cell_stamp[cell] == road_stamp) {
                    return;
                }
                road_cell_stamp[cell] = road_stamp;
                road_counts[cell] += 1u;
            });
        }
        ++road_data_index;
    }

    size_t district_data_index = 0;
    for (const auto& district : gs.districts) {
        const uint32_t handle = static_cast<uint32_t>(gs.districts.createHandleFromData(district_data_index).getIndex());
        gs.district_handle_by_id[district.id] = handle;

        bool has_bounds = false;
        const Bounds bounds = PolygonBounds(district.border, has_bounds);
        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) { district_counts[cell] += 1u; });
        }
        ++district_data_index;
    }

    size_t lot_data_index = 0;
    for (const auto& lot : gs.lots) {
        const uint32_t handle = static_cast<uint32_t>(gs.lots.createHandleFromData(lot_data_index).getIndex());
        gs.lot_handle_by_id[lot.id] = handle;

        bool has_bounds = false;
        Bounds bounds = PolygonBounds(lot.boundary, has_bounds);
        if (!has_bounds) {
            bounds = PointBounds(lot.centroid);
            has_bounds = true;
        }

        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) { lot_counts[cell] += 1u; });
        }
        ++lot_data_index;
    }

    size_t water_data_index = 0;
    for (const auto& water : gs.waterbodies) {
        const uint32_t handle = static_cast<uint32_t>(gs.waterbodies.createHandleFromData(water_data_index).getIndex());
        gs.water_handle_by_id[water.id] = handle;

        bool has_bounds = false;
        const Bounds bounds = PolygonBounds(water.boundary, has_bounds);
        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) { water_counts[cell] += 1u; });
        }
        ++water_data_index;
    }

    uint32_t building_index = 0u;
    for (const auto& building : gs.buildings) {
        gs.building_handle_by_id[building.id] = building_index;
        ForEachCellCoveredByBounds(grid, PointBounds(building.position), [&](uint32_t cell) {
            building_counts[cell] += 1u;
        });
        ++building_index;
    }

    // CSR memory layout remains contiguous and deterministic for future GPU upload/compute access.
    FinalizeLayerOffsets(road_counts, grid.roads);
    FinalizeLayerOffsets(district_counts, grid.districts);
    FinalizeLayerOffsets(lot_counts, grid.lots);
    FinalizeLayerOffsets(water_counts, grid.water);
    FinalizeLayerOffsets(building_counts, grid.buildings);

    std::vector<uint32_t> road_write = grid.roads.offsets;
    std::vector<uint32_t> district_write = grid.districts.offsets;
    std::vector<uint32_t> lot_write = grid.lots.offsets;
    std::vector<uint32_t> water_write = grid.water.offsets;
    std::vector<uint32_t> building_write = grid.buildings.offsets;

    std::fill(road_cell_stamp.begin(), road_cell_stamp.end(), 0u);
    road_stamp = 1u;

    road_data_index = 0;
    for (const auto& road : gs.roads) {
        const uint32_t handle = static_cast<uint32_t>(gs.roads.createHandleFromData(road_data_index).getIndex());
        next_road_stamp();

        if (road.points.empty()) {
            ++road_data_index;
            continue;
        }

        if (road.points.size() == 1) {
            ForEachCellCoveredByBounds(grid, PointBounds(road.points.front()), [&](uint32_t cell) {
                if (road_cell_stamp[cell] == road_stamp) {
                    return;
                }
                road_cell_stamp[cell] = road_stamp;
                grid.roads.handles[road_write[cell]++] = handle;
            });
            ++road_data_index;
            continue;
        }

        for (size_t i = 1; i < road.points.size(); ++i) {
            const Bounds segment_bounds = SegmentBounds(road.points[i - 1], road.points[i]);
            ForEachCellCoveredByBounds(grid, segment_bounds, [&](uint32_t cell) {
                if (road_cell_stamp[cell] == road_stamp) {
                    return;
                }
                road_cell_stamp[cell] = road_stamp;
                grid.roads.handles[road_write[cell]++] = handle;
            });
        }
        ++road_data_index;
    }

    district_data_index = 0;
    for (const auto& district : gs.districts) {
        const uint32_t handle = static_cast<uint32_t>(gs.districts.createHandleFromData(district_data_index).getIndex());
        bool has_bounds = false;
        const Bounds bounds = PolygonBounds(district.border, has_bounds);
        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) {
                grid.districts.handles[district_write[cell]++] = handle;
            });
        }
        ++district_data_index;
    }

    lot_data_index = 0;
    for (const auto& lot : gs.lots) {
        const uint32_t handle = static_cast<uint32_t>(gs.lots.createHandleFromData(lot_data_index).getIndex());
        bool has_bounds = false;
        Bounds bounds = PolygonBounds(lot.boundary, has_bounds);
        if (!has_bounds) {
            bounds = PointBounds(lot.centroid);
            has_bounds = true;
        }
        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) {
                grid.lots.handles[lot_write[cell]++] = handle;
            });
        }
        ++lot_data_index;
    }

    water_data_index = 0;
    for (const auto& water : gs.waterbodies) {
        const uint32_t handle = static_cast<uint32_t>(gs.waterbodies.createHandleFromData(water_data_index).getIndex());
        bool has_bounds = false;
        const Bounds bounds = PolygonBounds(water.boundary, has_bounds);
        if (has_bounds) {
            ForEachCellCoveredByBounds(grid, bounds, [&](uint32_t cell) {
                grid.water.handles[water_write[cell]++] = handle;
            });
        }
        ++water_data_index;
    }

    building_index = 0u;
    for (const auto& building : gs.buildings) {
        ForEachCellCoveredByBounds(grid, PointBounds(building.position), [&](uint32_t cell) {
            grid.buildings.handles[building_write[cell]++] = building_index;
        });
        ++building_index;
    }

    grid.valid = true;
    grid.build_version += 1u;
}

} // namespace RogueCity::App
