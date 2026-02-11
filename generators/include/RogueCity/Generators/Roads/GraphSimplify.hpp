#pragma once

#include "RogueCity/Generators/Urban/Graph.hpp"

namespace RogueCity::Generators::Roads {

    struct SimplifyConfig {
        float weld_radius = 5.0f;
        float min_edge_length = 20.0f;
        float collapse_angle_deg = 10.0f;
    };

    void simplifyGraph(Urban::Graph& g, const SimplifyConfig& cfg);

} // namespace RogueCity::Generators::Roads
