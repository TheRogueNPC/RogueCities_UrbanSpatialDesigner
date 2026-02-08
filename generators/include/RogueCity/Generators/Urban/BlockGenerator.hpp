#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class BlockGenerator {
    public:
        struct Config {
            bool prefer_road_cycles{ false };
        };

        [[nodiscard]] static std::vector<Core::BlockPolygon> generate(
            const std::vector<Core::District>& districts,
            const Config& config = Config{});
    };

} // namespace RogueCity::Generators::Urban

