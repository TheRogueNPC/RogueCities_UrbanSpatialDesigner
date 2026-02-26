#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_set>

namespace {

using RogueCity::Core::Vec2;
using RogueCity::Core::Editor::RenderSpatialGrid;
using RogueCity::Core::Editor::RenderSpatialLayer;

bool ComputeCellRangeForWorldRect(
    const RenderSpatialGrid& grid,
    const Vec2& world_min,
    const Vec2& world_max,
    int& out_min_cell_x,
    int& out_max_cell_x,
    int& out_min_cell_y,
    int& out_max_cell_y) {
    if (!grid.IsValid()) {
        return false;
    }

    const double min_x = std::min(world_min.x, world_max.x);
    const double max_x = std::max(world_min.x, world_max.x);
    const double min_y = std::min(world_min.y, world_max.y);
    const double max_y = std::max(world_min.y, world_max.y);

    const bool overlaps_world =
        !(max_x < grid.world_bounds.min.x ||
            min_x > grid.world_bounds.max.x ||
            max_y < grid.world_bounds.min.y ||
            min_y > grid.world_bounds.max.y);
    if (!overlaps_world) {
        return false;
    }

    out_min_cell_x = std::clamp(
        static_cast<int>(std::floor((min_x - grid.world_bounds.min.x) / grid.cell_size_x)),
        0,
        static_cast<int>(grid.width) - 1);
    out_max_cell_x = std::clamp(
        static_cast<int>(std::floor((max_x - grid.world_bounds.min.x) / grid.cell_size_x)),
        0,
        static_cast<int>(grid.width) - 1);
    out_min_cell_y = std::clamp(
        static_cast<int>(std::floor((min_y - grid.world_bounds.min.y) / grid.cell_size_y)),
        0,
        static_cast<int>(grid.height) - 1);
    out_max_cell_y = std::clamp(
        static_cast<int>(std::floor((max_y - grid.world_bounds.min.y) / grid.cell_size_y)),
        0,
        static_cast<int>(grid.height) - 1);
    return true;
}

std::unordered_set<uint32_t> CollectHandlesInWorldRect(
    const RenderSpatialGrid& grid,
    const RenderSpatialLayer& layer,
    const Vec2& world_min,
    const Vec2& world_max) {
    std::unordered_set<uint32_t> handles{};
    int min_cell_x = 0;
    int max_cell_x = 0;
    int min_cell_y = 0;
    int max_cell_y = 0;
    if (!ComputeCellRangeForWorldRect(
            grid, world_min, world_max, min_cell_x, max_cell_x, min_cell_y, max_cell_y)) {
        return handles;
    }

    for (int y = min_cell_y; y <= max_cell_y; ++y) {
        for (int x = min_cell_x; x <= max_cell_x; ++x) {
            const uint32_t cell = grid.ToCellIndex(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
            const uint32_t begin = layer.offsets[cell];
            const uint32_t end = layer.offsets[cell + 1u];
            for (uint32_t cursor = begin; cursor < end; ++cursor) {
                handles.insert(layer.handles[cursor]);
            }
        }
    }
    return handles;
}

bool CellContainsHandle(
    const RenderSpatialGrid& grid,
    const RenderSpatialLayer& layer,
    uint32_t cell_index,
    uint32_t handle) {
    const uint32_t begin = layer.offsets[cell_index];
    const uint32_t end = layer.offsets[cell_index + 1u];
    for (uint32_t cursor = begin; cursor < end; ++cursor) {
        if (layer.handles[cursor] == handle) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    using RogueCity::Core::BuildingSite;
    using RogueCity::Core::District;
    using RogueCity::Core::LotToken;
    using RogueCity::Core::Road;
    using RogueCity::Core::RoadType;
    using RogueCity::Core::WaterBody;
    using RogueCity::Core::WaterType;
    using RogueCity::Core::Editor::GlobalState;

    GlobalState gs{};
    gs.city_boundary = {
        Vec2(0.0, 0.0),
        Vec2(1000.0, 0.0),
        Vec2(1000.0, 1000.0),
        Vec2(0.0, 1000.0)
    };

    Road long_road{};
    long_road.id = 11u;
    long_road.type = RoadType::Arterial;
    long_road.points = { Vec2(20.0, 100.0), Vec2(980.0, 100.0) };
    gs.roads.add(long_road);

    Road local_road{};
    local_road.id = 12u;
    local_road.type = RoadType::Lane;
    local_road.points = { Vec2(760.0, 760.0), Vec2(900.0, 900.0) };
    gs.roads.add(local_road);

    District district{};
    district.id = 21u;
    district.border = {
        Vec2(150.0, 150.0),
        Vec2(320.0, 150.0),
        Vec2(320.0, 320.0),
        Vec2(150.0, 320.0)
    };
    gs.districts.add(district);

    LotToken lot{};
    lot.id = 31u;
    lot.district_id = district.id;
    lot.centroid = Vec2(230.0, 230.0);
    lot.boundary = {
        Vec2(200.0, 200.0),
        Vec2(260.0, 200.0),
        Vec2(260.0, 260.0),
        Vec2(200.0, 260.0)
    };
    gs.lots.add(lot);

    WaterBody water{};
    water.id = 41u;
    water.type = WaterType::Lake;
    water.boundary = {
        Vec2(680.0, 180.0),
        Vec2(760.0, 190.0),
        Vec2(740.0, 260.0),
        Vec2(670.0, 250.0)
    };
    gs.waterbodies.add(water);

    BuildingSite building{};
    building.id = 51u;
    building.position = Vec2(230.0, 230.0);
    building.lot_id = lot.id;
    building.district_id = district.id;
    gs.buildings.push_back(building);

    RogueCity::App::ViewportIndexBuilder::Build(gs);

    assert(gs.render_spatial_grid.IsValid());
    assert(!gs.viewport_index.empty());
    assert(gs.road_handle_by_id.contains(11u));
    assert(gs.district_handle_by_id.contains(21u));
    assert(gs.lot_handle_by_id.contains(31u));
    assert(gs.water_handle_by_id.contains(41u));
    assert(gs.building_handle_by_id.contains(51u));

    const auto& grid = gs.render_spatial_grid;
    const uint32_t long_road_handle = gs.road_handle_by_id.at(11u);
    int start_x = 0;
    int start_y = 0;
    int end_x = 0;
    int end_y = 0;
    assert(grid.WorldToCell(long_road.points.front(), start_x, start_y));
    assert(grid.WorldToCell(long_road.points.back(), end_x, end_y));
    assert(start_x != end_x);

    const uint32_t start_cell = grid.ToCellIndex(static_cast<uint32_t>(start_x), static_cast<uint32_t>(start_y));
    const uint32_t end_cell = grid.ToCellIndex(static_cast<uint32_t>(end_x), static_cast<uint32_t>(end_y));
    assert(CellContainsHandle(grid, grid.roads, start_cell, long_road_handle));
    assert(CellContainsHandle(grid, grid.roads, end_cell, long_road_handle));

    // Known world window should include district/lot/building handles around centroid.
    const auto district_handles = CollectHandlesInWorldRect(
        grid, grid.districts, Vec2(190.0, 190.0), Vec2(280.0, 280.0));
    const auto lot_handles = CollectHandlesInWorldRect(
        grid, grid.lots, Vec2(190.0, 190.0), Vec2(280.0, 280.0));
    const auto building_handles = CollectHandlesInWorldRect(
        grid, grid.buildings, Vec2(190.0, 190.0), Vec2(280.0, 280.0));

    assert(district_handles.contains(gs.district_handle_by_id.at(21u)));
    assert(lot_handles.contains(gs.lot_handle_by_id.at(31u)));
    assert(building_handles.contains(gs.building_handle_by_id.at(51u)));

    // Rebuild after an edit should update membership.
    const auto old_lot_handles = CollectHandlesInWorldRect(
        grid, grid.lots, Vec2(190.0, 190.0), Vec2(280.0, 280.0));
    assert(old_lot_handles.contains(gs.lot_handle_by_id.at(31u)));

    const uint32_t lot_handle = gs.lot_handle_by_id.at(31u);
    assert(gs.lots.isValidIndex(lot_handle));
    gs.lots[lot_handle].centroid = Vec2(860.0, 140.0);
    gs.lots[lot_handle].boundary = {
        Vec2(830.0, 110.0),
        Vec2(890.0, 110.0),
        Vec2(890.0, 170.0),
        Vec2(830.0, 170.0)
    };

    RogueCity::App::ViewportIndexBuilder::Build(gs);
    const auto& rebuilt_grid = gs.render_spatial_grid;
    const uint32_t rebuilt_lot_handle = gs.lot_handle_by_id.at(31u);

    const auto old_window_after_rebuild = CollectHandlesInWorldRect(
        rebuilt_grid, rebuilt_grid.lots, Vec2(190.0, 190.0), Vec2(280.0, 280.0));
    const auto new_window_after_rebuild = CollectHandlesInWorldRect(
        rebuilt_grid, rebuilt_grid.lots, Vec2(820.0, 100.0), Vec2(900.0, 180.0));

    assert(!old_window_after_rebuild.contains(rebuilt_lot_handle));
    assert(new_window_after_rebuild.contains(rebuilt_lot_handle));

    return 0;
}
