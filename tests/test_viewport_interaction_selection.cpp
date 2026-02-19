#include "ui/viewport/handlers/rc_viewport_non_axiom_pipeline.h"

#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"

#include <cassert>

int main() {
    using RogueCity::Core::BuildingSite;
    using RogueCity::Core::District;
    using RogueCity::Core::LotToken;
    using RogueCity::Core::Road;
    using RogueCity::Core::Vec2;
    using RogueCity::Core::Editor::SelectionItem;
    using RogueCity::Core::Editor::VpEntityKind;

    RogueCity::Core::Editor::GlobalState gs{};

    District district{};
    district.id = 101u;
    district.border = {
        Vec2(10.0, 10.0),
        Vec2(40.0, 10.0),
        Vec2(40.0, 40.0),
        Vec2(10.0, 40.0)
    };
    gs.districts.add(district);

    LotToken lot{};
    lot.id = 201u;
    lot.district_id = district.id;
    lot.centroid = Vec2(22.0, 22.0);
    lot.boundary = {
        Vec2(18.0, 18.0),
        Vec2(30.0, 18.0),
        Vec2(30.0, 30.0),
        Vec2(18.0, 30.0)
    };
    gs.lots.add(lot);

    Road road{};
    road.id = 301u;
    road.points = {
        Vec2(5.0, 22.0),
        Vec2(45.0, 22.0)
    };
    gs.roads.add(road);

    BuildingSite building{};
    building.id = 401u;
    building.position = Vec2(22.0, 22.0);
    building.lot_id = lot.id;
    building.district_id = district.id;
    gs.buildings.push_back(building);

    RogueCity::App::ViewportIndexBuilder::Build(gs);

    const auto metrics = RC_UI::Tools::BuildToolInteractionMetrics(1.0f, 1.0f);

    // Pick priority: building should win over lot/district at same anchor.
    const auto pick_building = RC_UI::Viewport::PickFromViewportIndexTestHook(gs, Vec2(22.0, 22.0), metrics);
    assert(pick_building.has_value());
    assert(pick_building->kind == VpEntityKind::Building);
    assert(pick_building->id == 401u);

    // Hidden-layer filtering: hide building layer and expect lot next.
    gs.SetEntityLayer(VpEntityKind::Building, 401u, 1u);
    gs.layer_manager.layers[1].visible = false;
    RogueCity::App::ViewportIndexBuilder::Build(gs);

    const auto pick_hidden_filtered = RC_UI::Viewport::PickFromViewportIndexTestHook(gs, Vec2(22.0, 22.0), metrics);
    assert(pick_hidden_filtered.has_value());
    assert(pick_hidden_filtered->kind != VpEntityKind::Building);

    // Region query hidden-layer handling.
    const auto visible_only = RC_UI::Viewport::QueryRegionFromViewportIndexTestHook(
        gs,
        Vec2(0.0, 0.0),
        Vec2(50.0, 50.0),
        false);
    const auto include_hidden = RC_UI::Viewport::QueryRegionFromViewportIndexTestHook(
        gs,
        Vec2(0.0, 0.0),
        Vec2(50.0, 50.0),
        true);

    bool found_hidden_building_visible_only = false;
    bool found_hidden_building_include_hidden = false;
    for (const SelectionItem& item : visible_only) {
        if (item.kind == VpEntityKind::Building && item.id == 401u) {
            found_hidden_building_visible_only = true;
        }
    }
    for (const SelectionItem& item : include_hidden) {
        if (item.kind == VpEntityKind::Building && item.id == 401u) {
            found_hidden_building_include_hidden = true;
        }
    }

    assert(!found_hidden_building_visible_only);
    assert(found_hidden_building_include_hidden);

    // Selection synchronization.
    gs.selection_manager.Select(VpEntityKind::Road, 301u);
    RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
    assert(gs.selection.selected_road);
    assert(gs.selection.selected_road->id == 301u);

    gs.selection_manager.Select(VpEntityKind::Building, 401u);
    RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
    assert(gs.selection.selected_building);
    assert(gs.selection.selected_building->id == 401u);

    return 0;
}
