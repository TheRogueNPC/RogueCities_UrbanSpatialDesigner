#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class BlockGenerator {
    public:
        struct Config {
            bool prefer_road_cycles;
        };

        [[nodiscard]] static std::vector<Core::BlockPolygon> generate(
            const std::vector<Core::District>& districts);

        [[nodiscard]] static std::vector<Core::BlockPolygon> generate(
            const std::vector<Core::District>& districts,
            const Config& config);
    };

} // namespace RogueCity::Generators::Urban
