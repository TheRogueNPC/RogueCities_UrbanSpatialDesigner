#pragma once

// Include necessary headers for Urban Graph generation within the RogueCity namespace.
#include "RogueCity/Generators/Urban/Graph.hpp"

namespace RogueCity::Generators::Roads {

    // Define a struct to hold configuration settings for simplifying the road graph.
    struct SimplifyConfig {
        // Weld radius used to merge close edges; default is 5.0f units.
        float weld_radius = 5.0f;
        // Minimum length of an edge that should not be collapsed; default is 20.0f units.
        float min_edge_length = 20.0f;
        // Angle in degrees used to determine if edges can be collapsed without significant change; default is 10.0f degrees.
        float collapse_angle_deg = 10.0f;
    };

    // Function to simplify the given road graph using specified configuration settings.
    void simplifyGraph(Urban::Graph& g, const SimplifyConfig& cfg);

} // namespace RogueCity::Generators::Roads
