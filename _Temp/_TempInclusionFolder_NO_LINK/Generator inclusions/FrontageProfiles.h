#pragma once

#include "CityModel.h"

namespace CityModel
{

    struct RoadFrontageProfile
    {
        double access{0.0};
        double exposure{0.0};
        double serviceability{0.0};
        double privacy{0.0};
    };

    struct FrontageProfile
    {
        float A{0.0f};
        float E{0.0f};
        float S{0.0f};
        float P{0.0f};
    };

    class FrontageProfiles
    {
    public:
        static RoadFrontageProfile get(RoadType type);
    };

    FrontageProfile get_frontage(RoadType type);

} // namespace CityModel
