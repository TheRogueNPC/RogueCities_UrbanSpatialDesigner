#pragma once

#include "RogueCity/Core/Types.hpp"

namespace RogueCity::Generators::Urban {

    struct RoadFrontageProfile {
        double access{ 0.0 };
        double exposure{ 0.0 };
        double serviceability{ 0.0 };
        double privacy{ 0.0 };
    };

    class FrontageProfiles {
    public:
        static RoadFrontageProfile get(Core::RoadType type);
    };

} // namespace RogueCity::Generators::Urban

