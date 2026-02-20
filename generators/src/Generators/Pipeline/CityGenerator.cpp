#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include "RogueCity/Core/Data/MaterialEncoding.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <utility>

namespace RogueCity::Generators {

namespace {

constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

void HashBytes(const void* data, size_t size, uint64_t& hash) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(bytes[i]);
        hash *= kFnvPrime;
    }
}

template <typename T>
void HashScalar(const T& value, uint64_t& hash) {
    HashBytes(&value, sizeof(T), hash);
}

void HashDouble(double value, uint64_t& hash) {
    if (value == 0.0) {
        value = 0.0;
    }
    HashScalar(value, hash);
}

void HashFloat(float value, uint64_t& hash) {
    if (value == 0.0f) {
        value = 0.0f;
    }
    HashScalar(value, hash);
}

void HashVec2(const Core::Vec2& value, uint64_t& hash) {
    HashDouble(value.x, hash);
    HashDouble(value.y, hash);
}

[[nodiscard]] Core::Bounds ConstraintsBounds(const Core::WorldConstraintField& constraints) {
    Core::Bounds world_bounds{};
    world_bounds.min = Core::Vec2(0.0, 0.0);
    world_bounds.max = Core::Vec2(
        static_cast<double>(constraints.width) * constraints.cell_size,
        static_cast<double>(constraints.height) * constraints.cell_size);
    return world_bounds;
}

[[nodiscard]] bool BoundsMatch(const Core::Bounds& a, const Core::Bounds& b, double tolerance) {
    return a.min.equals(b.min, tolerance) && a.max.equals(b.max, tolerance);
}

void AssertTextureMatchesConstraints(
    const Core::Data::TextureSpace& texture_space,
    const Core::WorldConstraintField& constraints) {
#ifndef NDEBUG
    const Core::Bounds expected_bounds = ConstraintsBounds(constraints);
    const double tolerance = std::max(1e-6, constraints.cell_size * 0.25);
    assert(BoundsMatch(texture_space.bounds(), expected_bounds, tolerance));
    assert(texture_space.resolution() == std::max(constraints.width, constraints.height));
#else
    (void)texture_space;
    (void)constraints;
#endif
}

[[nodiscard]] bool PointInPolygon(const Core::Vec2& point, const std::vector<Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const Core::Vec2& a = polygon[i];
        const Core::Vec2& b = polygon[j];
        const bool intersects = ((a.y > point.y) != (b.y > point.y)) &&
            (point.x < (b.x - a.x) * (point.y - a.y) / ((b.y - a.y) + 1e-12) + a.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

[[nodiscard]] bool PointInsideAnyAxiom(
    const Core::Vec2& point,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    for (const auto& axiom : axioms) {
        const double radius = std::max(8.0, axiom.radius);
        if (point.distanceTo(axiom.position) <= radius) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] fva::Container<Core::Road> ClipRoadsToAxiomInfluence(
    const fva::Container<Core::Road>& roads,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (axioms.empty()) {
        return roads;
    }

    fva::Container<Core::Road> clipped{};
    uint32_t next_id = 1u;

    for (const auto& road : roads) {
        if (road.points.size() < 2) {
            continue;
        }

        Core::Road segment = road;
        segment.points.clear();

        auto flush_segment = [&]() {
            if (segment.points.size() < 2) {
                segment.points.clear();
                return;
            }
            segment.id = next_id++;
            clipped.add(segment);
            segment.points.clear();
        };

        for (const auto& p : road.points) {
            if (PointInsideAnyAxiom(p, axioms)) {
                segment.points.push_back(p);
            } else {
                flush_segment();
            }
        }
        flush_segment();
    }

    return clipped;
}

[[nodiscard]] bool TerrainConfigEqual(
    const TerrainConstraintGenerator::Config& a,
    const TerrainConstraintGenerator::Config& b) {
    return a.max_buildable_slope_deg == b.max_buildable_slope_deg &&
        a.hostile_terrain_slope_deg == b.hostile_terrain_slope_deg &&
        a.min_buildable_fraction == b.min_buildable_fraction &&
        a.fragmentation_threshold == b.fragmentation_threshold &&
        a.policy_friction_threshold == b.policy_friction_threshold &&
        a.erosion_iterations == b.erosion_iterations;
}

[[nodiscard]] bool ValidatorConfigEqual(
    const PlanValidatorGenerator::Config& a,
    const PlanValidatorGenerator::Config& b) {
    return a.max_road_slope_deg == b.max_road_slope_deg &&
        a.max_lot_slope_deg == b.max_lot_slope_deg &&
        a.max_road_flood_level == b.max_road_flood_level &&
        a.max_lot_flood_level == b.max_lot_flood_level &&
        a.min_soil_strength == b.min_soil_strength &&
        a.max_policy_friction == b.max_policy_friction;
}

[[nodiscard]] bool ConfigEquivalent(
    const CityGenerator::Config& a,
    const CityGenerator::Config& b) {
    return a.width == b.width &&
        a.height == b.height &&
        a.cell_size == b.cell_size &&
        a.seed == b.seed &&
        a.num_seeds == b.num_seeds &&
        a.max_districts == b.max_districts &&
        a.max_lots == b.max_lots &&
        a.max_buildings == b.max_buildings &&
        a.enable_world_constraints == b.enable_world_constraints &&
        a.max_texture_resolution == b.max_texture_resolution &&
        a.incremental_mode == b.incremental_mode &&
        a.max_iterations_per_axiom == b.max_iterations_per_axiom &&
        a.adaptive_tracing == b.adaptive_tracing &&
        a.enforce_road_separation == b.enforce_road_separation &&
        a.min_trace_step_size == b.min_trace_step_size &&
        a.max_trace_step_size == b.max_trace_step_size &&
        a.trace_curvature_gain == b.trace_curvature_gain &&
        TerrainConfigEqual(a.terrain, b.terrain) &&
        ValidatorConfigEqual(a.validator, b.validator);
}

} // namespace

CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms) {
    return generate(axioms, Config{});
}

CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms,
    const Config& config) {
    return generate(axioms, config, nullptr);
}

CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    Core::Editor::GlobalState* global_state) {
    StageOptions options{};
    options.stages_to_run = FullStageMask();
    options.use_cache = false;
    return GenerateStages(axioms, config, options, global_state, nullptr);
}

CityGenerator::CityOutput CityGenerator::GenerateWithContext(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    GenerationContext* context,
    Core::Editor::GlobalState* global_state) {
    StageOptions options{};
    options.stages_to_run = FullStageMask();
    options.use_cache = false;
    return GenerateStages(axioms, config, options, global_state, context);
}

CityGenerator::CityOutput CityGenerator::RegenerateIncremental(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    StageMask dirty_stages,
    Core::Editor::GlobalState* global_state,
    GenerationContext* context) {
    if (dirty_stages.none()) {
        dirty_stages = FullStageMask();
    }
    CascadeDirty(dirty_stages);

    StageOptions options{};
    options.stages_to_run = dirty_stages;
    options.use_cache = true;
    options.cascade_downstream = true;
    return GenerateStages(axioms, config, options, global_state, context);
}

CityGenerator::ValidationResult CityGenerator::ValidateAndClampConfig(const Config& config) {
    ValidationResult result{};
    result.clamped_config = config;

    auto clamp_int = [&result](int& value, int min_v, int max_v, const char* label) {
        const int original = value;
        value = std::clamp(value, min_v, max_v);
        if (value != original) {
            result.warnings.emplace_back(std::string("Clamped ") + label + " to " + std::to_string(value));
        }
    };

    auto clamp_double = [&result](double& value, double min_v, double max_v, const char* label) {
        const double original = value;
        value = std::clamp(value, min_v, max_v);
        if (value != original) {
            result.warnings.emplace_back(std::string("Clamped ") + label + " to " + std::to_string(value));
        }
    };

    clamp_int(result.clamped_config.width, 500, 8192, "width");
    clamp_int(result.clamped_config.height, 500, 8192, "height");
    clamp_double(result.clamped_config.cell_size, 1.0, 50.0, "cell_size");
    clamp_int(result.clamped_config.num_seeds, 5, 200, "num_seeds");

    if (result.clamped_config.max_texture_resolution <= 64) {
        result.warnings.emplace_back("max_texture_resolution too low; raised to 64");
        result.clamped_config.max_texture_resolution = 64;
    }

    const double world_max = static_cast<double>(std::max(result.clamped_config.width, result.clamped_config.height));
    int implied_resolution = 1;
    if (result.clamped_config.cell_size > 0.0) {
        implied_resolution = static_cast<int>(std::ceil(world_max / result.clamped_config.cell_size));
    }

    if (implied_resolution > result.clamped_config.max_texture_resolution) {
        const double adjusted = world_max / static_cast<double>(result.clamped_config.max_texture_resolution);
        result.clamped_config.cell_size = std::max(1.0, adjusted);
        result.warnings.emplace_back(
            "Adjusted cell_size to satisfy max_texture_resolution limit: " +
            std::to_string(result.clamped_config.cell_size));
    }

    DeriveConstraintsFromWorldSize(result.clamped_config, result);

    if (result.clamped_config.seed == 0u) {
        result.warnings.emplace_back("seed was 0; replaced with 1 for deterministic runs");
        result.clamped_config.seed = 1u;
    }

    if (result.clamped_config.max_iterations_per_axiom <= 0) {
        result.warnings.emplace_back("max_iterations_per_axiom <= 0; raised to 1");
        result.clamped_config.max_iterations_per_axiom = 1;
    }

    result.clamped_config.min_trace_step_size =
        std::clamp(result.clamped_config.min_trace_step_size, 0.5, 50.0);
    result.clamped_config.max_trace_step_size =
        std::clamp(result.clamped_config.max_trace_step_size, 0.5, 100.0);
    if (result.clamped_config.max_trace_step_size < result.clamped_config.min_trace_step_size) {
        std::swap(result.clamped_config.max_trace_step_size, result.clamped_config.min_trace_step_size);
        result.warnings.emplace_back("trace step bounds were inverted and have been swapped");
    }
    result.clamped_config.trace_curvature_gain =
        std::clamp(result.clamped_config.trace_curvature_gain, 0.1, 8.0);

    result.valid = result.errors.empty();
    return result;
}

