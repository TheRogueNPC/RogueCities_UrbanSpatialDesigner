#pragma once

#include <vector>
#include <string>
#include <optional>

namespace rc_opendrive::core {

/**
 * @brief Defines a lane offset from the reference line.
 * ASAM OpenDRIVE 1.7 - 10.3 Lane Offset
 * 
 * Shifts the lane reference line laterally.
 * cubic polynomial: offset(ds) = a + b*ds + c*ds^2 + d*ds^3
 */
struct LaneOffset {
    double s = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
};

/**
 * @brief Defines the width of a lane.
 * ASAM OpenDRIVE 1.7 - 10.5 Lane Width
 * 
 * width(ds) = a + b*ds + c*ds^2 + d*ds^3
 */
struct LaneWidth {
    double sOffset = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
};

/**
 * @brief Defines the height of a lane offset from the road reference plane.
 * ASAM OpenDRIVE 1.7 - 10.6 Lane Height
 * 
 * Allows modeling of superelevation or banking per lane.
 */
struct LaneHeight {
    double sOffset = 0.0;
    double inner = 0.0;
    double outer = 0.0;
};

/**
 * @brief Defines a lane border (independent of width).
 * ASAM OpenDRIVE 1.7 - 10.4 Lane Border
 */
struct LaneBorder {
    double sOffset = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
};

/**
 * @brief Linkage between lanes (predecessor/successor).
 * Part of 10.1 Lane definition.
 */
struct LaneLink {
    std::optional<int> predecessorId;
    std::optional<int> successorId;
};

/**
 * @brief Represents a single lane.
 * ASAM OpenDRIVE 1.7 - 10.1 Lanes
 */
struct Lane {
    int id = 0;
    std::string type; // e.g., "driving", "sidewalk", "shoulder"
    bool level = false; // If true, keep level (no superelevation)
    
    LaneLink link;
    std::vector<LaneWidth> widths;
    std::vector<LaneHeight> heights;
    std::vector<LaneBorder> borders;
};

/**
 * @brief A section of the road with a constant number of lanes.
 * ASAM OpenDRIVE 1.7 - 10.2 Lane Sections
 */
struct LaneSection {
    double s = 0.0;
    bool singleSide = false;
    std::vector<Lane> left;
    std::vector<Lane> center;
    std::vector<Lane> right;
};

/**
 * @brief Container for all lane definitions in a road.
 */
struct Lanes {
    std::vector<LaneOffset> laneOffsets;
    std::vector<LaneSection> laneSections;
};

} // namespace rc_opendrive::core