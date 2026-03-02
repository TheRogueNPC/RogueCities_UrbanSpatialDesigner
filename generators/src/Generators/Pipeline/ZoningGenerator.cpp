#include "RogueCity/Generators/Pipeline/ZoningGenerator.hpp"
#include "RogueCity/Core/Geometry/PolygonOps.hpp"
#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace RogueCity::Generators {

    using namespace Core;
    namespace {

    // Applies optional CitySpec overrides onto runtime zoning config.
    // All values are clamped/sanitized to preserve valid operating ranges.
    ZoningGenerator::Config ApplyCitySpecOverrides(
        ZoningGenerator::Config config,
        const std::optional<CitySpec>& city_spec) {
        if (!city_spec.has_value()) {
            return config;
        }

        const CitySpec& spec = *city_spec;
        if (spec.seed != 0) {
            config.seed = spec.seed;
        }

        const auto& z = spec.zoningConstraints;
        config.minLotWidth = std::max(1.0f, z.minLotWidth);
        config.maxLotWidth = std::max(config.minLotWidth, z.maxLotWidth);
        config.minLotDepth = std::max(1.0f, z.minLotDepth);
        config.maxLotDepth = std::max(config.minLotDepth, z.maxLotDepth);
        config.minLotArea = std::max(1.0f, z.minLotArea);
        config.maxLotArea = std::max(config.minLotArea, z.maxLotArea);
        config.frontSetback = std::max(0.0f, z.frontSetback);
        config.sideSetback = std::max(0.0f, z.sideSetback);
        config.rearSetback = std::max(0.0f, z.rearSetback);
        config.residentialDensity = std::clamp(z.residentialDensity, 0.05f, 1.0f);
        config.commercialDensity = std::clamp(z.commercialDensity, 0.05f, 1.0f);
        config.industrialDensity = std::clamp(z.industrialDensity, 0.05f, 1.0f);
        config.civicDensity = std::clamp(z.civicDensity, 0.05f, 1.0f);
        config.allowRecursiveSubdivision = z.allowRecursiveSubdivision;
        config.maxSubdivisionDepth = std::max<uint32_t>(1, z.maxSubdivisionDepth);

        const auto& budget = spec.buildingBudget;
        config.totalBuildingBudget = std::max(0.0f, budget.totalBudget);
        config.residentialCost = std::max(0.01f, budget.residentialCost);
        config.commercialCost = std::max(0.01f, budget.commercialCost);
        config.industrialCost = std::max(0.01f, budget.industrialCost);
        config.civicCost = std::max(0.01f, budget.civicCost);
        config.luxuryCost = std::max(0.01f, budget.luxuryCost);

        const auto& pop = spec.populationConfig;
        config.residentialDensityPop = std::max(0.0f, pop.residentialDensity);
        config.mixedUseDensityPop = std::max(0.0f, pop.mixedUseDensity);
        config.rowhomeDensityPop = std::max(0.0f, pop.rowhomeDensity);
        config.luxuryDensityPop = std::max(0.0f, pop.luxuryDensity);
        config.commercialWorkers = std::max(0.0f, pop.commercialWorkers);
        config.industrialWorkers = std::max(0.0f, pop.industrialWorkers);
        config.civicWorkers = std::max(0.0f, pop.civicWorkers);

        if (budget.maxBuildingsPerDistrict > 0) {
            const uint32_t district_count = std::max<uint32_t>(1, static_cast<uint32_t>(spec.districts.size()));
            const uint32_t city_cap = district_count * budget.maxBuildingsPerDistrict;
            config.maxBuildings = std::min(config.maxBuildings, std::max<uint32_t>(district_count, city_cap));
        }

        return config;
    }

    float GetLotDensity(const ZoningGenerator::Config& config, LotType lot_type) {
        switch (lot_type) {
            case LotType::Residential:
            case LotType::RowhomeCompact:
            case LotType::LuxuryScenic:
                return std::clamp(config.residentialDensity, 0.0f, 1.0f);
            case LotType::MixedUse:
            case LotType::RetailStrip:
                return std::clamp(config.commercialDensity, 0.0f, 1.0f);
            case LotType::LogisticsIndustrial:
                return std::clamp(config.industrialDensity, 0.0f, 1.0f);
            case LotType::CivicCultural:
                return std::clamp(config.civicDensity, 0.0f, 1.0f);
            case LotType::BufferStrip:
                return std::clamp(config.civicDensity * 0.5f, 0.0f, 1.0f);
            case LotType::None:
            default:
                return 0.0f;
        }
    }

    float StableUnitSample(uint32_t seed, uint32_t lot_id, uint32_t site_index) {
        uint32_t value = seed ^ 0x9e3779b9u;
        value ^= lot_id + 0x85ebca6bu + (value << 6) + (value >> 2);
        value ^= (site_index + 1u) * 0xc2b2ae35u;
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return static_cast<float>(value & 0x00ffffffu) / 16777215.0f;
    }

    [[nodiscard]] bool IsFinite(const Vec2& point) {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    [[nodiscard]] Core::Geometry::Polygon ToGeoPolygon(const std::vector<Vec2>& ring) {
        Core::Geometry::Polygon poly{};
        poly.vertices = ring;
        return poly;
    }

    [[nodiscard]] Vec2 ClosestPointOnSegment(const Vec2& point, const Vec2& a, const Vec2& b) {
        const Vec2 ab = b - a;
        const double ab_len_sq = ab.lengthSquared();
        if (ab_len_sq <= 1e-12) {
            return a;
        }
        const double t = std::clamp((point - a).dot(ab) / ab_len_sq, 0.0, 1.0);
        return a + (ab * t);
    }

    [[nodiscard]] std::vector<Core::Geometry::Polygon> BuildSetbackRegion(
        const LotToken& lot,
        double inset_min_x,
        double inset_max_x,
        double inset_min_y,
        double inset_max_y,
        const ZoningGenerator::Config& config) {
        constexpr double kEpsilon = 1e-6;
        using GeoOps = Core::Geometry::PolygonOps;

        Core::Geometry::Polygon lot_poly = GeoOps::SimplifyPolygon(ToGeoPolygon(lot.boundary), kEpsilon);
        if (!GeoOps::IsValidPolygon(lot_poly, kEpsilon)) {
            return {};
        }

        std::vector<Core::Geometry::Polygon> regions;
        if ((inset_max_x - inset_min_x) > kEpsilon && (inset_max_y - inset_min_y) > kEpsilon) {
            Core::Geometry::Polygon inset_rect{};
            inset_rect.vertices = {
                { inset_min_x, inset_min_y },
                { inset_max_x, inset_min_y },
                { inset_max_x, inset_max_y },
                { inset_min_x, inset_max_y },
            };
            regions = GeoOps::ClipPolygons(lot_poly, inset_rect);
        }

        if (regions.empty()) {
            const double uniform_inset = std::max(
                0.0,
                std::min({
                    static_cast<double>(std::max(0.0f, config.sideSetback)),
                    static_cast<double>(std::max(0.0f, config.frontSetback)),
                    static_cast<double>(std::max(0.0f, config.rearSetback)),
                }));
            if (uniform_inset > kEpsilon) {
                regions = GeoOps::InsetPolygon(lot_poly, uniform_inset);
            }
        }

        if (regions.empty()) {
            regions.push_back(std::move(lot_poly));
        }

        std::vector<Core::Geometry::Polygon> cleaned;
        cleaned.reserve(regions.size());
        for (auto& region : regions) {
            region = GeoOps::SimplifyPolygon(region, kEpsilon);
            if (GeoOps::IsValidPolygon(region, kEpsilon)) {
                cleaned.push_back(std::move(region));
            }
        }
        return cleaned;
    }

    void ClampToSetbackRegion(Vec2& position, const std::vector<Core::Geometry::Polygon>& regions) {
        if (regions.empty()) {
            return;
        }

        for (const auto& region : regions) {
            if (Urban::PolygonUtil::insidePolygon(position, region.vertices)) {
                return;
            }
        }

        double best_dist_sq = std::numeric_limits<double>::infinity();
        Vec2 best_point = position;
        for (const auto& region : regions) {
            if (region.vertices.size() < 2) {
                continue;
            }
            for (size_t i = 0; i < region.vertices.size(); ++i) {
                const Vec2& a = region.vertices[i];
                const Vec2& b = region.vertices[(i + 1u) % region.vertices.size()];
                const Vec2 candidate = ClosestPointOnSegment(position, a, b);
                const double dist_sq = (candidate - position).lengthSquared();
                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_point = candidate;
                }
            }
        }

        if (std::isfinite(best_dist_sq)) {
            position = best_point;
            return;
        }

        position = Urban::PolygonUtil::centroid(regions.front().vertices);
    }

    void ApplySetbacksToPosition(
        Vec2& position,
        const LotToken& lot,
        const ZoningGenerator::Config& config) {
        if (lot.boundary.empty()) {
            return;
        }

        double min_x = lot.boundary.front().x;
        double max_x = lot.boundary.front().x;
        double min_y = lot.boundary.front().y;
        double max_y = lot.boundary.front().y;
        for (const auto& point : lot.boundary) {
            min_x = std::min(min_x, point.x);
            max_x = std::max(max_x, point.x);
            min_y = std::min(min_y, point.y);
            max_y = std::max(max_y, point.y);
        }

        const double side = static_cast<double>(std::max(0.0f, config.sideSetback));
        const double front = static_cast<double>(std::max(0.0f, config.frontSetback));
        const double rear = static_cast<double>(std::max(0.0f, config.rearSetback));

        double inset_min_x = min_x + side;
        double inset_max_x = max_x - side;
        if (inset_min_x > inset_max_x) {
            const double center_x = (min_x + max_x) * 0.5;
            inset_min_x = center_x;
            inset_max_x = center_x;
        }

        double inset_min_y = min_y + front;
        double inset_max_y = max_y - rear;
        if (inset_min_y > inset_max_y) {
            const double center_y = (min_y + max_y) * 0.5;
            inset_min_y = center_y;
            inset_max_y = center_y;
        }

        position.x = std::clamp(position.x, inset_min_x, inset_max_x);
        position.y = std::clamp(position.y, inset_min_y, inset_max_y);

        const auto setback_regions = BuildSetbackRegion(
            lot,
            inset_min_x,
            inset_max_x,
            inset_min_y,
            inset_max_y,
            config);
        if (!setback_regions.empty()) {
            ClampToSetbackRegion(position, setback_regions);
        }

        if (!IsFinite(position)) {
            position = lot.centroid;
        }
    }

    } // namespace

    // End-to-end zoning/materialization pass:
    // 1) resolve config overrides
    // 2) lot subdivision + classification
    // 3) budget allocation + building placement
    // 4) population rollup and performance metrics
    ZoningGenerator::ZoningOutput ZoningGenerator::generate(
        const ZoningInput& input,
        const Config& config
    ) {
        config_ = ApplyCitySpecOverrides(config, input.citySpec);
        RNG rng(config_.seed);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        ZoningOutput output{};
        
        // Decide if we should use parallelization based on workload
        // (currently advisory metadata; actual heavy work paths may still be sequential).
        const uint32_t axiomCount = input.citySpec.has_value() 
            ? static_cast<uint32_t>(input.citySpec->districts.size()) 
            : 0;
        const uint32_t districtCount = static_cast<uint32_t>(input.districts.size());
        const uint32_t lotDensity = std::max<uint32_t>(1, static_cast<uint32_t>(input.blocks.size()));
        output.usedParallelization = shouldUseParallelization(axiomCount, districtCount, lotDensity);
        
        // Stage 1: Subdivide districts into lots (AESP-aware)
        auto lots = subdivideDistricts(input.districts, input.blocks, input.roads, rng);
        
        // Stage 2: Classify lots using AESP (determine lot types)
        classifyLots(lots);
        
        // Stage 3: Allocate building budget across lots
        output.totalBudgetUsed = allocateBudget(lots, config_.totalBuildingBudget);
        
        // Stage 4: Place buildings on lots
        output.buildings = placeBuildings(lots, rng);
        
        // Stage 5: Calculate population from buildings
        calculatePopulation(output.buildings, output.totalResidents, output.totalWorkers);
        output.totalPopulation = output.totalResidents + output.totalWorkers;
        
        // Convert std::vector<LotToken> to output (UI expects this format)
        output.lots = std::move(lots);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        output.generationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        return output;
    }

    // Heuristic workload estimate used for deciding whether parallel execution is worthwhile.
    bool ZoningGenerator::shouldUseParallelization(
        uint32_t axiomCount,
        uint32_t districtCount,
        uint32_t lotDensity
    ) const {
        const uint64_t workload = static_cast<uint64_t>(std::max<uint32_t>(1, axiomCount)) *
            static_cast<uint64_t>(std::max<uint32_t>(1, districtCount)) *
            static_cast<uint64_t>(std::max<uint32_t>(1, lotDensity));
        return workload > static_cast<uint64_t>(config_.parallelizationThreshold);
    }

    // Subdivides districts into lots using existing blocks when available.
    // If block input is absent, blocks are generated on-the-fly from district geometry.
    std::vector<LotToken> ZoningGenerator::subdivideDistricts(
        const std::vector<District>& districts,
        const std::vector<BlockPolygon>& blocks,
        const fva::Container<Road>& roads,
        RNG& rng
    ) {
        (void)rng;
        std::vector<BlockPolygon> effective_blocks = blocks;
        if (effective_blocks.empty()) {
            effective_blocks = Urban::BlockGenerator::generate(districts);
        }

        Urban::LotGenerator::Config lot_cfg;
        lot_cfg.min_lot_width = config_.minLotWidth;
        lot_cfg.max_lot_width = config_.maxLotWidth;
        lot_cfg.min_lot_depth = config_.minLotDepth;
        lot_cfg.max_lot_depth = config_.maxLotDepth;
        lot_cfg.min_lot_area = config_.minLotArea;
        lot_cfg.max_lot_area = config_.maxLotArea;
        lot_cfg.max_lots = config_.maxLots;

        return Urban::LotGenerator::generate(roads, districts, effective_blocks, lot_cfg, config_.seed);
    }

    // Assigns lot types via AESP-style district classification unless the lot is user-locked.
    void ZoningGenerator::classifyLots(std::vector<LotToken>& lots) {
        for (auto& lot : lots) {
            if (lot.locked_type) {
                continue; // User-locked lot type
            }
            
            // Classify using AESP scores
            RogueProfiler::Scores scores{ lot.access, lot.exposure, lot.serviceability, lot.privacy };
            DistrictType districtType = RogueProfiler::classifyDistrict(scores);
            
            // Map DistrictType to LotType
            switch (districtType) {
                case DistrictType::Residential:
                    lot.lot_type = LotType::Residential;
                    break;
                case DistrictType::Commercial:
                    lot.lot_type = LotType::RetailStrip;
                    break;
                case DistrictType::Industrial:
                    lot.lot_type = LotType::LogisticsIndustrial;
                    break;
                case DistrictType::Civic:
                    lot.lot_type = LotType::CivicCultural;
                    break;
                case DistrictType::Mixed:
                default:
                    lot.lot_type = LotType::MixedUse;
                    break;
            }
        }
    }

    // Allocates available city budget across generated lots with type-dependent multipliers.
    // Returns capped total spend (never above requested total budget).
    float ZoningGenerator::allocateBudget(
        std::vector<LotToken>& lots,
        float totalBudget
    ) {
        if (lots.empty()) {
            return 0.0f;
        }

        totalBudget = std::max(0.0f, totalBudget);
        const float budgetPerLot = totalBudget / static_cast<float>(lots.size());
        float rawBudgetUsed = 0.0f;

        for (auto& lot : lots) {
            // Assign budget based on lot type
            float lotBudget = budgetPerLot;
            
            // Apply cost multipliers based on lot type
            switch (lot.lot_type) {
                case LotType::LuxuryScenic:
                    lotBudget *= config_.luxuryCost;
                    break;
                case LotType::MixedUse:
                case LotType::RetailStrip:
                    lotBudget *= config_.commercialCost;
                    break;
                case LotType::LogisticsIndustrial:
                    lotBudget *= config_.industrialCost;
                    break;
                case LotType::CivicCultural:
                    lotBudget *= config_.civicCost;
                    break;
                case LotType::Residential:
                case LotType::RowhomeCompact:
                default:
                    lotBudget *= config_.residentialCost;
                    break;
            }
            lot.budget_allocation = lotBudget;
            rawBudgetUsed += lotBudget;
        }

        // Keep per-lot allocations and returned aggregate strictly consistent.
        if (rawBudgetUsed > totalBudget && rawBudgetUsed > 0.0f) {
            const float scale = totalBudget / rawBudgetUsed;
            for (auto& lot : lots) {
                lot.budget_allocation *= scale;
            }
            return totalBudget;
        }

        return rawBudgetUsed;
    }

    // Generates building sites from lots with deterministic seed and configured caps.
    siv::Vector<BuildingSite> ZoningGenerator::placeBuildings(
        const std::vector<LotToken>& lots,
        RNG& rng
    ) {
        (void)rng;
        Urban::SiteGenerator::Config site_cfg;
        site_cfg.max_buildings = config_.maxBuildings;
        site_cfg.randomize_sites = false;
        auto generated_sites = Urban::SiteGenerator::generate(lots, site_cfg, config_.seed);
        if (generated_sites.empty()) {
            return generated_sites;
        }

        std::unordered_map<uint32_t, const LotToken*> lots_by_id;
        lots_by_id.reserve(lots.size());
        for (const auto& lot : lots) {
            lots_by_id[lot.id] = &lot;
        }

        std::unordered_map<uint32_t, uint32_t> emitted_per_lot;
        emitted_per_lot.reserve(lots.size());

        siv::Vector<BuildingSite> filtered_sites;
        filtered_sites.reserve(generated_sites.size());

        for (auto site : generated_sites) {
            const auto lot_it = lots_by_id.find(site.lot_id);
            if (lot_it == lots_by_id.end()) {
                continue;
            }

            const LotToken& lot = *lot_it->second;
            uint32_t& local_index = emitted_per_lot[lot.id];
            const float density = GetLotDensity(config_, lot.lot_type);
            const float sample = StableUnitSample(config_.seed, lot.id, local_index);
            local_index += 1u;
            if (sample > density) {
                continue;
            }

            ApplySetbacksToPosition(site.position, lot, config_);
            site.estimated_cost = getBuildingCost(site.type);
            filtered_sites.push_back(site);
        }

        for (size_t i = 0; i < filtered_sites.size(); ++i) {
            filtered_sites[i].id = static_cast<uint32_t>(i + 1u);
        }
        return filtered_sites;
    }

    // Converts building inventory into aggregate residents/workers counts.
    // Population factors come directly from zoning config density knobs.
    void ZoningGenerator::calculatePopulation(
        const siv::Vector<BuildingSite>& buildings,
        uint32_t& outResidents,
        uint32_t& outWorkers
    ) {
        float totalResidents = 0.0f;
        float totalWorkers = 0.0f;
        
        for (size_t i = 0; i < buildings.size(); ++i) {
            const auto& building = buildings[i];

            totalResidents += getPopulationDensity(building.type, true);
            float workers = getPopulationDensity(building.type, false);
            if (building.type == BuildingType::MixedUse) {
                workers *= 0.5f; // Keep mixed-use half-commercial worker contribution.
            }
            totalWorkers += workers;
        }
        
        outResidents = static_cast<uint32_t>(totalResidents);
        outWorkers = static_cast<uint32_t>(totalWorkers);
    }

    // Maps building type to normalized budget multiplier.
    float ZoningGenerator::getBuildingCost(BuildingType type) const {
        switch (type) {
            case BuildingType::Luxury: return config_.luxuryCost;
            case BuildingType::Retail:
            case BuildingType::MixedUse: return config_.commercialCost;
            case BuildingType::Industrial: return config_.industrialCost;
            case BuildingType::Civic: return config_.civicCost;
            case BuildingType::Residential:
            case BuildingType::Rowhome:
            default: return config_.residentialCost;
        }
    }

    // Returns per-building population/workforce contribution based on type and mode.
    // isResidential toggles resident densities vs worker densities.
    float ZoningGenerator::getPopulationDensity(BuildingType type, bool isResidential) const {
        if (isResidential) {
            switch (type) {
                case BuildingType::Residential: return config_.residentialDensityPop;
                case BuildingType::Rowhome: return config_.rowhomeDensityPop;
                case BuildingType::MixedUse: return config_.mixedUseDensityPop;
                case BuildingType::Luxury: return config_.luxuryDensityPop;
                default: return 0.0f;
            }
        } else {
            switch (type) {
                case BuildingType::Retail:
                case BuildingType::MixedUse: return config_.commercialWorkers;
                case BuildingType::Industrial: return config_.industrialWorkers;
                case BuildingType::Civic: return config_.civicWorkers;
                default: return 0.0f;
            }
        }
    }

} // namespace RogueCity::Generators