bool CityGenerator::ValidateAxioms(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    std::vector<std::string>* errors) {
    std::vector<std::string> local_errors;
    if (axioms.empty()) {
        local_errors.emplace_back("At least one axiom is required");
    }

    for (size_t i = 0; i < axioms.size(); ++i) {
        const auto& axiom = axioms[i];
        const std::string prefix = "Axiom[" + std::to_string(i) + "]: ";

        const bool in_bounds =
            axiom.position.x >= 0.0 &&
            axiom.position.y >= 0.0 &&
            axiom.position.x <= static_cast<double>(config.width) &&
            axiom.position.y <= static_cast<double>(config.height);
        if (!in_bounds) {
            local_errors.push_back(prefix + "position out of world bounds");
        }

        if (!(axiom.radius > 0.0) || !std::isfinite(axiom.radius)) {
            local_errors.push_back(prefix + "radius must be finite and > 0");
        }

        if (!(axiom.decay > 1.0) || !std::isfinite(axiom.decay)) {
            local_errors.push_back(prefix + "decay must be finite and > 1.0");
        }

        if (!std::isfinite(axiom.theta)) {
            local_errors.push_back(prefix + "theta must be finite");
        }

        switch (axiom.type) {
            case AxiomInput::Type::Organic:
                if (axiom.organic_curviness < 0.0f || axiom.organic_curviness > 1.0f) {
                    local_errors.push_back(prefix + "organic_curviness must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Radial:
                if (axiom.radial_spokes < 3 || axiom.radial_spokes > 24) {
                    local_errors.push_back(prefix + "radial_spokes must be in [3,24]");
                }
                break;
            case AxiomInput::Type::LooseGrid:
                if (axiom.loose_grid_jitter < 0.0f || axiom.loose_grid_jitter > 1.0f) {
                    local_errors.push_back(prefix + "loose_grid_jitter must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Suburban:
                if (axiom.suburban_loop_strength < 0.0f || axiom.suburban_loop_strength > 1.0f) {
                    local_errors.push_back(prefix + "suburban_loop_strength must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Stem:
                if (!(axiom.stem_branch_angle > 0.0f) || !std::isfinite(axiom.stem_branch_angle)) {
                    local_errors.push_back(prefix + "stem_branch_angle must be finite and > 0");
                }
                break;
            case AxiomInput::Type::Superblock:
                if (!(axiom.superblock_block_size > 0.0f) || !std::isfinite(axiom.superblock_block_size)) {
                    local_errors.push_back(prefix + "superblock_block_size must be finite and > 0");
                }
                break;
            case AxiomInput::Type::Grid:
            case AxiomInput::Type::Hexagonal:
            case AxiomInput::Type::Linear:
            case AxiomInput::Type::GridCorrective:
            case AxiomInput::Type::COUNT:
                break;
        }
    }

    if (errors != nullptr) {
        *errors = std::move(local_errors);
        return errors->empty();
    }
    return local_errors.empty();
}

uint64_t CityGenerator::HashAxioms(const std::vector<AxiomInput>& axioms) {
    uint64_t hash = kFnvOffsetBasis;
    const uint64_t count = static_cast<uint64_t>(axioms.size());
    HashScalar(count, hash);

    for (const auto& axiom : axioms) {
        HashScalar(static_cast<uint8_t>(axiom.type), hash);
        HashVec2(axiom.position, hash);
        HashDouble(axiom.radius, hash);
        HashDouble(axiom.theta, hash);
        HashDouble(axiom.decay, hash);
        HashFloat(axiom.organic_curviness, hash);
        HashScalar(axiom.radial_spokes, hash);
        HashFloat(axiom.loose_grid_jitter, hash);
        HashFloat(axiom.suburban_loop_strength, hash);
        HashFloat(axiom.stem_branch_angle, hash);
        HashFloat(axiom.superblock_block_size, hash);
    }

    return hash;
}

void CityGenerator::DeriveConstraintsFromWorldSize(Config& config, ValidationResult& result) {
    const double area_km2 =
        (static_cast<double>(config.width) * static_cast<double>(config.height)) / 1'000'000.0;

    // Capacity heuristics from the deterministic refactor plan:
    // - districts: ~1 per 0.25 km^2
    // - lots: ~2500 per km^2
    // - buildings: up to 2x lots
    const uint32_t derived_district_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(std::llround(area_km2 / 0.25)),
        16ll,
        4096ll));

    const uint32_t derived_lot_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(std::llround(area_km2 * 2500.0)),
        500ll,
        500000ll));

    const uint32_t derived_building_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(derived_lot_cap) * 2ll,
        1000ll,
        1000000ll));

    if (config.max_districts > derived_district_cap) {
        result.warnings.emplace_back(
            "max_districts exceeded derived cap; clamped to " + std::to_string(derived_district_cap));
        config.max_districts = derived_district_cap;
    }
    if (config.max_lots > derived_lot_cap) {
        result.warnings.emplace_back(
            "max_lots exceeded derived cap; clamped to " + std::to_string(derived_lot_cap));
        config.max_lots = derived_lot_cap;
    }
    if (config.max_buildings > derived_building_cap) {
        result.warnings.emplace_back(
            "max_buildings exceeded derived cap; clamped to " + std::to_string(derived_building_cap));
        config.max_buildings = derived_building_cap;
    }

    config.max_districts = std::max<uint32_t>(config.max_districts, 16u);
    config.max_lots = std::max<uint32_t>(config.max_lots, 500u);
    config.max_buildings = std::max<uint32_t>(config.max_buildings, 1000u);
}

void CityGenerator::InvalidateCacheIfInputChanged(const Config& config, uint64_t axioms_hash) {
    if (!cache_.has_signature ||
        !ConfigEquivalent(cache_.config, config) ||
        cache_.axioms_hash != axioms_hash) {
        cache_.valid_stages.reset();
        cache_.config = config;
        cache_.axioms_hash = axioms_hash;
        cache_.has_signature = true;
    }
}

