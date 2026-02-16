#include "RogueCity/App/Integration/CityOutputApplier.hpp"

#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Data/MaterialEncoding.hpp"

#include <algorithm>
#include <cstdint>
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

} // namespace

void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& gs,
    const CityOutputApplyOptions& options) {
    gs.roads.clear();
    for (const auto& road : output.roads) {
        gs.roads.add(road);
    }

    gs.districts.clear();
    for (const auto& district : output.districts) {
        gs.districts.add(district);
    }

    gs.blocks.clear();
    for (const auto& block : output.blocks) {
        gs.blocks.add(block);
    }

    gs.lots.clear();
    for (const auto& lot : output.lots) {
        gs.lots.add(lot);
    }

    gs.buildings.clear();
    for (size_t i = 0; i < output.buildings.size(); ++i) {
        gs.buildings.push_back(output.buildings[i]);
    }

    gs.generation_stats.roads_generated = static_cast<uint32_t>(gs.roads.size());
    gs.generation_stats.districts_generated = static_cast<uint32_t>(gs.districts.size());
    gs.generation_stats.lots_generated = static_cast<uint32_t>(gs.lots.size());
    gs.generation_stats.buildings_generated = static_cast<uint32_t>(gs.buildings.size());
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
