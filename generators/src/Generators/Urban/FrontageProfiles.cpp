#include "RogueCity/Generators/Urban/FrontageProfiles.hpp"

namespace RogueCity::Generators::Urban {

    RoadFrontageProfile FrontageProfiles::get(Core::RoadType type) {
        using Core::RoadType;
        switch (type) {
            case RoadType::Highway: return { 1.00, 1.00, 0.70, 0.00 };
            case RoadType::Arterial: return { 0.90, 0.90, 0.90, 0.20 };
            case RoadType::Avenue: return { 0.80, 0.80, 0.80, 0.50 };
            case RoadType::Boulevard: return { 0.70, 0.90, 0.50, 0.70 };
            case RoadType::Street: return { 0.80, 0.50, 0.80, 0.80 };
            case RoadType::Lane: return { 0.50, 0.20, 0.50, 1.00 };
            case RoadType::Alleyway: return { 0.30, 0.10, 1.00, 0.70 };
            case RoadType::CulDeSac: return { 0.30, 0.20, 0.50, 1.00 };
            case RoadType::Drive: return { 0.50, 0.30, 0.60, 0.90 };
            case RoadType::Driveway: return { 0.20, 0.05, 0.70, 1.00 };
            case RoadType::M_Major: return { 0.90, 0.90, 0.90, 0.20 };
            case RoadType::M_Minor: return { 0.80, 0.50, 0.80, 0.80 };
            default: return { 0.50, 0.50, 0.50, 0.50 };
        }
    }

} // namespace RogueCity::Generators::Urban

