#include "RogueCity/App/Integration/CityOutputApplier.hpp"

#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Data/MaterialEncoding.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RogueCity::App {
namespace {

bool PointInPolygon(const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& a = polygon[i];
        const auto& b = polygon[j];
        const bool intersects = ((a.y > point.y) != (b.y > point.y)) &&
            (point.x < (b.x - a.x) * (point.y - a.y) / ((b.y - a.y) + 1e-12) + a.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

void NormalizeUserMetadata(RogueCity::Core::Road& road) {
    if (road.is_user_created && road.generation_tag != RogueCity::Core::GenerationTag::M_user) {
        road.generation_tag = RogueCity::Core::GenerationTag::M_user;
        road.generation_locked = true;
    }
    if (road.generation_tag == RogueCity::Core::GenerationTag::M_user) {
        road.is_user_created = true;
    }
}

void NormalizeUserMetadata(RogueCity::Core::District& district) {
    if (district.is_user_placed && district.generation_tag != RogueCity::Core::GenerationTag::M_user) {
        district.generation_tag = RogueCity::Core::GenerationTag::M_user;
        district.generation_locked = true;
    }
    if (district.generation_tag == RogueCity::Core::GenerationTag::M_user) {
        district.is_user_placed = true;
    }
}

void NormalizeUserMetadata(RogueCity::Core::LotToken& lot) {
    if (lot.is_user_placed && lot.generation_tag != RogueCity::Core::GenerationTag::M_user) {
        lot.generation_tag = RogueCity::Core::GenerationTag::M_user;
        lot.generation_locked = true;
    }
    if (lot.generation_tag == RogueCity::Core::GenerationTag::M_user) {
        lot.is_user_placed = true;
    }
}

void NormalizeUserMetadata(RogueCity::Core::BuildingSite& building) {
    if (building.is_user_placed && building.generation_tag != RogueCity::Core::GenerationTag::M_user) {
        building.generation_tag = RogueCity::Core::GenerationTag::M_user;
        building.generation_locked = true;
    }
    if (building.generation_tag == RogueCity::Core::GenerationTag::M_user) {
        building.is_user_placed = true;
    }
}

void NormalizeUserMetadata(RogueCity::Core::WaterBody& water) {
    if (water.is_user_placed && water.generation_tag != RogueCity::Core::GenerationTag::M_user) {
        water.generation_tag = RogueCity::Core::GenerationTag::M_user;
        water.generation_locked = true;
    }
    if (water.generation_tag == RogueCity::Core::GenerationTag::M_user) {
        water.is_user_placed = true;
    }
}

template <typename T>
uint32_t AssignEntityId(T& entity,
                        std::unordered_set<uint32_t>& used_ids,
                        uint32_t* next_id,
                        std::unordered_map<uint32_t, uint32_t>* id_remap = nullptr) {
    const uint32_t original_id = entity.id;
    uint32_t assigned_id = entity.id;
    if (assigned_id == 0u) {
        assigned_id = std::max<uint32_t>(1u, *next_id);
    }
    while (used_ids.find(assigned_id) != used_ids.end()) {
        assigned_id += 1u;
    }
    entity.id = assigned_id;
    used_ids.insert(assigned_id);
    *next_id = std::max<uint32_t>(*next_id, assigned_id + 1u);
    if (id_remap != nullptr) {
        (*id_remap)[original_id] = assigned_id;
    }
    return assigned_id;
}

template <typename T>
void MarkGenerated(T& entity) {
    entity.generation_tag = RogueCity::Core::GenerationTag::Generated;
    entity.generation_locked = false;
}

} // namespace

void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& gs,
    const CityOutputApplyOptions& options) {
    std::vector<RogueCity::Core::Road> locked_user_roads;
    std::vector<RogueCity::Core::District> locked_user_districts;
    std::vector<RogueCity::Core::LotToken> locked_user_lots;
    std::vector<RogueCity::Core::BuildingSite> locked_user_buildings;

    if (options.preserve_locked_user_entities) {
        locked_user_roads.reserve(gs.roads.size());
        for (auto& road : gs.roads) {
            NormalizeUserMetadata(road);
            if (road.generation_tag == RogueCity::Core::GenerationTag::M_user && road.generation_locked) {
                locked_user_roads.push_back(road);
            }
        }

        locked_user_districts.reserve(gs.districts.size());
        for (auto& district : gs.districts) {
            NormalizeUserMetadata(district);
            if (district.generation_tag == RogueCity::Core::GenerationTag::M_user && district.generation_locked) {
                locked_user_districts.push_back(district);
            }
        }

        locked_user_lots.reserve(gs.lots.size());
        for (auto& lot : gs.lots) {
            NormalizeUserMetadata(lot);
            if (lot.generation_tag == RogueCity::Core::GenerationTag::M_user && lot.generation_locked) {
                locked_user_lots.push_back(lot);
            }
        }

        locked_user_buildings.reserve(gs.buildings.size());
        for (auto& building : gs.buildings) {
            NormalizeUserMetadata(building);
            if (building.generation_tag == RogueCity::Core::GenerationTag::M_user && building.generation_locked) {
                locked_user_buildings.push_back(building);
            }
        }
    }

    std::vector<RogueCity::Core::Road> generated_roads;
    generated_roads.reserve(output.roads.size());
    for (auto road : output.roads) {
        road.is_user_created = false;
        MarkGenerated(road);
        generated_roads.push_back(std::move(road));
    }

    std::vector<RogueCity::Core::District> generated_districts;
    generated_districts.reserve(output.districts.size());
    for (auto district : output.districts) {
        district.is_user_placed = false;
        MarkGenerated(district);
        generated_districts.push_back(std::move(district));
    }

    std::vector<RogueCity::Core::LotToken> generated_lots;
    generated_lots.reserve(output.lots.size());
    for (auto lot : output.lots) {
        lot.is_user_placed = false;
        MarkGenerated(lot);
        generated_lots.push_back(std::move(lot));
    }

    std::vector<RogueCity::Core::BuildingSite> generated_buildings;
    generated_buildings.reserve(output.buildings.size());
    for (auto building : output.buildings) {
        building.is_user_placed = false;
        MarkGenerated(building);
        generated_buildings.push_back(std::move(building));
    }

    std::unordered_set<uint32_t> used_road_ids;
    uint32_t next_road_id = 1u;
    for (const auto& road : locked_user_roads) {
        if (road.id > 0u) {
            used_road_ids.insert(road.id);
            next_road_id = std::max(next_road_id, road.id + 1u);
        }
    }
    for (auto& road : generated_roads) {
        AssignEntityId(road, used_road_ids, &next_road_id);
    }

    std::unordered_map<uint32_t, uint32_t> district_id_remap;
    std::unordered_set<uint32_t> used_district_ids;
    uint32_t next_district_id = 1u;
    for (const auto& district : locked_user_districts) {
        if (district.id > 0u) {
            used_district_ids.insert(district.id);
            next_district_id = std::max(next_district_id, district.id + 1u);
        }
    }
    for (auto& district : generated_districts) {
        AssignEntityId(district, used_district_ids, &next_district_id, &district_id_remap);
    }

    for (auto& lot : generated_lots) {
        auto district_it = district_id_remap.find(lot.district_id);
        if (district_it != district_id_remap.end()) {
            lot.district_id = district_it->second;
        }
    }

    std::unordered_map<uint32_t, uint32_t> lot_id_remap;
    std::unordered_set<uint32_t> used_lot_ids;
    uint32_t next_lot_id = 1u;
    for (const auto& lot : locked_user_lots) {
        if (lot.id > 0u) {
            used_lot_ids.insert(lot.id);
            next_lot_id = std::max(next_lot_id, lot.id + 1u);
        }
    }
    for (auto& lot : generated_lots) {
        AssignEntityId(lot, used_lot_ids, &next_lot_id, &lot_id_remap);
    }

    for (auto& building : generated_buildings) {
        auto district_it = district_id_remap.find(building.district_id);
        if (district_it != district_id_remap.end()) {
            building.district_id = district_it->second;
        }
        auto lot_it = lot_id_remap.find(building.lot_id);
        if (lot_it != lot_id_remap.end()) {
            building.lot_id = lot_it->second;
        }
    }

    std::unordered_set<uint32_t> used_building_ids;
    uint32_t next_building_id = 1u;
    for (const auto& building : locked_user_buildings) {
        if (building.id > 0u) {
            used_building_ids.insert(building.id);
            next_building_id = std::max(next_building_id, building.id + 1u);
        }
    }
    for (auto& building : generated_buildings) {
        AssignEntityId(building, used_building_ids, &next_building_id);
    }

    gs.roads.clear();
    for (const auto& road : generated_roads) {
        gs.roads.add(road);
    }
    for (const auto& road : locked_user_roads) {
        gs.roads.add(road);
    }

    gs.districts.clear();
    for (const auto& district : generated_districts) {
        gs.districts.add(district);
    }
    for (const auto& district : locked_user_districts) {
        gs.districts.add(district);
    }

    gs.blocks.clear();
    for (const auto& block : output.blocks) {
        gs.blocks.add(block);
    }

    gs.lots.clear();
    for (const auto& lot : generated_lots) {
        gs.lots.add(lot);
    }
    for (const auto& lot : locked_user_lots) {
        gs.lots.add(lot);
    }

    gs.buildings.clear();
    for (const auto& building : generated_buildings) {
        gs.buildings.push_back(building);
    }
    for (const auto& building : locked_user_buildings) {
        gs.buildings.push_back(building);
    }

    gs.generation_stats.roads_generated = static_cast<uint32_t>(generated_roads.size());
    gs.generation_stats.districts_generated = static_cast<uint32_t>(generated_districts.size());
    gs.generation_stats.lots_generated = static_cast<uint32_t>(generated_lots.size());
    gs.generation_stats.buildings_generated = static_cast<uint32_t>(generated_buildings.size());
    gs.world_constraints = output.world_constraints;
    gs.site_profile = output.site_profile;
    gs.plan_violations = output.plan_violations;
    gs.plan_approved = output.plan_approved;

    RogueCity::Core::Bounds world_bounds{};
    world_bounds.min = RogueCity::Core::Vec2(0.0, 0.0);
    if (output.world_constraints.isValid()) {
        world_bounds.max = RogueCity::Core::Vec2(
            static_cast<double>(output.world_constraints.width) * output.world_constraints.cell_size,
            static_cast<double>(output.world_constraints.height) * output.world_constraints.cell_size);
    } else {
        world_bounds.max = RogueCity::Core::Vec2(
            static_cast<double>(output.tensor_field.getWidth()) * output.tensor_field.getCellSize(),
            static_cast<double>(output.tensor_field.getHeight()) * output.tensor_field.getCellSize());
    }

    const int resolution = output.world_constraints.isValid()
        ? std::max(output.world_constraints.width, output.world_constraints.height)
        : std::max(output.tensor_field.getWidth(), output.tensor_field.getHeight());
    gs.InitializeTextureSpace(world_bounds, std::max(1, resolution));

    if (gs.HasTextureSpace()) {
        auto& texture_space = gs.TextureSpaceRef();
        if (output.world_constraints.isValid()) {
            auto& height_layer = texture_space.heightLayer();
            auto& material_layer = texture_space.materialLayer();
            auto& distance_layer = texture_space.distanceLayer();
            const auto& coords = texture_space.coordinateSystem();
            for (int y = 0; y < height_layer.height(); ++y) {
                for (int x = 0; x < height_layer.width(); ++x) {
                    const RogueCity::Core::Vec2 world = coords.pixelToWorld({x, y});
                    height_layer.at(x, y) = output.world_constraints.sampleHeightMeters(world);
                    material_layer.at(x, y) = RogueCity::Core::Data::EncodeMaterialSample(
                        output.world_constraints.sampleFloodMask(world),
                        output.world_constraints.sampleNoBuild(world));
                    distance_layer.at(x, y) = output.world_constraints.sampleSlopeDegrees(world);
                }
            }
            gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Height);
            gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Material);
            gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Distance);
        }

        output.tensor_field.writeToTextureSpace(texture_space);
        gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Tensor);

        auto& zone_layer = texture_space.zoneLayer();
        zone_layer.fill(0u);
        const auto& coords = texture_space.coordinateSystem();
        for (const auto& district : output.districts) {
            if (district.border.size() < 3) {
                continue;
            }

            RogueCity::Core::Bounds district_bounds{};
            district_bounds.min = district.border.front();
            district_bounds.max = district.border.front();
            for (const auto& p : district.border) {
                district_bounds.min.x = std::min(district_bounds.min.x, p.x);
                district_bounds.min.y = std::min(district_bounds.min.y, p.y);
                district_bounds.max.x = std::max(district_bounds.max.x, p.x);
                district_bounds.max.y = std::max(district_bounds.max.y, p.y);
            }

            const auto pmin = coords.worldToPixel(district_bounds.min);
            const auto pmax = coords.worldToPixel(district_bounds.max);
            const int x0 = std::clamp(std::min(pmin.x, pmax.x), 0, zone_layer.width() - 1);
            const int x1 = std::clamp(std::max(pmin.x, pmax.x), 0, zone_layer.width() - 1);
            const int y0 = std::clamp(std::min(pmin.y, pmax.y), 0, zone_layer.height() - 1);
            const int y1 = std::clamp(std::max(pmin.y, pmax.y), 0, zone_layer.height() - 1);
            const uint8_t zone_value = static_cast<uint8_t>(static_cast<uint8_t>(district.type) + 1u);

            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    const RogueCity::Core::Vec2 world = coords.pixelToWorld({x, y});
                    if (PointInPolygon(world, district.border)) {
                        zone_layer.at(x, y) = zone_value;
                    }
                }
            }
        }
        gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Zone);
    }

    gs.entity_layers.clear();
    if (options.rebuild_viewport_index) {
        RogueCity::App::ViewportIndexBuilder::Build(gs);
    }
    if (options.mark_dirty_layers_clean) {
        gs.dirty_layers.MarkAllClean();
    }
}

} // namespace RogueCity::App
