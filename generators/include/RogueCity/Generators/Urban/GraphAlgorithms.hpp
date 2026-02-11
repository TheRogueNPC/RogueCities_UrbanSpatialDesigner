#pragma once

#include "RogueCity/Generators/Urban/Graph.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace RogueCity::Generators::Urban {

    struct PathResult {
        std::vector<VertexID> vertices;
        double cost = std::numeric_limits<double>::infinity();

        [[nodiscard]] bool reachable() const {
            return !vertices.empty() && std::isfinite(cost);
        }
    };

    class GraphAlgorithms {
    public:
        [[nodiscard]] static PathResult shortestPath(
            const Graph& g,
            VertexID src,
            VertexID dst);

        [[nodiscard]] static PathResult simplestPath(
            const Graph& g,
            VertexID src,
            VertexID dst,
            double turn_penalty);

        [[nodiscard]] static std::vector<float> sampledEdgeCentrality(
            const Graph& g,
            size_t sample_count,
            uint32_t seed = 1u);
    };

} // namespace RogueCity::Generators::Urban
