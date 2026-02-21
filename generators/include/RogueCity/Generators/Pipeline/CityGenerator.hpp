#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Generators/Pipeline/GenerationContext.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"
#include "RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp"
#include "RogueCity/Generators/Pipeline/TerrainConstraintGenerator.hpp"
#include "RogueCity/Generators/Tensors/BasisFields.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/DistrictGenerator.hpp"
#include "RogueCity/Generators/Urban/LotGenerator.hpp"
#include "RogueCity/Generators/Urban/RoadGenerator.hpp"
#include "RogueCity/Generators/Urban/SiteGenerator.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace RogueCity::Core::Editor {
    struct GlobalState;
}
namespace RogueCity::Core::Data {
    class TextureSpace;
}

namespace RogueCity::Generators {

using namespace Core;

class CityGenerator {
public:
    struct Config {
        int width{ 2000 };
        int height{ 2000 };
        double cell_size{ 10.0 };
        uint32_t seed{ 12345 };
        int num_seeds{ 20 };
        uint32_t max_districts{ 256 };
        uint32_t max_lots{ 50000 };
        uint32_t max_buildings{ 100000 };
        bool enable_world_constraints{ true };
        int max_texture_resolution{ 2048 };
        TerrainConstraintGenerator::Config terrain{};
        PlanValidatorGenerator::Config validator{};

        bool incremental_mode{ false };
        int max_iterations_per_axiom{ 5 };

        // Quality controls for streamline tracing.
        bool adaptive_tracing{ false };
        bool enforce_road_separation{ false };
        double min_trace_step_size{ 2.0 };
        double max_trace_step_size{ 12.0 };
        double trace_curvature_gain{ 1.5 };
    };

    struct ValidationResult {
        bool valid{ true };
        Config clamped_config{};
        std::vector<std::string> errors{};
        std::vector<std::string> warnings{};

        [[nodiscard]] bool ok() const noexcept {
            return valid && errors.empty();
        }
    };

    struct AxiomInput {
        struct RingSchema {
            double core_ratio{ 0.33 };
            double falloff_ratio{ 0.67 };
            double outskirts_ratio{ 1.0 };
            // Width near outskirts where merges are preferred.
            double merge_band_ratio{ 0.12 };
            bool strict_core{ true };
        };

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

        int id{ 0 };
        Type type{ Type::Grid };
        Vec2 position{};
        double radius{ 250.0 };
        double theta{ 0.0 };
        double decay{ 2.0 };
        RingSchema ring_schema{};
        bool lock_generated_roads{ false };

        float organic_curviness{ 0.5f };
        int radial_spokes{ 8 };
        float loose_grid_jitter{ 0.15f };
        float suburban_loop_strength{ 0.7f };
        float stem_branch_angle{ 0.7f };
        float superblock_block_size{ 250.0f };
    };

    struct CityOutput {
        fva::Container<Road> roads;
        std::vector<District> districts;
        std::vector<BlockPolygon> blocks;
        std::vector<LotToken> lots;
        siv::Vector<BuildingSite> buildings;
        std::vector<Vec2> city_boundary;
        std::vector<Polyline> connector_debug_edges;
        TensorFieldGenerator tensor_field;
        WorldConstraintField world_constraints;
        SiteProfile site_profile;
        std::vector<PlanViolation> plan_violations;
        bool plan_approved{ true };
    };

    struct StageOptions {
        StageMask stages_to_run{ FullStageMask() };
        bool use_cache{ true };
        // When false, only explicitly requested stages run (no downstream cascade).
        bool cascade_downstream{ true };
        // When true, seed sampling is constrained to axiom influence volumes.
        bool constrain_seeds_to_axiom_bounds{ true };
        // When true, generated geometry is clipped/filtered to axiom influence volumes.
        bool constrain_roads_to_axiom_bounds{ true };
    };

    CityGenerator() = default;

    [[nodiscard]] CityOutput generate(
        const std::vector<AxiomInput>& axioms);

