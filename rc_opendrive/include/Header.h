#pragma once

#include <string>
#include <vector>
#include <optional>

namespace rc_opendrive::core {

/**
 * @brief Defines the spatial reference system of the road network.
 * ASAM OpenDRIVE 1.7 - 2.2 Geo Reference
 * 
 * The content is typically a PROJ string.
 */
struct GeoReference {
    std::string content;
};

/**
 * @brief Defines an inertial offset for the road network.
 * ASAM OpenDRIVE 1.7 - 2.3 Offset
 * 
 * Moves the local coordinate system to a new position in the inertial system.
 */
struct Offset {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double hdg = 0.0; // Heading in radians
};

/**
 * @brief The header element is the very first element in the OpenDRIVE file.
 * ASAM OpenDRIVE 1.7 - 2.1 Header
 */
struct Header {
    int rev_major = 1;
    int rev_minor = 7;
    std::string name;
    std::string version;
    std::string date;
    
    // Bounding box of the road network
    double north = 0.0;
    double south = 0.0;
    double east = 0.0;
    double west = 0.0;
    
    std::string vendor;

    std::optional<GeoReference> geo_reference;
    std::optional<Offset> offset;
};

} // namespace rc_opendrive::core
