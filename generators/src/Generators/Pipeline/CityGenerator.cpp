#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/AxiomInteractionResolver.hpp"
#include "RogueCity/Generators/Pipeline/MajorConnectorGraph.hpp"

#include "RogueCity/Core/Data/MaterialEncoding.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace RogueCity::Generators {

namespace {

// FNV-1a 64-bit constants used to build a deterministic signature of generation inputs.
// That signature drives stage-cache invalidation in incremental mode.
constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

// Appends raw bytes into the rolling FNV-1a hash.
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

// Normalizes signed zero before hashing so +0.0 and -0.0 produce the same hash.
void HashDouble(double value, uint64_t& hash) {
    if (value == 0.0) {
        value = 0.0;
    }
    HashScalar(value, hash);
}

// Float variant of signed-zero normalization.
void HashFloat(float value, uint64_t& hash) {
    if (value == 0.0f) {
        value = 0.0f;
    }
    HashScalar(value, hash);
}

// Hashes a 2D vector component-wise to keep field order explicit and stable.
void HashVec2(const Core::Vec2& value, uint64_t& hash) {
    HashDouble(value.x, hash);
    HashDouble(value.y, hash);
}

// Converts constraint-grid extents into world-space bounds.
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

// Debug-only guardrail that catches texture/constraint desynchronization early.
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

// Standard ray-casting point-in-polygon test.
// Used for district rasterization and other geometry classification paths.
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

// Returns true if the point falls inside the influence radius of any input axiom.
// A small minimum radius keeps tiny axioms from becoming impossible to hit.
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

// Splits roads into contiguous segments that remain inside axiom influence regions.
// Segments crossing outside influence are cut and emitted as separate roads.
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

// Lightweight centroid helper used for coarse spatial filtering.
[[nodiscard]] Core::Vec2 PolygonCentroid(const std::vector<Core::Vec2>& polygon) {
    if (polygon.empty()) {
        return {};
    }

    Core::Vec2 centroid{};
    for (const auto& p : polygon) {
        centroid += p;
    }
    centroid /= static_cast<double>(polygon.size());
    return centroid;
}

// Treats a polygon as "touching" axiom influence if its centroid falls within influence.
// This is intentionally coarse and fast; we do not perform full polygon overlap tests here.
[[nodiscard]] bool PolygonTouchesAnyAxiom(
    const std::vector<Core::Vec2>& polygon,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (polygon.empty()) {
        return false;
    }

    return PointInsideAnyAxiom(PolygonCentroid(polygon), axioms);
}

// Retains districts whose centroid-based footprint intersects any axiom influence zone.
[[nodiscard]] std::vector<Core::District> ClipDistrictsToAxiomInfluence(
    const std::vector<Core::District>& districts,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (axioms.empty()) {
        return districts;
    }

    std::vector<Core::District> clipped{};
    clipped.reserve(districts.size());
    for (const auto& district : districts) {
        if (PolygonTouchesAnyAxiom(district.border, axioms)) {
            clipped.push_back(district);
        }
    }
    return clipped;
}

// Applies the same influence clipping policy to lots.
// If boundary data exists we classify by polygon centroid, else by lot centroid.
[[nodiscard]] std::vector<Core::LotToken> ClipLotsToAxiomInfluence(
    const std::vector<Core::LotToken>& lots,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (axioms.empty()) {
        return lots;
    }

    std::vector<Core::LotToken> clipped{};
    clipped.reserve(lots.size());
    for (const auto& lot : lots) {
        const bool inside =
            !lot.boundary.empty()
            ? PolygonTouchesAnyAxiom(lot.boundary, axioms)
            : PointInsideAnyAxiom(lot.centroid, axioms);
        if (inside) {
            clipped.push_back(lot);
        }
    }
    return clipped;
}

// Building-level influence clip: keep only sites whose placement point remains in-bounds.
[[nodiscard]] siv::Vector<Core::BuildingSite> ClipBuildingsToAxiomInfluence(
    const siv::Vector<Core::BuildingSite>& buildings,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (axioms.empty()) {
        return buildings;
    }

    siv::Vector<Core::BuildingSite> clipped{};
    for (const auto& building : buildings) {
        if (PointInsideAnyAxiom(building.position, axioms)) {
            clipped.push_back(building);
        }
    }
    return clipped;
}

using BoostPoint = boost::geometry::model::d2::point_xy<double>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;

[[nodiscard]] std::vector<Core::Vec2> BuildCityBoundary(
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    std::vector<Core::Vec2> boundary{};
    if (axioms.empty()) {
        return boundary;
    }

    BoostPolygon sample_cloud{};
    auto& cloud = sample_cloud.outer();
    cloud.clear();
    cloud.reserve(axioms.size() * 16 + 1);

    double median_radius = 0.0;
    std::vector<double> radii;
    radii.reserve(axioms.size());
    for (const auto& axiom : axioms) {
        radii.push_back(std::max(20.0, axiom.radius * std::clamp(axiom.ring_schema.outskirts_ratio, 0.2, 1.5)));
    }
    std::sort(radii.begin(), radii.end());
    median_radius = radii[radii.size() / 2];
    const double buffer = std::max(25.0, median_radius * 0.12);

    constexpr int kSamples = 12;
    for (const auto& axiom : axioms) {
        const double r = std::max(20.0, axiom.radius * std::clamp(axiom.ring_schema.outskirts_ratio, 0.2, 1.5));
        for (int i = 0; i < kSamples; ++i) {
            const double angle = (2.0 * M_PI * static_cast<double>(i)) / static_cast<double>(kSamples);
            cloud.emplace_back(
                axiom.position.x + std::cos(angle) * r,
                axiom.position.y + std::sin(angle) * r);
        }
    }
    if (cloud.empty()) {
        return boundary;
    }
    const bool closed =
        std::abs(cloud.front().x() - cloud.back().x()) < 1e-9 &&
        std::abs(cloud.front().y() - cloud.back().y()) < 1e-9;
    if (!closed) {
        cloud.push_back(cloud.front());
    }
    boost::geometry::correct(sample_cloud);

    BoostPolygon hull{};
    boost::geometry::convex_hull(sample_cloud, hull);
    if (hull.outer().size() < 3) {
        return boundary;
    }

    Core::Vec2 centroid{};
    for (const auto& p : hull.outer()) {
        centroid += Core::Vec2(p.x(), p.y());
    }
    centroid /= static_cast<double>(hull.outer().size());

    boundary.reserve(hull.outer().size());
    for (const auto& p : hull.outer()) {
        Core::Vec2 v(p.x(), p.y());
        Core::Vec2 dir = v - centroid;
        const double len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 1e-8) {
            dir /= len;
            v += dir * buffer;
        }
        boundary.push_back(v);
    }