bool CityGenerator::ShouldAbort(GenerationContext* context) const {
    return context != nullptr && context->ShouldAbort();
}

CityGenerator::CityOutput CityGenerator::GenerateStages(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    const StageOptions& options,
    Core::Editor::GlobalState* global_state,
    GenerationContext* context) {
    CityOutput output{};

    const ValidationResult validation = ValidateAndClampConfig(config);
    config_ = validation.clamped_config;
    rng_ = RNG(config_.seed);

#ifndef NDEBUG
    for (const auto& warning : validation.warnings) {
        std::cerr << "[CityGenerator::ValidateAndClampConfig] " << warning << '\n';
    }
#endif

    std::vector<std::string> axiom_errors;
    if (!validation.ok() || !ValidateAxioms(axioms, config_, &axiom_errors)) {
        output.plan_approved = false;
        for (const auto& err : validation.errors) {
            Core::PlanViolation violation{};
            violation.type = Core::PlanViolationType::PolicyConflict;
            violation.entity_type = Core::PlanEntityType::Global;
            violation.severity = 1.0f;
            violation.message = err;
            output.plan_violations.push_back(std::move(violation));
        }
        for (const auto& err : axiom_errors) {
            Core::PlanViolation violation{};
            violation.type = Core::PlanViolationType::PolicyConflict;
            violation.entity_type = Core::PlanEntityType::Global;
            violation.severity = 1.0f;
            violation.message = err;
            output.plan_violations.push_back(std::move(violation));
        }
        return output;
    }

    StageMask dirty = options.stages_to_run;
    if (dirty.none()) {
        dirty = FullStageMask();
    }
    if (options.cascade_downstream) {
        CascadeDirty(dirty);
    }

    const uint64_t axioms_hash = HashAxioms(axioms);
    InvalidateCacheIfInputChanged(config_, axioms_hash);
    if (!options.use_cache) {
        cache_.valid_stages.reset();
    }

    const auto should_run_stage = [&](GenerationStage stage) {
        const bool dirty_stage = IsStageDirty(dirty, stage);
        const bool cached_stage = cache_.valid_stages.test(StageIndex(stage));
        return !options.use_cache || dirty_stage || !cached_stage;
    };

    const auto copy_cache_to_output = [&]() {
        output.world_constraints = cache_.world_constraints;
        output.site_profile = cache_.site_profile;
        output.tensor_field = cache_.tensor_field;
        output.roads = cache_.roads;
        output.districts = cache_.districts;
        output.blocks = cache_.blocks;
        output.lots = cache_.lots;
        output.buildings = cache_.buildings;
        output.plan_violations = cache_.plan_violations;
        output.plan_approved = cache_.plan_approved;
    };

    // Stage 0: Terrain constraints and site profile.
    if (config_.enable_world_constraints) {
        if (should_run_stage(GenerationStage::Terrain)) {
            TerrainConstraintGenerator terrain;
            TerrainConstraintGenerator::Input terrain_input;
            terrain_input.world_width = config_.width;
            terrain_input.world_height = config_.height;
            terrain_input.cell_size = config_.cell_size;
            terrain_input.seed = config_.seed;
            auto terrain_output = terrain.generate(terrain_input, config_.terrain);
            cache_.world_constraints = std::move(terrain_output.constraints);
            cache_.site_profile = terrain_output.profile;
            cache_.valid_stages.set(StageIndex(GenerationStage::Terrain), true);
        }
    } else {
        cache_.world_constraints = WorldConstraintField{};
        cache_.site_profile = SiteProfile{};
        cache_.valid_stages.set(StageIndex(GenerationStage::Terrain), true);
    }

    const WorldConstraintField* constraints = cache_.world_constraints.isValid() ? &cache_.world_constraints : nullptr;
    initializeTextureSpaceIfNeeded(global_state, constraints);
    const Core::Data::TextureSpace* texture_space =
        (global_state != nullptr && global_state->HasTextureSpace()) ? &global_state->TextureSpaceRef() : nullptr;
    if (texture_space != nullptr && constraints != nullptr) {
        AssertTextureMatchesConstraints(*texture_space, *constraints);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 1: Tensor field.
    if (should_run_stage(GenerationStage::TensorField)) {
        cache_.tensor_field = generateTensorField(axioms, context);
        cache_.valid_stages.set(StageIndex(GenerationStage::TensorField), true);
    }

    if (global_state != nullptr && global_state->HasTextureSpace()) {
        auto& texture_space_mut = global_state->TextureSpaceRef();
        cache_.tensor_field.writeToTextureSpace(texture_space_mut);
        cache_.tensor_field.bindTextureSpace(&texture_space_mut);
        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Tensor);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 2: Roads.
    if (should_run_stage(GenerationStage::Roads)) {
        const SiteProfile* site_profile = constraints != nullptr ? &cache_.site_profile : nullptr;
        std::vector<Vec2> seeds = generateSeeds(
            constraints,
            texture_space,
            context,
            options.constrain_seeds_to_axiom_bounds ? &axioms : nullptr);
        if (ShouldAbort(context)) {
            copy_cache_to_output();
            output.plan_approved = false;
            return output;
        }

        cache_.roads = traceRoads(
            cache_.tensor_field,
            seeds,
            constraints,
            site_profile,
            texture_space,
            context);
        if (options.constrain_roads_to_axiom_bounds) {
            cache_.roads = ClipRoadsToAxiomInfluence(cache_.roads, axioms);
        }
        cache_.valid_stages.set(StageIndex(GenerationStage::Roads), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 3: Districts.
    if (should_run_stage(GenerationStage::Districts)) {
        cache_.districts = classifyDistricts(cache_.roads, constraints, context);
        cache_.valid_stages.set(StageIndex(GenerationStage::Districts), true);
    }

    if (global_state != nullptr && global_state->HasTextureSpace()) {
        auto& texture_space_mut = global_state->TextureSpaceRef();
        rasterizeDistrictZonesToTexture(cache_.districts, texture_space_mut);
        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Zone);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 4: Blocks.
    if (should_run_stage(GenerationStage::Blocks)) {
        cache_.blocks = generateBlocks(cache_.districts, context);
        cache_.valid_stages.set(StageIndex(GenerationStage::Blocks), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 5: Lots.
    if (should_run_stage(GenerationStage::Lots)) {
        const SiteProfile* site_profile = constraints != nullptr ? &cache_.site_profile : nullptr;
        cache_.lots = generateLots(cache_.roads, cache_.districts, cache_.blocks, site_profile, context);
        cache_.valid_stages.set(StageIndex(GenerationStage::Lots), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 6: Buildings.
    if (should_run_stage(GenerationStage::Buildings)) {
        const SiteProfile* site_profile = constraints != nullptr ? &cache_.site_profile : nullptr;
        cache_.buildings = generateBuildings(cache_.lots, site_profile, context);
        if (texture_space != nullptr) {
            cache_.buildings = filterBuildingsByTexture(
                cache_.buildings,
                cache_.lots,
                cache_.districts,
                *texture_space);
        }
        cache_.valid_stages.set(StageIndex(GenerationStage::Buildings), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        output.plan_approved = false;
        return output;
    }

    // Stage 7: Validation.
    if (constraints != nullptr) {
        if (should_run_stage(GenerationStage::Validation)) {
            PlanValidatorGenerator validator;
            PlanValidatorGenerator::Input validator_input;
            validator_input.constraints = constraints;
            validator_input.site_profile = &cache_.site_profile;
            validator_input.roads = &cache_.roads;
            validator_input.lots = &cache_.lots;
            auto validation_output = validator.validate(validator_input, config_.validator);
            cache_.plan_violations = std::move(validation_output.violations);
            cache_.plan_approved = validation_output.approved;
            cache_.valid_stages.set(StageIndex(GenerationStage::Validation), true);
        }
    } else {
        cache_.plan_violations.clear();
        cache_.plan_approved = true;
        cache_.valid_stages.set(StageIndex(GenerationStage::Validation), true);
    }

    copy_cache_to_output();
    return output;
}

TensorFieldGenerator CityGenerator::generateTensorField(
    const std::vector<AxiomInput>& axioms,
    GenerationContext* context) {
    TensorFieldGenerator::Config field_config;
    field_config.width = std::max(1, static_cast<int>(config_.width / config_.cell_size));
    field_config.height = std::max(1, static_cast<int>(config_.height / config_.cell_size));
    field_config.cell_size = config_.cell_size;

    TensorFieldGenerator field(field_config);

    for (const auto& axiom : axioms) {
        if (ShouldAbort(context)) {
            break;
        }

        switch (axiom.type) {
            case AxiomInput::Type::Organic:
                field.addOrganicField(axiom.position, axiom.radius, axiom.theta, axiom.organic_curviness, axiom.decay);
                break;
            case AxiomInput::Type::Grid:
                field.addGridField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                break;
            case AxiomInput::Type::Radial:
                field.addRadialField(axiom.position, axiom.radius, axiom.radial_spokes, axiom.decay);
                break;
            case AxiomInput::Type::Hexagonal:
                field.addHexagonalField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                break;
            case AxiomInput::Type::Stem:
                field.addStemField(axiom.position, axiom.radius, axiom.theta, axiom.stem_branch_angle, axiom.decay);
                break;
            case AxiomInput::Type::LooseGrid:
                field.addLooseGridField(axiom.position, axiom.radius, axiom.theta, axiom.loose_grid_jitter, axiom.decay);
                break;
            case AxiomInput::Type::Suburban:
                field.addSuburbanField(axiom.position, axiom.radius, axiom.suburban_loop_strength, axiom.decay);
                break;
            case AxiomInput::Type::Superblock:
                field.addSuperblockField(axiom.position, axiom.radius, axiom.theta, axiom.superblock_block_size, axiom.decay);
                break;
            case AxiomInput::Type::Linear:
                field.addLinearField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                break;
            case AxiomInput::Type::GridCorrective:
                field.addGridCorrective(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                break;
            case AxiomInput::Type::COUNT:
                break;
        }

        if (context != nullptr) {
            context->BumpIterations();
        }
    }

    if (!ShouldAbort(context)) {
        field.generateField();
    }
    return field;
}

std::vector<Vec2> CityGenerator::generateSeeds(
    const WorldConstraintField* constraints,
    const Core::Data::TextureSpace* texture_space,
    GenerationContext* context,
    const std::vector<AxiomInput>* seed_axiom_bounds) {
    std::vector<Vec2> seeds;
    seeds.reserve(static_cast<size_t>(std::max(1, config_.num_seeds)));

    const auto sample_seed = [&]() {
        if (seed_axiom_bounds != nullptr && !seed_axiom_bounds->empty()) {
            const int max_index = static_cast<int>(seed_axiom_bounds->size()) - 1;
            const int index = std::clamp(rng_.uniformInt(0, max_index), 0, max_index);
            const auto& axiom = (*seed_axiom_bounds)[static_cast<size_t>(index)];
            const double radius = std::max(8.0, axiom.radius);
            const double angle = rng_.uniform(0.0, 2.0 * M_PI);
            const double radial = std::sqrt(rng_.uniform(0.0, 1.0)) * radius;
            const Vec2 local(std::cos(angle) * radial, std::sin(angle) * radial);
            return Vec2(
                std::clamp(axiom.position.x + local.x, 0.0, static_cast<double>(config_.width)),
                std::clamp(axiom.position.y + local.y, 0.0, static_cast<double>(config_.height)));
        }
        return Vec2(
            rng_.uniform(0.0, static_cast<double>(config_.width)),
            rng_.uniform(0.0, static_cast<double>(config_.height)));
    };

    const int max_attempts = std::max(8, config_.num_seeds * 25);
    int attempts = 0;
    while (static_cast<int>(seeds.size()) < config_.num_seeds && attempts < max_attempts) {
        if (ShouldAbort(context)) {
            break;
        }

        ++attempts;
        if (context != nullptr) {
            context->BumpIterations();
        }

        Vec2 seed = sample_seed();

        if (texture_space != nullptr) {
            const Vec2 uv = texture_space->coordinateSystem().worldToUV(seed);
            const uint8_t material = texture_space->materialLayer().sampleNearest(uv);
            const bool blocked = Core::Data::DecodeMaterialNoBuild(material);
            const uint8_t flood = Core::Data::DecodeMaterialFloodMask(material);
            if (blocked || flood > 1u) {
                continue;
            }
        }

        if (constraints != nullptr && constraints->isValid()) {
            if (constraints->sampleNoBuild(seed)) {
                continue;
            }
            if (constraints->sampleFloodMask(seed) > 1u) {
                continue;
            }
        }
        seeds.push_back(seed);
    }

    if (seeds.empty()) {
        if (seed_axiom_bounds != nullptr && !seed_axiom_bounds->empty()) {
            for (const auto& axiom : *seed_axiom_bounds) {
                seeds.push_back(Vec2(
                    std::clamp(axiom.position.x, 0.0, static_cast<double>(config_.width)),
                    std::clamp(axiom.position.y, 0.0, static_cast<double>(config_.height))));
            }
        }
        if (seeds.empty()) {
            seeds.push_back(Vec2(
                static_cast<double>(config_.width) * 0.5,
                static_cast<double>(config_.height) * 0.5));
        }
    }

    return seeds;
}

fva::Container<Road> CityGenerator::traceRoads(
    const TensorFieldGenerator& field,
    const std::vector<Vec2>& seeds,
    const WorldConstraintField* constraints,
    const SiteProfile* profile,
    const Core::Data::TextureSpace* texture_space,
    GenerationContext* context) {
    if (ShouldAbort(context)) {
        return {};
    }

    if (context != nullptr) {
        context->BumpIterations();
    }

    std::vector<Vec2> active_seeds;
    active_seeds.reserve(seeds.size());
    for (const auto& seed : seeds) {
        if (ShouldAbort(context)) {
            break;
        }
        active_seeds.push_back(seed);
        if (context != nullptr) {
            context->BumpIterations();
        }
    }
    if (active_seeds.empty()) {
        return {};
    }

    Urban::RoadGenerator::Config road_cfg;
    road_cfg.tracing.step_size = 5.0;
    road_cfg.tracing.max_length = 800.0;
    road_cfg.tracing.bidirectional = true;
    road_cfg.tracing.max_iterations = 500;
    road_cfg.tracing.texture_space = texture_space;
    road_cfg.tracing.adaptive_step_size = config_.adaptive_tracing;
    road_cfg.tracing.enforce_network_separation = config_.enforce_road_separation;
    road_cfg.tracing.min_step_size = config_.min_trace_step_size;
    road_cfg.tracing.max_step_size = config_.max_trace_step_size;
    road_cfg.tracing.curvature_gain = config_.trace_curvature_gain;
    road_cfg.tracing.separation_cell_size = std::max(road_cfg.tracing.min_separation, road_cfg.tracing.step_size * 2.0);

    if (constraints != nullptr && constraints->isValid()) {
        road_cfg.tracing.constraints = constraints;
        road_cfg.tracing.max_slope_degrees = 30.0;
        road_cfg.tracing.max_flood_level = 1u;
        road_cfg.tracing.min_soil_strength = 0.14f;
        if (profile != nullptr && profile->mode == GenerationMode::ConservationOnly) {
            road_cfg.tracing.max_length = 520.0;
            road_cfg.tracing.max_flood_level = 0u;
        } else if (profile != nullptr && profile->mode == GenerationMode::HillTown) {
            road_cfg.tracing.max_slope_degrees = 34.0;
        }
    }

    return Urban::RoadGenerator::generate(active_seeds, field, road_cfg);
}

std::vector<District> CityGenerator::classifyDistricts(
    const fva::Container<Road>& roads,
    const WorldConstraintField* constraints,
    GenerationContext* context) {
    Urban::DistrictGenerator::Config district_cfg;
    district_cfg.grid_resolution = std::clamp(
        static_cast<int>(std::sqrt(std::max<size_t>(4, roads.size() / 4))),
        4,
        14);
    district_cfg.max_districts = config_.max_districts;

    Bounds bounds;
    bounds.min = Vec2(0.0, 0.0);
    bounds.max = Vec2(static_cast<double>(config_.width), static_cast<double>(config_.height));
    auto districts = Urban::DistrictGenerator::generate(roads, bounds, district_cfg);

    if (constraints != nullptr && constraints->isValid()) {
        for (auto& district : districts) {
            if (ShouldAbort(context)) {
                break;
            }

            if (context != nullptr) {
                context->BumpIterations();
            }

            if (district.border.empty()) {
                continue;
            }
            Vec2 centroid{};
            for (const auto& p : district.border) {
                centroid += p;
            }
            centroid /= static_cast<double>(district.border.size());

            const uint8_t tags = constraints->sampleHistoryTags(centroid);
            if (HasHistoryTag(tags, HistoryTag::SacredSite)) {
                district.type = DistrictType::Civic;
            } else if (HasHistoryTag(tags, HistoryTag::Brownfield) ||
                       HasHistoryTag(tags, HistoryTag::Contaminated)) {
                district.type = DistrictType::Industrial;
            }

            const float nature = constraints->sampleNatureScore(centroid);
            const float slope = constraints->sampleSlopeDegrees(centroid);
            const float flood_penalty = constraints->sampleFloodMask(centroid) > 0u ? 0.18f : 0.0f;
            district.desirability = std::clamp(1.0f - (slope / 45.0f) + nature * 0.25f - flood_penalty, 0.0f, 1.0f);
        }
    }

    return districts;
}

std::vector<BlockPolygon> CityGenerator::generateBlocks(
    const std::vector<District>& districts,
    GenerationContext* context) {
    if (ShouldAbort(context)) {
        return {};
    }
    if (context != nullptr) {
        context->BumpIterations(static_cast<uint64_t>(districts.size()));
    }

    Urban::BlockGenerator::Config block_cfg;
    block_cfg.prefer_road_cycles = false;
    return Urban::BlockGenerator::generate(districts, block_cfg);
}

std::vector<LotToken> CityGenerator::generateLots(
    const fva::Container<Road>& roads,
    const std::vector<District>& districts,
    const std::vector<BlockPolygon>& blocks,
    const SiteProfile* profile,
    GenerationContext* context) {
    if (ShouldAbort(context)) {
        return {};
    }

    if (context != nullptr) {
        context->BumpIterations(static_cast<uint64_t>(blocks.size()));
    }

    Urban::LotGenerator::Config lot_cfg;
    lot_cfg.min_lot_width = 10.0f;
    lot_cfg.max_lot_width = 45.0f;
    lot_cfg.min_lot_depth = 12.0f;
    lot_cfg.max_lot_depth = 55.0f;
    lot_cfg.max_lots = config_.max_lots;

    if (profile != nullptr) {
        const float buildable_scale = std::clamp(profile->buildable_fraction, 0.15f, 1.0f);
        lot_cfg.max_lots = std::max<uint32_t>(
            600u,
            static_cast<uint32_t>(static_cast<float>(lot_cfg.max_lots) * buildable_scale));

        if (profile->mode == GenerationMode::Patchwork) {
            lot_cfg.min_lot_width = 8.0f;
            lot_cfg.min_lot_depth = 10.0f;
        } else if (profile->mode == GenerationMode::ConservationOnly) {
            lot_cfg.min_lot_width = 14.0f;
            lot_cfg.min_lot_depth = 18.0f;
        }
    }

    return Urban::LotGenerator::generate(roads, districts, blocks, lot_cfg, config_.seed);
}

siv::Vector<BuildingSite> CityGenerator::generateBuildings(
    const std::vector<LotToken>& lots,
    const SiteProfile* profile,
    GenerationContext* context) {
    if (ShouldAbort(context)) {
        return {};
    }

    if (context != nullptr) {
        context->BumpIterations(static_cast<uint64_t>(lots.size()));
    }

    Urban::SiteGenerator::Config site_cfg;
    site_cfg.max_buildings = config_.max_buildings;
    if (profile != nullptr && profile->mode == GenerationMode::ConservationOnly) {
        site_cfg.max_buildings = std::max<uint32_t>(1000u, config_.max_buildings / 2u);
    }
    return Urban::SiteGenerator::generate(lots, site_cfg, config_.seed);
}

void CityGenerator::rasterizeDistrictZonesToTexture(
    const std::vector<District>& districts,
    Core::Data::TextureSpace& texture_space) const {
    auto& zone = texture_space.zoneLayer();
    if (zone.empty()) {
        return;
    }

    zone.fill(0u);
    const auto& coords = texture_space.coordinateSystem();

    for (const auto& district : districts) {
        if (district.border.size() < 3) {
            continue;
        }

        Bounds district_bounds{};
        district_bounds.min = district.border.front();
        district_bounds.max = district.border.front();
        for (const auto& p : district.border) {
            district_bounds.min.x = std::min(district_bounds.min.x, p.x);
            district_bounds.min.y = std::min(district_bounds.min.y, p.y);
            district_bounds.max.x = std::max(district_bounds.max.x, p.x);
            district_bounds.max.y = std::max(district_bounds.max.y, p.y);
        }

        const Core::Data::Int2 pmin = coords.worldToPixel(district_bounds.min);
        const Core::Data::Int2 pmax = coords.worldToPixel(district_bounds.max);
        const int x0 = std::clamp(std::min(pmin.x, pmax.x), 0, zone.width() - 1);
        const int x1 = std::clamp(std::max(pmin.x, pmax.x), 0, zone.width() - 1);
        const int y0 = std::clamp(std::min(pmin.y, pmax.y), 0, zone.height() - 1);
        const int y1 = std::clamp(std::max(pmin.y, pmax.y), 0, zone.height() - 1);
        const uint8_t zone_value = static_cast<uint8_t>(static_cast<uint8_t>(district.type) + 1u);

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                const Vec2 world = coords.pixelToWorld({ x, y });
                if (PointInPolygon(world, district.border)) {
                    zone.at(x, y) = zone_value;
                }
            }
        }
    }
}

siv::Vector<BuildingSite> CityGenerator::filterBuildingsByTexture(
    const siv::Vector<BuildingSite>& buildings,
    const std::vector<LotToken>& lots,
    const std::vector<District>& districts,
    const Core::Data::TextureSpace& texture_space) const {
    siv::Vector<BuildingSite> filtered;
    filtered.reserve(buildings.size());

    std::unordered_map<uint32_t, uint32_t> lot_to_district;
    lot_to_district.reserve(lots.size());
    for (const auto& lot : lots) {
        lot_to_district[lot.id] = lot.district_id;
    }

    std::unordered_map<uint32_t, DistrictType> district_type_by_id;
    district_type_by_id.reserve(districts.size());
    for (const auto& district : districts) {
        district_type_by_id[district.id] = district.type;
    }

    const auto& coords = texture_space.coordinateSystem();
    for (const auto& building : buildings) {
        const Vec2 uv = coords.worldToUV(building.position);
        const float slope = texture_space.distanceLayer().sampleBilinear(uv);
        const uint8_t material = texture_space.materialLayer().sampleNearest(uv);
        const bool blocked = Core::Data::DecodeMaterialNoBuild(material);
        if (blocked || slope > static_cast<float>(config_.terrain.max_buildable_slope_deg)) {
            continue;
        }

        const uint8_t zone_value = texture_space.zoneLayer().sampleNearest(uv);
        if (zone_value != 0u) {
            const auto lot_it = lot_to_district.find(building.lot_id);
            if (lot_it != lot_to_district.end()) {
                const auto district_it = district_type_by_id.find(lot_it->second);
                if (district_it != district_type_by_id.end()) {
                    const uint8_t expected = static_cast<uint8_t>(static_cast<uint8_t>(district_it->second) + 1u);
                    if (expected != zone_value) {
                        continue;
                    }
                }
            }
        }

        filtered.push_back(building);
    }

    return filtered;
}

void CityGenerator::initializeTextureSpaceIfNeeded(
    Core::Editor::GlobalState* global_state,
    const WorldConstraintField* constraints) const {
    if (global_state == nullptr) {
        return;
    }

    Bounds world_bounds{};
    world_bounds.min = Vec2(0.0, 0.0);
    world_bounds.max = Vec2(static_cast<double>(config_.width), static_cast<double>(config_.height));

    int resolution = 0;
    if (constraints != nullptr && constraints->isValid()) {
        resolution = std::max(constraints->width, constraints->height);
#if !defined(NDEBUG)
        [[maybe_unused]] const double constraints_world_width = static_cast<double>(constraints->width) * constraints->cell_size;
        [[maybe_unused]] const double constraints_world_height = static_cast<double>(constraints->height) * constraints->cell_size;
        [[maybe_unused]] const double tolerance = std::max(1.0, constraints->cell_size);
        assert(std::abs(constraints_world_width - static_cast<double>(config_.width)) <= tolerance);
        assert(std::abs(constraints_world_height - static_cast<double>(config_.height)) <= tolerance);
#endif
    }
    if (resolution <= 0 && config_.cell_size > 0.0) {
        const double meters = static_cast<double>(std::max(config_.width, config_.height));
        resolution = static_cast<int>(std::round(meters / config_.cell_size));
    }

    resolution = std::max(1, resolution);
    global_state->InitializeTextureSpace(world_bounds, resolution);

    if (constraints != nullptr && constraints->isValid() && global_state->HasTextureSpace()) {
        auto& texture_space = global_state->TextureSpaceRef();
        auto& height_layer = texture_space.heightLayer();
        auto& material_layer = texture_space.materialLayer();
        auto& distance_layer = texture_space.distanceLayer();
        const auto& coords = texture_space.coordinateSystem();

        for (int y = 0; y < height_layer.height(); ++y) {
            for (int x = 0; x < height_layer.width(); ++x) {
                const Vec2 world = coords.pixelToWorld({ x, y });
                height_layer.at(x, y) = constraints->sampleHeightMeters(world);
                material_layer.at(x, y) = Core::Data::EncodeMaterialSample(
                    constraints->sampleFloodMask(world),
                    constraints->sampleNoBuild(world));
                distance_layer.at(x, y) = constraints->sampleSlopeDegrees(world);
            }
        }

        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Height);
        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Material);
        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Distance);
        global_state->MarkTextureLayerDirty(Core::Data::TextureLayer::Tensor);
    }
}

} // namespace RogueCity::Generators
