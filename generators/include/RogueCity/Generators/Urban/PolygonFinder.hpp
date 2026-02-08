#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class PolygonFinder {
    public:
        [[nodiscard]] static std::vector<Core::BlockPolygon> fromDistricts(
            const std::vector<Core::District>& districts);

        [[nodiscard]] static std::vector<Core::BlockPolygon> fromGraph(
            const Graph& graph,
            const std::vector<Core::District>& districts);
    };

} // namespace RogueCity::Generators::Urban