    if (boundary.size() > 1 && boundary.front().equals(boundary.back())) {
        boundary.pop_back();
    }
    return boundary;
}

void AssignRoadSourceAxiom(
    fva::Container<Core::Road>& roads,
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    if (axioms.empty()) {
        return;
    }
    for (auto& road : roads) {
        if (road.points.empty()) {
            continue;
        }
        const Core::Vec2 ref = road.points[road.points.size() / 2];
        int best_id = axioms.front().id;
        double best_distance = std::numeric_limits<double>::max();
        for (const auto& axiom : axioms) {
            const double d = ref.distanceTo(axiom.position);
            if (d < best_distance) {
                best_distance = d;
                best_id = axiom.id;
            }
        }
        road.source_axiom_id = best_id;
    }
}

// Field-by-field equality helpers for cache signature comparison.
// These are intentionally strict to guarantee deterministic cache correctness.
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

// Full configuration equivalence used to decide whether cached stage outputs remain valid.
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

// Convenience overload: uses default configuration and runs the full pipeline.
CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms) {
    return generate(axioms, Config{});
}

// Convenience overload: allows explicit config but no editor global-state integration.
CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms,
    const Config& config) {
    return generate(axioms, config, nullptr);
}

// Baseline full run entry point. This path always recomputes every stage.
CityGenerator::CityOutput CityGenerator::generate(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    Core::Editor::GlobalState* global_state) {
    StageOptions options{};
    options.stages_to_run = FullStageMask();
    options.use_cache = false;
    return GenerateStages(axioms, config, options, global_state, nullptr);
}

// Full run with cooperative cancellation/progress context.
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

// Incremental entry point:
// - caller marks dirty stages
// - downstream dependency stages are cascaded dirty
// - clean, still-valid stages may be reused from cache
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

// Validates and sanitizes config values into safe operating ranges.
// Any automatic correction is captured as a warning for diagnostics.
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
// Integer variant of the above helper.
    auto clamp_double = [&result](double& value, double min_v, double max_v, const char* label) {
        const double original = value;
        value = std::clamp(value, min_v, max_v);
        if (value != original) {
            result.warnings.emplace_back(std::string("Clamped ") + label + " to " + std::to_string(value));
        }
    };
// Range checks for basic world parameters. These are primarily to prevent pathological memory usage and ensure stable generation.
    clamp_int(result.clamped_config.width, 500, 8192, "width");
    clamp_int(result.clamped_config.height, 500, 8192, "height");
    clamp_double(result.clamped_config.cell_size, 1.0, 50.0, "cell_size");
    clamp_int(result.clamped_config.num_seeds, 5, 200, "num_seeds");

    if (result.clamped_config.max_texture_resolution <= 64) {
        result.warnings.emplace_back("max_texture_resolution too low; raised to 64");
        result.clamped_config.max_texture_resolution = 64;
    }
