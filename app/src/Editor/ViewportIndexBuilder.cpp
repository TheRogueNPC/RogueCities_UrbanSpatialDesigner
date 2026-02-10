// FILE: ViewportIndexBuilder.cpp
// PURPOSE: Build the viewport index from GlobalState entities.
#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/ViewportIndex.hpp"

#include <cstdio>
#include <unordered_map>
#include <vector>

namespace RogueCity::App {

namespace {

using RogueCity::Core::Editor::VpProbeData;
using RogueCity::Core::Editor::VpEntityKind;
using RogueCity::Core::Editor::SetViewportLabel;

static void SetLabelWithId(VpProbeData& probe, const char* prefix, uint32_t id) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%s %u", prefix, id);
    SetViewportLabel(probe, buffer);
}

} // namespace

void ViewportIndexBuilder::Build(RogueCity::Core::Editor::GlobalState& gs) {
    auto& index = gs.viewport_index;
    index.clear();
    const size_t expected =
        gs.districts.size() +
        gs.lots.size() +
        gs.buildings.size() +
        gs.roads.size() +
        gs.waterbodies.size();
    index.reserve(expected);

    std::unordered_map<uint32_t, std::vector<const RogueCity::Core::LotToken*>> lots_by_district;
    std::unordered_map<uint32_t, std::vector<const RogueCity::Core::BuildingSite*>> buildings_by_lot;

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
        SetLabelWithId(road_probe, "Road", road.id);
        index.push_back(road_probe);
    }

    // Water bodies (top-level entries)
    for (const auto& water : gs.waterbodies) {
        VpProbeData water_probe{};
        water_probe.kind = VpEntityKind::Water;
        water_probe.id = water.id;
        SetLabelWithId(water_probe, "Water", water.id);
        index.push_back(water_probe);
    }
}

} // namespace RogueCity::App
