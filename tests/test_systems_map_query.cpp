#include "ui/panels/rc_system_map_query.h"

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cassert>

int main() {
    using RC_UI::Panels::SystemMap::BuildSystemsMapBounds;
    using RC_UI::Panels::SystemMap::QuerySystemsMapEntity;
    using RogueCity::Core::BuildingSite;
    using RogueCity::Core::District;
    using RogueCity::Core::LotToken;
    using RogueCity::Core::Road;
    using RogueCity::Core::Vec2;
    using RogueCity::Core::WaterBody;
    using RogueCity::Core::Editor::GlobalState;
    using RogueCity::Core::Editor::VpEntityKind;

    GlobalState gs{};

    gs.world_constraints.resize(100, 80, 10.0);

    Road road{};
    road.id = 11u;
    road.points = {Vec2(0.0, 5.0), Vec2(10.0, 5.0)};
    gs.roads.add(road);

    District district{};
    district.id = 12u;
    district.border = {
        Vec2(18.0, 18.0),
        Vec2(22.0, 18.0),
        Vec2(22.0, 22.0),
        Vec2(18.0, 22.0)
    };
    gs.districts.add(district);

    LotToken lot{};
    lot.id = 13u;
    lot.centroid = Vec2(30.0, 30.0);
    lot.boundary = {
        Vec2(28.0, 28.0),
        Vec2(32.0, 28.0),
        Vec2(32.0, 32.0),
        Vec2(28.0, 32.0)
    };
    gs.lots.add(lot);

    BuildingSite building{};
    building.id = 14u;
    building.position = Vec2(40.0, 40.0);
    gs.buildings.push_back(building);

    WaterBody water{};
    water.id = 15u;
    water.boundary = {Vec2(50.0, 50.0), Vec2(55.0, 50.0), Vec2(60.0, 50.0)};
    gs.waterbodies.add(water);

    RogueCity::Core::Bounds bounds{};
    const bool has_bounds = BuildSystemsMapBounds(gs, bounds);
    assert(has_bounds);
    assert(bounds.min.x == 0.0);
    assert(bounds.min.y == 0.0);
    assert(bounds.max.x == 1000.0);
    assert(bounds.max.y == 800.0);

    auto runtime = gs.systems_map;
    runtime.show_roads = true;
    runtime.show_districts = false;
    runtime.show_lots = false;
    runtime.show_buildings = false;
    runtime.show_water = false;

    auto hit = QuerySystemsMapEntity(gs, runtime, Vec2(5.0, 5.0), 2.0f);
    assert(hit.valid);
    assert(hit.kind == VpEntityKind::Road);
    assert(hit.id == 11u);

    runtime.show_roads = false;
    runtime.show_districts = true;
    hit = QuerySystemsMapEntity(gs, runtime, Vec2(20.0, 20.0), 5.0f);
    assert(hit.valid);
    assert(hit.kind == VpEntityKind::District);
    assert(hit.id == 12u);

    runtime.show_districts = false;
    runtime.show_lots = true;
    hit = QuerySystemsMapEntity(gs, runtime, Vec2(30.0, 30.0), 5.0f);
    assert(hit.valid);
    assert(hit.kind == VpEntityKind::Lot);
    assert(hit.id == 13u);

    runtime.show_lots = false;
    runtime.show_buildings = true;
    hit = QuerySystemsMapEntity(gs, runtime, Vec2(40.0, 40.0), 2.0f);
    assert(hit.valid);
    assert(hit.kind == VpEntityKind::Building);
    assert(hit.id == 14u);

    runtime.show_buildings = false;
    runtime.show_water = true;
    hit = QuerySystemsMapEntity(gs, runtime, Vec2(55.0, 50.0), 2.0f);
    assert(hit.valid);
    assert(hit.kind == VpEntityKind::Water);
    assert(hit.id == 15u);

    runtime.show_water = false;
    hit = QuerySystemsMapEntity(gs, runtime, Vec2(55.0, 50.0), 2.0f);
    assert(!hit.valid);

    return 0;
}
