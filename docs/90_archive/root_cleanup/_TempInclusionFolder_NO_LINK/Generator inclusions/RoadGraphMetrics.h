#pragma once

#include <vector>

#include "Graph.h"

namespace RoadGraphMetrics
{
    struct CommunityResult
    {
        std::vector<int> node_community;
        std::vector<int> node_degree;
        int community_count{0};
    };

    std::vector<int> compute_degrees(const Graph &graph);
    CommunityResult compute_components(const Graph &graph);
} // namespace RoadGraphMetrics

