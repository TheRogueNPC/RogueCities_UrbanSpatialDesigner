#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/CitySpec.hpp"

#include <optional>

namespace RogueCity::Generators {

/// Builds world constraint rasters and site diagnostics before road tracing.
class TerrainConstraintGenerator {
public:
    struct Config {
        float max_buildable_slope_deg{ 24.0f };
        float hostile_terrain_slope_deg{ 32.0f };
        float min_buildable_fraction{ 0.30f };
        float fragmentation_threshold{ 0.40f };
        float policy_friction_threshold{ 0.55f };
        int erosion_iterations{ 3 };
    };

    struct Input {
        int world_width{ 2000 };
        int world_height{ 2000 };
        double cell_size{ 10.0 };
        uint32_t seed{ 1 };
        std::optional<Core::CitySpec> city_spec{};
    };

    struct Output {
        Core::WorldConstraintField constraints;
        Core::SiteProfile profile;
    };

    [[nodiscard]] Output generate(const Input& input) const;
    [[nodiscard]] Output generate(const Input& input, const Config& config) const;
};

} // namespace RogueCity::Generators
