#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"

#include <optional>

namespace RogueCity::Generators {

/// Builds world constraint rasters and site diagnostics before road tracing.
class TerrainConstraintGenerator {
public:
    /// Configuration structure for the terrain constraint generator.
    struct Config {
        /// Maximum buildable slope in degrees.
        float max_buildable_slope_deg{ 24.0f };
        /// Hostile terrain slope in degrees, beyond which areas are considered unsuitable for building.
        float hostile_terrain_slope_deg{ 32.0f };
        /// Minimum fraction of the world that must be buildable to allow a site.
        float min_buildable_fraction{ 0.30f };
        /// Fragmentation threshold, used in determining if the terrain is too fragmented.
        float fragmentation_threshold{ 0.40f };
        /// Policy friction threshold, used in assessing the suitability of areas based on policy constraints.
        float policy_friction_threshold{ 0.55f };
        /// Number of erosion iterations to perform.
        int erosion_iterations{ 3 };
    };

    /// Input structure for the terrain constraint generator.
    struct Input {
        /// Width of the world grid in cells.
        int world_width{ 2000 };
        /// Height of the world grid in cells.
        int world_height{ 2000 };
        /// Size of each cell in meters.
        double cell_size{ 10.0 };
        /// Random seed for reproducibility.
        uint32_t seed{ 1 };
        /// Optional city specification, used to provide additional constraints or preferences during generation.
        std::optional<Core::CitySpec> city_spec{};
    };

    /// Output structure for the terrain constraint generator.
    struct Output {
        /// World constraint field, containing information about buildable areas and constraints.
        Core::WorldConstraintField constraints;
        /// Site profile data, useful for detailed analysis of generated sites.
        Core::SiteProfile profile;
    };

    // Generate world constraints and site diagnostics using default configuration.
    [[nodiscard]] Output generate(const Input& input) const;

    // Generate world constraints and site diagnostics with a custom configuration.
    [[nodiscard]] Output generate(const Input& input, const Config& config) const;
};

} // namespace RogueCity::Generators
