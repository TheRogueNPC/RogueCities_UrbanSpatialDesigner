#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"

namespace RogueCity::Generators::Urban {

    class LotGenerator {
    public:
        struct Config {
            float min_lot_width{ 10.0f };
            float max_lot_width{ 50.0f };
            float min_lot_depth{ 15.0f };
            float max_lot_depth{ 60.0f };
            float min_lot_area{ 150.0f };
            float max_lot_area{ 3000.0f };
            uint32_t max_lots{ 50000 };
        };

        [[nodiscard]] static std::vector<Core::LotToken> generate(
            const fva::Container<Core::Road>& roads,
            const std::vector<Core::District>& districts,
            const std::vector<Core::BlockPolygon>& blocks,
            const Config& config,
            uint32_t seed);
    };

} // namespace RogueCity::Generators::Urban
