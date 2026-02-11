#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include <vector>

namespace RogueCity::Generators {

    using namespace Core;

    /// Classifies roads into hierarchy (Highway, Arterial, Street, etc.)
    /// based on connectivity and network topology
    class RoadClassifier {
    public:
        /// Classify roads based on network analysis
        static void classifyNetwork(fva::Container<Road>& roads);
        static void classifyGraph(Urban::Graph& graph, uint32_t centrality_samples = 64u);

        /// Classify single road based on length and connectivity
        static RoadType classifyRoad(const Road& road, double avg_length);

    private:
        static RoadType classifyScore(float score, float length_norm, float centrality, float endpoint_degree);
    };

} // namespace RogueCity::Generators
