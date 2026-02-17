#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"

namespace RogueCity::Generators::Urban {

    class DistrictGenerator {
    public:
        struct Config {
            int grid_resolution{ 6 };
            double road_influence_radius{ 350.0 };
            uint32_t max_districts{ 256 };
        };

        [[nodiscard]] static std::vector<Core::District> generate(
            const fva::Container<Core::Road>& roads,
            const Core::Bounds& bounds);

        [[nodiscard]] static std::vector<Core::District> generate(
            const fva::Container<Core::Road>& roads,
            const Core::Bounds& bounds,
            const Config& config);
    };

} // namespace RogueCity::Generators::Urban