    [[nodiscard]] CityOutput generate(
        const std::vector<AxiomInput>& axioms,
        const Config& config);

    [[nodiscard]] CityOutput generate(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        Core::Editor::GlobalState* global_state);

    [[nodiscard]] CityOutput GenerateWithContext(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        GenerationContext* context,
        Core::Editor::GlobalState* global_state = nullptr);

    [[nodiscard]] CityOutput GenerateStages(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        const StageOptions& options,
        Core::Editor::GlobalState* global_state = nullptr,
        GenerationContext* context = nullptr);

    [[nodiscard]] CityOutput RegenerateIncremental(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        StageMask dirty_stages,
        Core::Editor::GlobalState* global_state = nullptr,
        GenerationContext* context = nullptr);

    [[nodiscard]] static ValidationResult ValidateAndClampConfig(const Config& config);
    [[nodiscard]] static bool ValidateAxioms(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        std::vector<std::string>* errors = nullptr);

private:
    struct StageCache {
        bool has_signature{ false };
        Config config{};
        uint64_t axioms_hash{ 0 };

        WorldConstraintField world_constraints{};
        SiteProfile site_profile{};
        TensorFieldGenerator tensor_field{};
        fva::Container<Road> roads{};
        std::vector<District> districts{};
        std::vector<BlockPolygon> blocks{};
        std::vector<LotToken> lots{};
        siv::Vector<BuildingSite> buildings{};
        std::vector<Vec2> city_boundary{};
        std::vector<Polyline> connector_debug_edges{};
        std::vector<PlanViolation> plan_violations{};
        bool plan_approved{ true };

        StageMask valid_stages{};
    };

    [[nodiscard]] static uint64_t HashAxioms(const std::vector<AxiomInput>& axioms);
    static void DeriveConstraintsFromWorldSize(Config& config, ValidationResult& result);

    void InvalidateCacheIfInputChanged(const Config& config, uint64_t axioms_hash);

    [[nodiscard]] bool ShouldAbort(GenerationContext* context) const;

    TensorFieldGenerator generateTensorField(
        const std::vector<AxiomInput>& axioms,
        GenerationContext* context);

    std::vector<Vec2> generateSeeds(
        const WorldConstraintField* constraints,
        const Core::Data::TextureSpace* texture_space,
        GenerationContext* context,
        const std::vector<AxiomInput>* seed_axiom_bounds);

    fva::Container<Road> traceRoads(
        const TensorFieldGenerator& field,
        const std::vector<Vec2>& seeds,
        const WorldConstraintField* constraints,
        const SiteProfile* profile,
        const Core::Data::TextureSpace* texture_space,
        GenerationContext* context);

    std::vector<District> classifyDistricts(
        const fva::Container<Road>& roads,
        const WorldConstraintField* constraints,
        GenerationContext* context);

    std::vector<BlockPolygon> generateBlocks(
        const std::vector<District>& districts,
        GenerationContext* context);

    std::vector<LotToken> generateLots(
        const fva::Container<Road>& roads,
        const std::vector<District>& districts,
        const std::vector<BlockPolygon>& blocks,
        const SiteProfile* profile,
        GenerationContext* context);

    siv::Vector<BuildingSite> generateBuildings(
        const std::vector<LotToken>& lots,
        const SiteProfile* profile,
        GenerationContext* context);

    void rasterizeDistrictZonesToTexture(
        const std::vector<District>& districts,
        Core::Data::TextureSpace& texture_space) const;

    [[nodiscard]] siv::Vector<BuildingSite> filterBuildingsByTexture(
        const siv::Vector<BuildingSite>& buildings,
        const std::vector<LotToken>& lots,
        const std::vector<District>& districts,
        const Core::Data::TextureSpace& texture_space) const;

    void initializeTextureSpaceIfNeeded(
        Core::Editor::GlobalState* global_state,
        const WorldConstraintField* constraints) const;

    Config config_{};
    RNG rng_{ 12345 };
    StageCache cache_{};
};

} // namespace RogueCity::Generators
