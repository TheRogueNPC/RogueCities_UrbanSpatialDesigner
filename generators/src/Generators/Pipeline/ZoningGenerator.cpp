#include "RogueCity/Generators/Pipeline/ZoningGenerator.hpp"
#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>

namespace RogueCity::Generators {

    using namespace Core;
    namespace {

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

    } // namespace

    ZoningGenerator::ZoningOutput ZoningGenerator::generate(
        const ZoningInput& input,
        const Config& config
    ) {
        config_ = ApplyCitySpecOverrides(config, input.citySpec);
        RNG rng(config_.seed);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        ZoningOutput output{};
        
        // Decide if we should use parallelization based on workload
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
        output.totalBudgetUsed = allocateBudget(lots, config.totalBuildingBudget);
        
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

    float ZoningGenerator::allocateBudget(
        std::vector<LotToken>& lots,
        float totalBudget
    ) {
        if (lots.empty()) {
            return 0.0f;
        }

        float budgetUsed = 0.0f;
        
        const float budgetPerLot = totalBudget / static_cast<float>(lots.size());
        
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
            budgetUsed += lotBudget;
        }
        
        return std::min(budgetUsed, totalBudget);
    }

    siv::Vector<BuildingSite> ZoningGenerator::placeBuildings(
        const std::vector<LotToken>& lots,
        RNG& rng
    ) {
        (void)rng;
        Urban::SiteGenerator::Config site_cfg;
        site_cfg.max_buildings = config_.maxBuildings;
        site_cfg.randomize_sites = false;
        return Urban::SiteGenerator::generate(lots, site_cfg, config_.seed);
    }

    void ZoningGenerator::calculatePopulation(
        const siv::Vector<BuildingSite>& buildings,
        uint32_t& outResidents,
        uint32_t& outWorkers
    ) {
        float totalResidents = 0.0f;
        float totalWorkers = 0.0f;
        
        for (size_t i = 0; i < buildings.size(); ++i) {
            const auto& building = buildings[i];
            
            switch (building.type) {
                case BuildingType::Residential:
                    totalResidents += config_.residentialDensityPop;
                    break;
                case BuildingType::Rowhome:
                    totalResidents += config_.rowhomeDensityPop;
                    break;
                case BuildingType::MixedUse:
                    totalResidents += config_.mixedUseDensityPop;
                    totalWorkers += config_.commercialWorkers * 0.5f; // Half commercial
                    break;
                case BuildingType::Retail:
                    totalWorkers += config_.commercialWorkers;
                    break;
                case BuildingType::Industrial:
                    totalWorkers += config_.industrialWorkers;
                    break;
                case BuildingType::Civic:
                    totalWorkers += config_.civicWorkers;
                    break;
                case BuildingType::Luxury:
                    totalResidents += config_.luxuryDensityPop;
                    break;
                default:
                    break;
            }
        }
        
        outResidents = static_cast<uint32_t>(totalResidents);
        outWorkers = static_cast<uint32_t>(totalWorkers);
    }

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
