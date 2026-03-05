#pragma once

namespace rc_opendrive::core {

/**
 * @brief Represents a position in the Inertial Coordinate System.
 * ASAM OpenDRIVE 1.7 - 3.1.1 Inertial Coordinate System
 * 
 * A right-handed coordinate system according to ISO 8855.
 * x: East
 * y: North
 * z: Up
 */
struct InertialPos {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/**
 * @brief Represents a position in the Reference Line Coordinate System.
 * ASAM OpenDRIVE 1.7 - 3.1.2 Reference Line Coordinate System
 * 
 * s: Coordinate along the reference line (measured in meters from start)
 * t: Lateral coordinate (positive to the left)
 * h: Vertical coordinate (positive up, orthogonal to st-plane)
 */
struct TrackCoord {
    double s = 0.0;
    double t = 0.0;
    double h = 0.0;
};

/**
 * @brief Represents orientation in Euler angles (s-t-h system).
 * ASAM OpenDRIVE 1.7 - 3.1.1 Inertial Coordinate System
 */
struct Orientation {
    double h = 0.0; // Heading (yaw)
    double p = 0.0; // Pitch
    double r = 0.0; // Roll
};

} // namespace rc_opendrive::core