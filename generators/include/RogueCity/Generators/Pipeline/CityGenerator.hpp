#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Generators/Tensors/BasisFields.hpp"
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Generators/Urban/RoadGenerator.hpp"
#include "RogueCity/Generators/Urban/DistrictGenerator.hpp"
#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/LotGenerator.hpp"
#include "RogueCity/Generators/Urban/SiteGenerator.hpp"
#include "RogueCity/Generators/Pipeline/TerrainConstraintGenerator.hpp"
#include "RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include <vector>
#include <cstdint>

namespace RogueCity::Core::Editor {
    struct GlobalState;
}
namespace RogueCity::Core::Data {
    class TextureSpace;
}

namespace RogueCity::Generators {

    using namespace Core;

    /// Orchestrates full city generation pipeline
    class CityGenerator {
    public:
        /// Generation configuration
        struct Config {
            int width{ 2000 };           // City width (meters)
            int height{ 2000 };          // City height (meters)
            double cell_size{ 10.0 };    // Tensor field resolution (meters)
            uint32_t seed{ 12345 };      // RNG seed
            int num_seeds{ 20 };         // Number of streamline seeds
            uint32_t max_districts{ 256 };
            uint32_t max_lots{ 50000 };
            uint32_t max_buildings{ 100000 };
            bool enable_world_constraints{ true };
            TerrainConstraintGenerator::Config terrain{};
            PlanValidatorGenerator::Config validator{};
            
            // Incremental generation (RC-0.09-Test P0 fix)
            bool incremental_mode{ false };         // Enable stepping mode
            int max_iterations_per_axiom{ 5 };      // Limit iterations per placement
            // TODO: RC-0.10 - Implement async task queue with cancellation tokens
        };

        /// Axiom input (user-placed planning intent)
        struct AxiomInput {
            enum class Type : uint8_t {
                Organic = 0,
                Grid = 1,
                Radial = 2,
                Hexagonal = 3,
                Stem = 4,
                LooseGrid = 5,
                Suburban = 6,
                Superblock = 7,
                Linear = 8,
                GridCorrective = 9,
                COUNT = 10
            };
            
            Type type{ Type::Grid };
            Vec2 position{};
            double radius{ 250.0 };
            double theta{ 0.0 };  // Grid/Linear/Stem orientation (radians)
            double decay{ 2.0 };

            // Type-specific parameters (interpreted by `type`)
            float organic_curviness{ 0.5f };          // Organic [0..1]
            int radial_spokes{ 8 };                   // Radial [3..24]
            float loose_grid_jitter{ 0.15f };         // LooseGrid [0..1]
            float suburban_loop_strength{ 0.7f };     // Suburban [0..1]
            float stem_branch_angle{ 0.7f };          // Stem (radians)
            float superblock_block_size{ 250.0f };    // Superblock (meters)
        };

        /// Generated city output
        struct CityOutput {
            fva::Container<Road> roads;
            std::vector<District> districts;
            std::vector<BlockPolygon> blocks;
            std::vector<LotToken> lots;
            siv::Vector<BuildingSite> buildings;
            TensorFieldGenerator tensor_field;
            WorldConstraintField world_constraints;
            SiteProfile site_profile;
            std::vector<PlanViolation> plan_violations;
            bool plan_approved{ true };
        };

        CityGenerator() = default;

        /// Generate city from axioms
        [[nodiscard]] CityOutput generate(
            const std::vector<AxiomInput>& axioms,
            const Config& config,
            Core::Editor::GlobalState* global_state = nullptr
        );

        /// Generate with default config
        [[nodiscard]] CityOutput generate(const std::vector<AxiomInput>& axioms) {
            return generate(axioms, Config{});
        }

    private:
        Config config_;
        RNG rng_{ 12345 };

        /// Pipeline stages
        TensorFieldGenerator generateTensorField(const std::vector<AxiomInput>& axioms);
        std::vector<Vec2> generateSeeds(
            const WorldConstraintField* constraints,
            const Core::Data::TextureSpace* texture_space);
        fva::Container<Road> traceRoads(
            const TensorFieldGenerator& field,
            const std::vector<Vec2>& seeds,
            const WorldConstraintField* constraints,
            const SiteProfile* profile,
            const Core::Data::TextureSpace* texture_space);
        std::vector<District> classifyDistricts(
            const fva::Container<Road>& roads,
            const WorldConstraintField* constraints);
        std::vector<BlockPolygon> generateBlocks(const std::vector<District>& districts);
        std::vector<LotToken> generateLots(
            const fva::Container<Road>& roads,
            const std::vector<District>& districts,
            const std::vector<BlockPolygon>& blocks,
            const SiteProfile* profile);
        siv::Vector<BuildingSite> generateBuildings(
            const std::vector<LotToken>& lots,
            const SiteProfile* profile);
        void rasterizeDistrictZonesToTexture(
            const std::vector<District>& districts,
            Core::Data::TextureSpace& texture_space) const;
        siv::Vector<BuildingSite> filterBuildingsByTexture(
            const siv::Vector<BuildingSite>& buildings,
            const std::vector<LotToken>& lots,
            const std::vector<District>& districts,
            const Core::Data::TextureSpace& texture_space) const;
        void initializeTextureSpaceIfNeeded(
            Core::Editor::GlobalState* global_state,
            const WorldConstraintField* constraints) const;
    };

} // namespace RogueCity::Generators
