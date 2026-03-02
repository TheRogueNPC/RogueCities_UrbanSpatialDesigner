#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

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

        // Road-cycle aware overload.
        // When config.prefer_road_cycles is true and the graph is non-empty,
        // delegates to PolygonFinder::fromGraph which filters/validates blocks
        // against the road network.  Falls back to fromDistricts when graph is empty.
        [[nodiscard]] static std::vector<Core::BlockPolygon> generate(
            const std::vector<Core::District>& districts,
            const Graph& road_graph,
            const Config& config);
    };

} // namespace RogueCity::Generators::Urban
