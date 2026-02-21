#include "RogueCity/Generators/Urban/SiteGenerator.hpp"

#include <algorithm>

namespace RogueCity::Generators::Urban {

    namespace {

        // Maps lot program to primary building archetype.
        Core::BuildingType mapBuildingType(Core::LotType lot_type) {
            switch (lot_type) {
                case Core::LotType::Residential: return Core::BuildingType::Residential;
                case Core::LotType::RowhomeCompact: return Core::BuildingType::Rowhome;
                case Core::LotType::RetailStrip: return Core::BuildingType::Retail;
                case Core::LotType::MixedUse: return Core::BuildingType::MixedUse;
                case Core::LotType::LogisticsIndustrial: return Core::BuildingType::Industrial;
                case Core::LotType::CivicCultural: return Core::BuildingType::Civic;
                case Core::LotType::LuxuryScenic: return Core::BuildingType::Luxury;
                case Core::LotType::BufferStrip: return Core::BuildingType::Utility;
                default: return Core::BuildingType::None;
            }
        }

        // Coarse cost model per building archetype.
        float estimateCost(Core::BuildingType type) {
            switch (type) {
                case Core::BuildingType::Industrial: return 3.0f;
                case Core::BuildingType::Civic: return 4.0f;
                case Core::BuildingType::Luxury: return 8.0f;
                case Core::BuildingType::Retail:
                case Core::BuildingType::MixedUse:
                    return 2.0f;
                case Core::BuildingType::Rowhome:
                case Core::BuildingType::Residential:
                case Core::BuildingType::Utility:
                default:
                    return 1.0f;
            }
        }

        // Deterministic per-lot building count heuristic.
        int buildingsPerLot(const Core::LotToken& lot, Core::RNG& rng) {
            switch (lot.lot_type) {
                case Core::LotType::RowhomeCompact: return rng.uniformInt(2, 5);
                case Core::LotType::MixedUse: return rng.uniformInt(1, 2);
                case Core::LotType::RetailStrip: return rng.uniformInt(1, 2);
                case Core::LotType::BufferStrip: return (rng.uniform() < 0.35) ? 1 : 0;
                default: return 1;
            }
        }

    } // namespace

    // Emits building sites for lots with deterministic RNG seeded by lot ID.
    // Positions are centroid-based with optional jitter constrained by lot boundary extents.
    siv::Vector<Core::BuildingSite> SiteGenerator::generate(
        const std::vector<Core::LotToken>& lots,
        const Config& config,
        uint32_t seed) {
        siv::Vector<Core::BuildingSite> buildings;
        buildings.reserve(std::min<uint32_t>(config.max_buildings, static_cast<uint32_t>(lots.size() * 2)));

        uint32_t id = 1;
        for (const auto& lot : lots) {
            if (id > config.max_buildings) {
                break;
            }

            Core::RNG rng(seed ^ (lot.id * 2654435761u));
            const int n = buildingsPerLot(lot, rng);
            const Core::BuildingType type = mapBuildingType(lot.lot_type);
            if (type == Core::BuildingType::None) {
                continue;
            }

            // Emit one or more sites for this lot depending on archetype.
            for (int i = 0; i < n && id <= config.max_buildings; ++i) {
                Core::BuildingSite site;
                site.id = id++;
                site.lot_id = lot.id;
                site.district_id = lot.district_id;
                site.type = type;
                site.estimated_cost = estimateCost(type);

                Core::Vec2 pos = lot.centroid;
                if (!lot.boundary.empty()) {
                    const auto& a = lot.boundary.front();
                    const auto& c = lot.boundary[2 % lot.boundary.size()];
                    const double half_w = std::abs(c.x - a.x) * 0.5;
                    const double half_h = std::abs(c.y - a.y) * 0.5;
                    const double j = std::clamp(static_cast<double>(config.jitter), 0.0, 0.95);
                    pos.x += rng.uniform(-half_w * j, half_w * j);
                    pos.y += rng.uniform(-half_h * j, half_h * j);
                }
                site.position = pos;
                buildings.push_back(site);
            }
        }

        return buildings;
    }

} // namespace RogueCity::Generators::Urban
