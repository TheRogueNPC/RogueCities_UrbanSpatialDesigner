#pragma once

#include <string>
#include <vector>
#include <optional>

#include "RoadGeometry.h"
#include "RoadProfile.h"
#include "RoadLane.h"

namespace rc_opendrive::core {

/**
 * @brief Reference to a linked element (road or junction).
 * ASAM OpenDRIVE 1.7 - 6. Road Linkage
 */
struct RoadLinkInfo {
    std::string elementType; // "road" or "junction"
    std::string elementId;
    std::string contactPoint; // "start" or "end"
};

/**
 * @brief Linkage for the road itself.
 * ASAM OpenDRIVE 1.7 - 6. Road Linkage
 */
struct RoadLink {
    std::optional<RoadLinkInfo> predecessor;
    std::optional<RoadLinkInfo> successor;
};

/**
 * @brief Defines the main usage type of the road (e.g., motorway).
 * ASAM OpenDRIVE 1.7 - 5. Road Type
 */
struct RoadType {
    double s = 0.0;
    std::string type;
    std::string country;
};

/**
 * @brief The top-level Road element.
 * ASAM OpenDRIVE 1.7 - 9. Road
 * 
 * Aggregates all properties of a single road segment.
 */
struct Road {
    std::string name;
    double length = 0.0;
    std::string id;
    std::string junction; // ID of the junction this road belongs to, or "-1"
    
    RoadLink link;
    std::vector<RoadType> types;
    
    // Geometry (Plan View)
    PlanView planView;
    
    // Vertical and Lateral Profiles
    ElevationProfile elevationProfile;
    LateralProfile lateralProfile;
    
    // Lanes
    Lanes lanes;
};

} // namespace rc_opendrive::core