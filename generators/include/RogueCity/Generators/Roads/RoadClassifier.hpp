#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include <vector>

namespace RogueCity::Generators {

    using namespace Core;

    /// Classifies roads into hierarchy (Highway, Arterial, Street, etc.)
    /// based on connectivity and network topology
    class RoadClassifier {
    public:
        /// Classify roads based on network analysis
        static void classifyNetwork(fva::Container<Road>& roads);

        /// Classify single road based on length and connectivity
        static RoadType classifyRoad(const Road& road, double avg_length);

    private:
        // Helper methods for Phase 3
    };

} // namespace RogueCity::Generators