// The cell size and max texture resolution are interdependent based on world size, so we perform a preliminary clamping pass before validating their combined constraints.
    const double world_max = static_cast<double>(std::max(result.clamped_config.width, result.clamped_config.height));
    int implied_resolution = 1;
    if (result.clamped_config.cell_size > 0.0) {
        implied_resolution = static_cast<int>(std::ceil(world_max / result.clamped_config.cell_size));
    }
// If the implied resolution from the cell size exceeds the max texture resolution, we need to increase the cell size to avoid generating unusably large textures.
    if (implied_resolution > result.clamped_config.max_texture_resolution) {
        const double adjusted = world_max / static_cast<double>(result.clamped_config.max_texture_resolution);
        result.clamped_config.cell_size = std::max(1.0, adjusted);
        result.warnings.emplace_back(
            "Adjusted cell_size to satisfy max_texture_resolution limit: " +
            std::to_string(result.clamped_config.cell_size));
    }

// After clamping the basic world parameters, we can derive conservative upper bounds for district/lot/building counts to prevent pathological cases on small maps and unbounded memory growth on large maps.
    DeriveConstraintsFromWorldSize(result.clamped_config, result);

// Seed 0 is technically valid but can lead to silent failures in some downstream algorithms that use zero as a sentinel value, so we proactively clamp it to 1 and warn the user.
    if (result.clamped_config.seed == 0u) {
        result.warnings.emplace_back("seed was 0; replaced with 1 for deterministic runs");
        result.clamped_config.seed = 1u;
    }
// We allow users to set very high iteration counts for stress-testing and profiling, but we clamp the minimum to 1 to prevent no-op runs that can be confusing for new users.
    if (result.clamped_config.max_iterations_per_axiom <= 0) {
        result.warnings.emplace_back("max_iterations_per_axiom <= 0; raised to 1");
        result.clamped_config.max_iterations_per_axiom = 1;
    }
// Trace step sizes are clamped to reasonable ranges to prevent extreme values that can cause instability or excessively long generation times. We also enforce that the max step size is not smaller than the min step size, swapping them if necessary and issuing a warning.

    result.clamped_config.min_trace_step_size = // We allow very small step sizes for high-detail tracing, but we clamp the minimum to 0.5 to prevent excessively long generation times and potential numerical instability.

        std::clamp(result.clamped_config.min_trace_step_size, 0.5, 50.0); // We allow very large step sizes for fast, low-detail tracing, but we clamp the maximum to 100 to prevent completely skipping over important features and causing generation failures.

    result.clamped_config.max_trace_step_size = // We allow very large step sizes for fast, low-detail tracing, but we clamp the maximum to 100 to prevent completely skipping over important features and causing generation failures.

        std::clamp(result.clamped_config.max_trace_step_size, 0.5, 100.0); // we enforce that the max step size is not smaller than the min step size, swapping them if necessary and issuing a warning. 

        // If the max step size is smaller than the min step size, swap them and issue a warning. This is a common user error that can lead to confusing behavior, so we handle it gracefully rather than treating it as a hard error.
    if (result.clamped_config.max_trace_step_size < result.clamped_config.min_trace_step_size) {

        std::swap(result.clamped_config.max_trace_step_size, result.clamped_config.min_trace_step_size); // After swapping, we also clamp them again to ensure they remain within the valid ranges defined above, in case the original values were outside those ranges.
        result.clamped_config.min_trace_step_size = std::clamp(result.clamped_config.min_trace_step_size, 0.5, 50.0); // After swapping, we also clamp them again to ensure they remain within the valid ranges defined above, in case the original values were outside those ranges.
        result.clamped_config.max_trace_step_size = std::clamp(result.clamped_config.max_trace_step_size, 0.5, 100.0); // After swapping, we also clamp them again to ensure they remain within the valid ranges defined above, in case the original values were outside those ranges.
        
        result.warnings.emplace_back("trace step bounds were inverted and have been swapped"); // After swapping and clamping, we issue a warning to inform the user of the correction.
    }
    result.clamped_config.trace_curvature_gain =
        std::clamp(result.clamped_config.trace_curvature_gain, 0.1, 8.0);

    result.valid = result.errors.empty();
    return result;
}

