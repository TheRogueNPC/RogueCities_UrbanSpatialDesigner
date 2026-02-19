#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/LotGenerator.hpp"
#include "RogueCity/Generators/Urban/SiteGenerator.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"
#include <vector>
#include <cstdint>
#include <optional>

namespace RogueCity::Generators {

    using namespace Core;

    /// Orchestrates zone/lot/building placement pipeline with AESP integration
    class ZoningGenerator {
    public:
        /// Zoning generation configuration
        struct Config {
            // Threading thresholds (adaptive context-aware toggle)
            uint32_t parallelizationThreshold{ 8 };  // Use RogueWorker if axiom/district count >= this
            
            // Lot subdivision parameters
            float minLotWidth{ 10.0f };
            float maxLotWidth{ 50.0f };
            float minLotDepth{ 15.0f };
            float maxLotDepth{ 60.0f };
            float minLotArea{ 150.0f };
            float maxLotArea{ 3000.0f };
            
            // Building placement parameters
            float frontSetback{ 3.0f };
            float sideSetback{ 2.0f };
            float rearSetback{ 5.0f };
            
            // Density multipliers per district type
            float residentialDensity{ 0.6f };
            float commercialDensity{ 0.8f };
            float industrialDensity{ 0.7f };
            float civicDensity{ 0.5f };
            
            // Budget constraints
            float totalBuildingBudget{ 1000.0f };
            float residentialCost{ 1.0f };
            float commercialCost{ 2.0f };
            float industrialCost{ 3.0f };
            float civicCost{ 4.0f };
            float luxuryCost{ 8.0f };
            
            // Population parameters
            float residentialDensityPop{ 50.0f };
            float mixedUseDensityPop{ 30.0f };
            float rowhomeDensityPop{ 20.0f };
            float luxuryDensityPop{ 10.0f };
            float commercialWorkers{ 25.0f };
            float industrialWorkers{ 100.0f };
            float civicWorkers{ 40.0f };
            
            // Subdivision settings
            bool allowRecursiveSubdivision{ true };
            uint32_t maxSubdivisionDepth{ 3 };
            uint32_t maxLots{ 50000 };
            uint32_t maxBuildings{ 100000 };
            
            uint32_t seed{ 12345 };
        };

        /// Input data for zoning generation (from existing city)
        struct ZoningInput {
            fva::Container<Road> roads;              // Existing road network
            std::vector<District> districts;          // District boundaries with AESP metadata
            std::vector<BlockPolygon> blocks;         // City blocks (for lot subdivision)
            std::optional<CitySpec> citySpec;         // Optional CitySpec for budget/population overrides
        };

        /// Generated zoning output
        struct ZoningOutput {
            std::vector<LotToken> lots;               // FVA-compatible lots (stable handles)
            siv::Vector<BuildingSite> buildings;      // SIV for high-churn entities
            
            // Metadata for UI display
            float totalBudgetUsed{ 0.0f };
            uint32_t totalPopulation{ 0 };
            uint32_t totalResidents{ 0 };
            uint32_t totalWorkers{ 0 };
            
            // Performance metrics
            double generationTimeMs{ 0.0 };
            bool usedParallelization{ false };
        };

        ZoningGenerator() = default;

        /// Generate lots and buildings from districts/roads
        [[nodiscard]] ZoningOutput generate(
            const ZoningInput& input,
            const Config& config
        );

        /// Generate with default config
        [[nodiscard]] ZoningOutput generate(const ZoningInput& input) {
            return generate(input, Config{});
        }

    private:
        Config config_;
        // ===== PIPELINE STAGES =====

        /// Stage 1: Subdivide districts into lots (AESP-aware)
        [[nodiscard]] std::vector<LotToken> subdivideDistricts(
            const std::vector<District>& districts,
            const std::vector<BlockPolygon>& blocks,
            const fva::Container<Road>& roads,
            RNG& rng);

        /// Stage 2: Classify lots using AESP (determine lot types)
        void classifyLots(std::vector<LotToken>& lots);

        /// Stage 3: Allocate building budget across lots
        [[nodiscard]] float allocateBudget(
            std::vector<LotToken>& lots,
            float totalBudget);

        /// Stage 4: Place buildings on lots
        [[nodiscard]] siv::Vector<BuildingSite> placeBuildings(
            const std::vector<LotToken>& lots,
            RNG& rng);

        /// Stage 5: Calculate population from buildings
        void calculatePopulation(
            const siv::Vector<BuildingSite>& buildings,
            uint32_t& outResidents,
            uint32_t& outWorkers);

        // ===== HELPER METHODS =====

        /// Determine if parallelization should be used (context-aware toggle)
        [[nodiscard]] bool shouldUseParallelization(
            uint32_t axiomCount,
            uint32_t districtCount,
            uint32_t lotDensity
        ) const;

        /// Get building cost from type
        [[nodiscard]] float getBuildingCost(BuildingType type) const;

        /// Get population density for building type
        [[nodiscard]] float getPopulationDensity(BuildingType type, bool isResidential) const;
    };

} // namespace RogueCity::Generators
