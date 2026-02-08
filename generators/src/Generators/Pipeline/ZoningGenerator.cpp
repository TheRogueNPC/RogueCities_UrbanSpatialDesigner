#include "RogueCity/Generators/Pipeline/ZoningGenerator.hpp"
#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>

namespace RogueCity::Generators {

    using namespace Core;

    ZoningGenerator::ZoningOutput ZoningGenerator::generate(
        const ZoningInput& input,
        const Config& config
    ) {
        config_ = config;
        RNG rng(config.seed);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        ZoningOutput output{};
        
        // Decide if we should use parallelization based on workload
        const uint32_t axiomCount = input.citySpec.has_value() 
            ? static_cast<uint32_t>(input.citySpec->districts.size()) 
            : 0;
        const uint32_t districtCount = static_cast<uint32_t>(input.districts.size());
        output.usedParallelization = shouldUseParallelization(axiomCount, districtCount);
        
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
        uint32_t districtCount
    ) const {
        const uint32_t totalWorkload = axiomCount + districtCount;
        return totalWorkload >= config_.parallelizationThreshold;
    }

    std::vector<LotToken> ZoningGenerator::subdivideDistricts(
        const std::vector<District>& districts,
        const std::vector<BlockPolygon>& blocks,
        const fva::Container<Road>& roads,
        RNG& rng
    ) {
        std::vector<LotToken> lots;
        lots.reserve(blocks.size() * 4); // Heuristic: avg 4 lots per block
        
        // Subdivide each block into lots
        for (const auto& block : blocks) {
            subdivideBlock(block, lots, rng, 0);
        }
        
        // Assign lot IDs and compute AESP scores from adjacent roads
        for (size_t i = 0; i < lots.size(); ++i) {
            lots[i].id = static_cast<uint32_t>(i);
            
            // Compute AESP scores using AESPClassifier
            AESPClassifier::AESPScores scores = AESPClassifier::computeScores(
                lots[i].primary_road,
                lots[i].secondary_road
            );
            
            lots[i].access = scores.A;
            lots[i].exposure = scores.E;
            lots[i].serviceability = scores.S;
            lots[i].privacy = scores.P;
        }
        
        return lots;
    }

    void ZoningGenerator::subdivideBlock(
        const BlockPolygon& block,
        std::vector<LotToken>& outLots,
        RNG& rng,
        uint32_t depth
    ) {
        // Simple heuristic subdivision for MVP
        // TODO: Implement OBB-based recursive subdivision per design doc
        
        if (depth >= config_.maxSubdivisionDepth) {
            return;
        }
        
        // Create a single lot from the block (placeholder logic)
        LotToken lot{};
        lot.district_id = block.district_id;
        
        // Compute centroid
        Vec2 centroid{ 0.0, 0.0 };
        for (const auto& pt : block.outer) {
            centroid.x += pt.x;
            centroid.y += pt.y;
        }
        if (!block.outer.empty()) {
            centroid.x /= static_cast<double>(block.outer.size());
            centroid.y /= static_cast<double>(block.outer.size());
        }
        lot.centroid = centroid;
        
        // Assign default road types (will be refined in AESP stage)
        lot.primary_road = RoadType::Street;
        lot.secondary_road = RoadType::Lane;
        
        outLots.push_back(lot);
    }

    void ZoningGenerator::classifyLots(std::vector<LotToken>& lots) {
        for (auto& lot : lots) {
            if (lot.locked_type) {
                continue; // User-locked lot type
            }
            
            // Classify using AESP scores
            AESPClassifier::AESPScores scores{ lot.access, lot.exposure, lot.serviceability, lot.privacy };
            DistrictType districtType = AESPClassifier::classifyDistrict(scores);
            
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
        float budgetUsed = 0.0f;
        
        // Simple uniform allocation for MVP
        // TODO: Implement priority-based budget allocation (high AESP scores first)
        
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
            
            budgetUsed += lotBudget;
        }
        
        return std::min(budgetUsed, totalBudget);
    }

    siv::Vector<BuildingSite> ZoningGenerator::placeBuildings(
        const std::vector<LotToken>& lots,
        RNG& rng
    ) {
        siv::Vector<BuildingSite> buildings;
        buildings.reserve(lots.size()); // One building per lot for MVP
        
        for (const auto& lot : lots) {
            BuildingSite building{};
            building.id = static_cast<uint32_t>(buildings.size());
            building.lot_id = lot.id;
            building.district_id = lot.district_id;
            building.position = lot.centroid;
            
            // Map LotType to BuildingType
            switch (lot.lot_type) {
                case LotType::Residential:
                    building.type = BuildingType::Residential;
                    break;
                case LotType::RowhomeCompact:
                    building.type = BuildingType::Rowhome;
                    break;
                case LotType::RetailStrip:
                    building.type = BuildingType::Retail;
                    break;
                case LotType::MixedUse:
                    building.type = BuildingType::MixedUse;
                    break;
                case LotType::LogisticsIndustrial:
                    building.type = BuildingType::Industrial;
                    break;
                case LotType::CivicCultural:
                    building.type = BuildingType::Civic;
                    break;
                case LotType::LuxuryScenic:
                    building.type = BuildingType::Luxury;
                    break;
                default:
                    building.type = BuildingType::None;
                    break;
            }
            
            buildings.push_back(building);
        }
        
        return buildings;
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
