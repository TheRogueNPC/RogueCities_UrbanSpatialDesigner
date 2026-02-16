#pragma once

#include "RogueCity/Core/Types.hpp"

namespace RogueCity::Generators {

class RogueProfiler {
public:
    struct Scores {
        float access{ 0.5f };
        float exposure{ 0.5f };
        float serviceability{ 0.5f };
        float privacy{ 0.5f };
    };

    [[nodiscard]] static Scores computeScores(Core::RoadType primary, Core::RoadType secondary);
    [[nodiscard]] static Core::DistrictType classifyDistrict(const Scores& scores);
    [[nodiscard]] static Core::DistrictType classifyLot(const Core::LotToken& lot);

    [[nodiscard]] static float roadTypeToAccess(Core::RoadType type);
    [[nodiscard]] static float roadTypeToExposure(Core::RoadType type);
    [[nodiscard]] static float roadTypeToServiceability(Core::RoadType type);
    [[nodiscard]] static float roadTypeToPrivacy(Core::RoadType type);
};

} // namespace RogueCity::Generators
