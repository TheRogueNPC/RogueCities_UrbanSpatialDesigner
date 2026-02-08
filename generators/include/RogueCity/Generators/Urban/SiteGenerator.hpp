#pragma once

#include "RogueCity/Core/Types.hpp"

namespace RogueCity::Generators::Urban {

    class SiteGenerator {
    public:
        struct Config {
            uint32_t max_buildings{ 100000 };
            bool randomize_sites{ false };
            float jitter{ 0.35f };
        };

        [[nodiscard]] static siv::Vector<Core::BuildingSite> generate(
            const std::vector<Core::LotToken>& lots,
            const Config& config,
            uint32_t seed);
    };

} // namespace RogueCity::Generators::Urban