// Performs semantic validation of axiom parameters against world bounds and type-specific limits.
// Returns true if all axioms are valid, otherwise populates the errors vector with validation messages.
bool CityGenerator::ValidateAxioms(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    std::vector<std::string>* errors) {
    std::vector<std::string> local_errors;
    std::unordered_set<int> seen_ids;
    seen_ids.reserve(axioms.size());

    // Ensure at least one axiom is provided.
    if (axioms.empty()) {
        local_errors.emplace_back("At least one axiom is required");
    }

    // Iterate through each axiom and validate its parameters.
    for (size_t i = 0; i < axioms.size(); ++i) {
        const auto& axiom = axioms[i];
        const std::string prefix = "Axiom[" + std::to_string(i) + "]: ";

        if (axiom.id > 0 && !seen_ids.insert(axiom.id).second) {
            local_errors.push_back(prefix + "duplicate id");
        }

        // Check if the axiom's position is within the world bounds.
        const bool in_bounds =
            axiom.position.x >= 0.0 &&
            axiom.position.y >= 0.0 &&
            axiom.position.x <= static_cast<double>(config.width) &&
            axiom.position.y <= static_cast<double>(config.height);
        if (!in_bounds) {
            local_errors.push_back(prefix + "position out of world bounds");
        }

        // Validate that the radius is positive and finite.
        if (!(axiom.radius > 0.0) || !std::isfinite(axiom.radius)) {
            local_errors.push_back(prefix + "radius must be finite and > 0");
        }

        // Validate that the decay factor is greater than 1 and finite.
        if (!(axiom.decay > 1.0) || !std::isfinite(axiom.decay)) {
            local_errors.push_back(prefix + "decay must be finite and > 1.0");
        }

        if (axiom.ring_schema.core_ratio <= 0.0 ||
            axiom.ring_schema.falloff_ratio < axiom.ring_schema.core_ratio ||
            axiom.ring_schema.outskirts_ratio < axiom.ring_schema.falloff_ratio ||
            axiom.ring_schema.outskirts_ratio > 1.5 ||
            axiom.ring_schema.merge_band_ratio < 0.0 ||
            axiom.ring_schema.merge_band_ratio > 0.5) {
            local_errors.push_back(prefix + "invalid ring schema ratios");
        }

        // Ensure the theta value is finite.
        if (!std::isfinite(axiom.theta)) {
            local_errors.push_back(prefix + "theta must be finite");
        }

        // Perform type-specific validation for the axiom.
        switch (axiom.type) {
            case AxiomInput::Type::Organic:
                // Validate organic_curviness is within [0, 1].
                if (axiom.organic_curviness < 0.0f || axiom.organic_curviness > 1.0f) {
                    local_errors.push_back(prefix + "organic_curviness must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Radial:
                // Validate radial_spokes is within [3, 24].
                if (axiom.radial_spokes < 3 || axiom.radial_spokes > 24) {
                    local_errors.push_back(prefix + "radial_spokes must be in [3,24]");
                }
                break;
            case AxiomInput::Type::LooseGrid:
                // Validate loose_grid_jitter is within [0, 1].
                if (axiom.loose_grid_jitter < 0.0f || axiom.loose_grid_jitter > 1.0f) {
                    local_errors.push_back(prefix + "loose_grid_jitter must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Suburban:
                // Validate suburban_loop_strength is within [0, 1].
                if (axiom.suburban_loop_strength < 0.0f || axiom.suburban_loop_strength > 1.0f) {
                    local_errors.push_back(prefix + "suburban_loop_strength must be in [0,1]");
                }
                break;
            case AxiomInput::Type::Stem:
                // Validate stem_branch_angle is positive and finite.
                if (!(axiom.stem_branch_angle > 0.0f) || !std::isfinite(axiom.stem_branch_angle)) {
                    local_errors.push_back(prefix + "stem_branch_angle must be finite and > 0");
                }
                break;
            case AxiomInput::Type::Superblock:
                // Validate superblock_block_size is positive and finite.
                if (!(axiom.superblock_block_size > 0.0f) || !std::isfinite(axiom.superblock_block_size)) {
                    local_errors.push_back(prefix + "superblock_block_size must be finite and > 0");
                }
                break;
                //todo - consider adding some validation for the grid/hex/linear types, such as ensuring the radius is above a certain threshold to prevent degenerate cases. For now we will allow any positive radius for these types since they can be used in creative ways and the existing bounds checks will prevent pathological cases.
            case AxiomInput::Type::Grid: //todo needs implamentation. 
            case AxiomInput::Type::Hexagonal: //todo needs implamentation. 
            case AxiomInput::Type::Linear: //todo needs implamentation. 
            case AxiomInput::Type::GridCorrective: //todo needs implamentation. 
            case AxiomInput::Type::COUNT: //todo needs implamentation. 
                break;
        }
    }

    // If an errors vector is provided, populate it with the validation results.
    if (errors != nullptr) {
        *errors = std::move(local_errors);
        return errors->empty();
    }
    return local_errors.empty();
}
//perhaps consider an underterminstic but fast hash for the axioms if performance of this becomes an issue, but for now we will prioritize correctness and simplicity with a deterministic FNV-1a implementation that treats floating-point values carefully to ensure consistent signatures across platforms and runs.

// Produces a deterministic hash across all axiom parameters.
// Combined with config equivalence, this forms the cache input signature.
//this function produces a deterministic hash of the axiom inputs by iterating through each axiom and hashing its fields in a consistent order. We use the FNV-1a algorithm for hashing, which is simple and has good distribution properties for small inputs. We also take care to normalize floating-point values to ensure that equivalent values produce the same hash, such as treating +0.0 and -0.0 as equal. The resulting hash value can be used as part of the cache signature to determine if cached stage outputs remain valid when axioms change.
uint64_t CityGenerator::HashAxioms(const std::vector<AxiomInput>& axioms) { 
    uint64_t hash = kFnvOffsetBasis;
    const uint64_t count = static_cast<uint64_t>(axioms.size());
    HashScalar(count, hash);
// We iterate through each axiom and hash its fields in a consistent order. The field order is explicitly defined here to ensure that the same set of axioms always produces the same hash, regardless of any internal memory layout or padding differences. We also normalize floating-point values to ensure that equivalent values produce the same hash, such as treating +0.0 and -0.0 as equal.
    for (const auto& axiom : axioms) {
        HashScalar(axiom.id, hash);
        HashScalar(static_cast<uint8_t>(axiom.type), hash);
        HashVec2(axiom.position, hash);
        HashDouble(axiom.radius, hash);
        HashDouble(axiom.theta, hash);
        HashDouble(axiom.decay, hash);
        HashDouble(axiom.ring_schema.core_ratio, hash);
        HashDouble(axiom.ring_schema.falloff_ratio, hash);
        HashDouble(axiom.ring_schema.outskirts_ratio, hash);
        HashDouble(axiom.ring_schema.merge_band_ratio, hash);
        HashScalar(static_cast<uint8_t>(axiom.ring_schema.strict_core ? 1u : 0u), hash);
        HashScalar(static_cast<uint8_t>(axiom.lock_generated_roads ? 1u : 0u), hash);
        HashFloat(axiom.organic_curviness, hash);
        HashScalar(axiom.radial_spokes, hash);
        HashFloat(axiom.loose_grid_jitter, hash);
        HashFloat(axiom.suburban_loop_strength, hash);
        HashFloat(axiom.stem_branch_angle, hash);
        HashFloat(axiom.superblock_block_size, hash);
    }

    return hash;
}

// Derives conservative upper caps for districts/lots/buildings from world area.
// This prevents pathological counts on small maps and unbounded memory growth on large maps.
void CityGenerator::DeriveConstraintsFromWorldSize(Config& config, ValidationResult& result) {
    const double area_km2 =
        (static_cast<double>(config.width) * static_cast<double>(config.height)) / 1'000'000.0;
    // Capacity heuristics from the deterministic refactor plan:
    const uint32_t derived_district_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(std::llround(area_km2 / 0.25)),
        16ll,
        4096ll));
    // - districts: ~1 per 0.25 km^2
    const uint32_t derived_lot_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(std::llround(area_km2 * 2500.0)),
        500ll,
        500000ll));
    // - lots: ~2500 per km^2
    const uint32_t derived_building_cap = static_cast<uint32_t>(std::clamp(
        static_cast<long long>(derived_lot_cap) * 2ll,
        1000ll,
        1000000ll));
    // - buildings: up to 2x lots
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

// Invalidates all cached stages when any input signature component changes.
void CityGenerator::InvalidateCacheIfInputChanged(const Config& config, uint64_t axioms_hash) {
    if (!cache_.has_signature ||
        !ConfigEquivalent(cache_.config, config) ||
        cache_.axioms_hash != axioms_hash) {
        cache_.valid_stages.reset();
        cache_.config = config;
        cache_.axioms_hash = axioms_hash;
        cache_.city_boundary.clear();
        cache_.connector_debug_edges.clear();
        cache_.has_signature = true;
    }
}

// Shared cancellation check used throughout long-running stage loops.
bool CityGenerator::ShouldAbort(GenerationContext* context) const {
    return context != nullptr && context->ShouldAbort();
}

// Core stage orchestrator.
// Responsibilities:
// 1) validate and clamp inputs
// 2) resolve stage dirtiness/caching strategy
// 3) execute requested stages in dependency order
// 4) publish cache-backed output and trim non-requested stages when needed
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

    std::vector<AxiomInput> resolved_axioms = axioms;
    std::vector<Core::Polyline> interaction_debug_edges{};
    if (global_state == nullptr || global_state->config.feature_axiom_overlap_resolution) {
        AxiomInteractionResolver resolver;
        const AxiomInteractionResult interaction = resolver.Resolve(axioms);
        resolved_axioms = interaction.resolved_axioms;
        interaction_debug_edges.reserve(interaction.falloff_borders.size());
        for (const auto& edge : interaction.falloff_borders) {
            Core::Polyline poly{};
            poly.points.push_back(edge.border_start);
            poly.points.push_back(edge.border_end);
            interaction_debug_edges.push_back(std::move(poly));
        }
    }
    for (size_t i = 0; i < resolved_axioms.size(); ++i) {
        if (resolved_axioms[i].id <= 0) {
            resolved_axioms[i].id = static_cast<int>(i + 1);
        }
    }

    // Resolve requested stages. Empty masks are treated as "run everything".
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

    // Stage decision helper:
    // - without cascading, run only explicitly dirty stages
    // - with cascading + cache, run dirty stages or stages missing in cache
    const auto should_run_stage = [&](GenerationStage stage) {
        const bool dirty_stage = IsStageDirty(dirty, stage);
        const bool cached_stage = cache_.valid_stages.test(StageIndex(stage));
        if (!options.cascade_downstream) {
            return dirty_stage;
        }
        if (!options.use_cache) {
            return dirty_stage;
        }
        return dirty_stage || !cached_stage;
    };

    // Materialize output from cache snapshot (the cache is the single source of truth between runs).
    const auto copy_cache_to_output = [&]() {
        output.world_constraints = cache_.world_constraints;
        output.site_profile = cache_.site_profile;
        output.tensor_field = cache_.tensor_field;
        output.roads = cache_.roads;
        output.districts = cache_.districts;
        output.blocks = cache_.blocks;
        output.lots = cache_.lots;
        output.buildings = cache_.buildings;
        output.city_boundary = cache_.city_boundary;
        output.connector_debug_edges = cache_.connector_debug_edges;
        output.plan_violations = cache_.plan_violations;
        output.plan_approved = cache_.plan_approved;
    };

    // In "exact stage" mode (no cascade), hide non-requested artifacts from returned output.
    // Cache is still retained; this only shapes what the caller receives.
    const auto trim_output_for_partial_stages = [&]() {
        if (options.cascade_downstream) {
            return;
        }

        if (!IsStageDirty(dirty, GenerationStage::Terrain)) {
            output.world_constraints = WorldConstraintField{};
            output.site_profile = SiteProfile{};
        }
        if (!IsStageDirty(dirty, GenerationStage::TensorField)) {
            output.tensor_field = TensorFieldGenerator{};
        }
        if (!IsStageDirty(dirty, GenerationStage::Roads)) {
            output.roads.clear();
            output.connector_debug_edges.clear();
        }
        if (!IsStageDirty(dirty, GenerationStage::Districts)) {
            output.districts.clear();
        }
        if (!IsStageDirty(dirty, GenerationStage::Blocks)) {
            output.blocks.clear();
        }
        if (!IsStageDirty(dirty, GenerationStage::Lots)) {
            output.lots.clear();
        }
        if (!IsStageDirty(dirty, GenerationStage::Buildings)) {
            output.buildings.clear();
        }
        if (!IsStageDirty(dirty, GenerationStage::Validation)) {
            output.plan_violations.clear();
            output.plan_approved = true;
        }
        if (!IsStageDirty(dirty, GenerationStage::Terrain) &&
            !IsStageDirty(dirty, GenerationStage::Roads)) {
            output.city_boundary.clear();
        }
    };

    // Stage 0: Terrain constraints and site profile.
    // Generates topography/environment masks plus a coarse site profile used by later stages.
    if (config_.enable_world_constraints) {
        // Check if the Terrain generation stage should be executed.
        if (should_run_stage(GenerationStage::Terrain)) {
            // Initialize the TerrainConstraintGenerator to generate terrain constraints.
            TerrainConstraintGenerator terrain;
            TerrainConstraintGenerator::Input terrain_input;

            // Set up the input parameters for terrain generation.
            terrain_input.world_width = config_.width; // Width of the world.
            terrain_input.world_height = config_.height; // Height of the world.
            terrain_input.cell_size = config_.cell_size; // Size of each cell in the grid.
            terrain_input.seed = config_.seed; // Seed for random generation.

            // Generate terrain constraints and site profile based on the input configuration.
            auto terrain_output = terrain.generate(terrain_input, config_.terrain);

            // Store the generated constraints and site profile in the cache.
            cache_.world_constraints = std::move(terrain_output.constraints);
            cache_.site_profile = terrain_output.profile;

            // Mark the Terrain stage as valid in the cache.
            cache_.valid_stages.set(StageIndex(GenerationStage::Terrain), true);
        }
    } else {
        // If world constraints are disabled, initialize empty constraints and site profile.
        cache_.world_constraints = WorldConstraintField{}; // Empty world constraints.
        cache_.site_profile = SiteProfile{}; // Empty site profile.

        // Mark the Terrain stage as valid in the cache.
        cache_.valid_stages.set(StageIndex(GenerationStage::Terrain), true);
    }

    // Keep texture space aligned with current world bounds/constraints before writing downstream layers.
    const WorldConstraintField* constraints = cache_.world_constraints.isValid() ? &cache_.world_constraints : nullptr;
    initializeTextureSpaceIfNeeded(global_state, constraints);
    const Core::Data::TextureSpace* texture_space =
        (global_state != nullptr && global_state->HasTextureSpace()) ? &global_state->TextureSpaceRef() : nullptr;
    if (texture_space != nullptr && constraints != nullptr) {
        AssertTextureMatchesConstraints(*texture_space, *constraints);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 1: Tensor field.
    // Encodes directional flow guidance for road tracing.
    if (should_run_stage(GenerationStage::TensorField)) {
        cache_.tensor_field = generateTensorField(resolved_axioms, context);
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
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 2: Roads.
    // Seeds and traces the navigable network from the tensor field.
    if (should_run_stage(GenerationStage::Roads)) {
        const SiteProfile* site_profile = constraints != nullptr ? &cache_.site_profile : nullptr;
        std::vector<Vec2> seeds = generateSeeds(
            constraints,
            texture_space,
            context,
            options.constrain_seeds_to_axiom_bounds ? &resolved_axioms : nullptr);
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
            cache_.roads = ClipRoadsToAxiomInfluence(cache_.roads, resolved_axioms);
        }

        if (global_state == nullptr || global_state->config.feature_major_connector_graph) {
            MajorConnectorGraph graph_builder;
            const MajorConnectorGraphOutput connector_graph = graph_builder.Build(resolved_axioms);
            std::unordered_map<int, Core::Vec2> axiom_positions{};
            axiom_positions.reserve(resolved_axioms.size());
            for (const auto& axiom : resolved_axioms) {
                axiom_positions[axiom.id] = axiom.position;
            }

            uint32_t next_road_id = 1u;
            for (const auto& road : cache_.roads) {
                next_road_id = std::max(next_road_id, road.id + 1u);
            }

            cache_.connector_debug_edges = interaction_debug_edges;
            for (const auto& edge : connector_graph.edges) {
                if (edge.first < 0 || edge.second < 0 ||
                    edge.first >= static_cast<int>(resolved_axioms.size()) ||
                    edge.second >= static_cast<int>(resolved_axioms.size())) {
                    continue;
                }
                const auto& a = resolved_axioms[static_cast<size_t>(edge.first)];
                const auto& b = resolved_axioms[static_cast<size_t>(edge.second)];

                Core::Road connector{};
                connector.id = next_road_id++;
                connector.type = Core::RoadType::Arterial;
                connector.source_axiom_id = a.id;
                connector.generation_tag = Core::GenerationTag::Generated;
                connector.generation_locked = false;
                connector.points.push_back(a.position);
                connector.points.push_back(b.position);
                cache_.roads.add(std::move(connector));

                Core::Polyline poly{};
                poly.points.push_back(a.position);
                poly.points.push_back(b.position);
                cache_.connector_debug_edges.push_back(std::move(poly));
            }
        } else {
            cache_.connector_debug_edges = interaction_debug_edges;
        }

        AssignRoadSourceAxiom(cache_.roads, resolved_axioms);
        cache_.valid_stages.set(StageIndex(GenerationStage::Roads), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 3: Districts.
    // Converts road network topology into district polygons/types.
    if (should_run_stage(GenerationStage::Districts)) {
        cache_.districts = classifyDistricts(cache_.roads, constraints, context);
        if (options.constrain_roads_to_axiom_bounds) {
            cache_.districts = ClipDistrictsToAxiomInfluence(cache_.districts, resolved_axioms);
        }
        cache_.valid_stages.set(StageIndex(GenerationStage::Districts), true);
    }

    if (global_state != nullptr && global_state->HasTextureSpace()) {
        auto& texture_space_mut = global_state->TextureSpaceRef();
        rasterizeDistrictZonesToTexture(cache_.districts, texture_space_mut);
        global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Zone);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 4: Blocks.
    // Subdivides district space into block-level polygons for lot generation.
    if (should_run_stage(GenerationStage::Blocks)) {
        cache_.blocks = generateBlocks(cache_.districts, context);
        cache_.valid_stages.set(StageIndex(GenerationStage::Blocks), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 5: Lots.
    // Packs buildable parcels into blocks, tuned by site profile and config caps.
    if (should_run_stage(GenerationStage::Lots)) {
        const SiteProfile* site_profile = constraints != nullptr ? &cache_.site_profile : nullptr;
        cache_.lots = generateLots(cache_.roads, cache_.districts, cache_.blocks, site_profile, context);
        if (options.constrain_roads_to_axiom_bounds) {
            cache_.lots = ClipLotsToAxiomInfluence(cache_.lots, resolved_axioms);
        }
        cache_.valid_stages.set(StageIndex(GenerationStage::Lots), true);
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 6: Buildings.
    // Places building sites on lots, then performs texture-driven feasibility filtering.
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
        if (options.constrain_roads_to_axiom_bounds) {
            cache_.buildings = ClipBuildingsToAxiomInfluence(cache_.buildings, resolved_axioms);
        }
        cache_.valid_stages.set(StageIndex(GenerationStage::Buildings), true);
    }

    if (global_state == nullptr || global_state->config.feature_city_boundary_hull) {
        cache_.city_boundary = BuildCityBoundary(resolved_axioms);
    } else {
        cache_.city_boundary.clear();
    }

    if (ShouldAbort(context)) {
        copy_cache_to_output();
        trim_output_for_partial_stages();
        output.plan_approved = false;
        return output;
    }

    // Stage 7: Validation.
    // Evaluates generated layout against terrain/policy constraints.
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
    trim_output_for_partial_stages();
    return output;
}

// Builds tensor field inputs from axioms and materializes the composite field.
TensorFieldGenerator CityGenerator::generateTensorField(
    const std::vector<AxiomInput>& axioms,
    GenerationContext* context) {
    // Configure the dimensions and cell size of the tensor field.
    TensorFieldGenerator::Config field_config;
    field_config.width = std::max(1, static_cast<int>(config_.width / config_.cell_size)); // Ensure width is at least 1.
    field_config.height = std::max(1, static_cast<int>(config_.height / config_.cell_size)); // Ensure height is at least 1.
    field_config.cell_size = config_.cell_size; // Set the size of each cell in the field.

    // Initialize the tensor field generator with the configuration.
    TensorFieldGenerator field(field_config);

    // Iterate over each axiom to add its influence to the tensor field.
    for (const auto& axiom : axioms) {
        // Check if the generation process should be aborted.
        if (ShouldAbort(context)) {
            break;
        }

        // Add the axiom's influence to the tensor field based on its type.
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
                break; // No action for COUNT type.
        }

        // Increment the iteration count in the context, if provided.
        if (context != nullptr) {
            context->BumpIterations();
        }
    }

    // Perform a final solve pass to bake all influences into the tensor field grid.
    if (!ShouldAbort(context)) {
        field.generateField();
    }

    // Return the generated tensor field.
    return field;
}

// Generates seed points for road tracing.
// Preference order:
// 1) optionally bias sampling inside axiom influence circles
// 2) reject samples blocked by texture/constraint data
// 3) fallback to deterministic center/axiom-position seeds if no valid sample was found
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
            // sqrt(u) yields uniform sampling over disk area instead of clustering at center.
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

        // First-pass reject against current material texture if available.
        if (texture_space != nullptr) {
            const Vec2 uv = texture_space->coordinateSystem().worldToUV(seed);
            const uint8_t material = texture_space->materialLayer().sampleNearest(uv);
            const bool blocked = Core::Data::DecodeMaterialNoBuild(material);
            const uint8_t flood = Core::Data::DecodeMaterialFloodMask(material);
            if (blocked || flood > 1u) {
                continue;
            }
        }

        // Secondary reject against authoritative constraint field if present.
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

    // Robust fallback: never return an empty seed set.
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

// Configures and executes road tracing from seed points.
// Constraint/profile data can tighten tracing policy for environmentally sensitive modes.
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

    // Baseline tracing defaults tuned for broad urban layouts.
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

    // Terrain-aware overrides constrain road growth into feasible cells only.
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

// Generates district polygons from the road graph, then enriches district metadata
// (type/desirability) from constraint-derived environmental signals.
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

    // Context-sensitive retagging and desirability scoring.
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
            // Historical tags can force semantic district categories.
            if (HasHistoryTag(tags, HistoryTag::SacredSite)) {
                district.type = DistrictType::Civic;
            } else if (HasHistoryTag(tags, HistoryTag::Brownfield) ||
                       HasHistoryTag(tags, HistoryTag::Contaminated)) {
                district.type = DistrictType::Industrial;
            }

            const float nature = constraints->sampleNatureScore(centroid);
            const float slope = constraints->sampleSlopeDegrees(centroid);
            const float flood_penalty = constraints->sampleFloodMask(centroid) > 0u ? 0.18f : 0.0f;
            // Simple normalized desirability blend used by downstream heuristics/UI.
            district.desirability = std::clamp(1.0f - (slope / 45.0f) + nature * 0.25f - flood_penalty, 0.0f, 1.0f);
        }
    }

    return districts;
}

// Converts district polygons into block polygons.
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

    // Generates lots from roads/districts/blocks and tunes lot sizing by site profile.
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

        // Environment-aware scaling can reduce lot density in constrained worlds.
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

    // Generates building sites from lot inventory with profile-specific caps.
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

    // Rasterizes district types into the texture-space zone layer.
    // Encoding uses "district type + 1" so zero remains reserved for "unassigned".
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

            // Iterate only over district AABB in pixel space, then apply exact polygon test.
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

    // Applies final texture-aware feasibility checks to generated buildings:
    // - slope/material buildability
    // - optional district-zone consistency with lot ownership
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

        // Keep candidates only if they pass physical and zoning consistency gates.
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

    // Ensures global texture-space exists and is synchronized with current world/constraint data.
    // When constraint data is available, height/material/slope layers are re-seeded from it.
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

            // Bake constraint field samples into texture layers pixel-by-pixel.
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

            // The upstream terrain layers are now authoritative; tensor remains dirty until regenerated.
            global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Height);
            global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Material);
            global_state->ClearTextureLayerDirty(Core::Data::TextureLayer::Distance);
            global_state->MarkTextureLayerDirty(Core::Data::TextureLayer::Tensor);
        }
    }
} // namespace RogueCity::Generators
