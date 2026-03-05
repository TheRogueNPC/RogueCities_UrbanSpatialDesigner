#pragma once

#include <string>
#include <vector>
#include <variant>

namespace rc_opendrive::core {

/**
 * @brief Represents a straight line in the reference line.
 * ASAM OpenDRIVE 1.7 - 8.4.1 Line
 */
struct Line {
    // Line has no additional attributes
};

/**
 * @brief Represents a spiral (clothoid) in the reference line.
 * ASAM OpenDRIVE 1.7 - 8.4.2 Spiral
 */
struct Spiral {
    double curvStart = 0.0;
    double curvEnd = 0.0;
};

/**
 * @brief Represents an arc (constant curvature) in the reference line.
 * ASAM OpenDRIVE 1.7 - 8.4.3 Arc
 */
struct Arc {
    double curvature = 0.0;
};

/**
 * @brief Represents a cubic polynomial in the reference line.
 * ASAM OpenDRIVE 1.7 - 8.4.4 Poly3
 * Note: Deprecated in OpenDRIVE 1.6, but retained for compatibility.
 */
struct Poly3 {
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
};

/**
 * @brief Represents a parametric cubic polynomial in the reference line.
 * ASAM OpenDRIVE 1.7 - 8.4.5 ParamPoly3
 */
struct ParamPoly3 {
    enum class PRange {
        ArcLength,
        Normalized
    };

    double aU = 0.0;
    double bU = 0.0;
    double cU = 0.0;
    double dU = 0.0;
    double aV = 0.0;
    double bV = 0.0;
    double cV = 0.0;
    double dV = 0.0;
    PRange pRange = PRange::Normalized;
};

/**
 * @brief Defines the geometry of the reference line.
 * ASAM OpenDRIVE 1.7 - 8.2 Geometry
 * 
 * The geometry element defines the layout of the road reference line in the
 * inertial coordinate system (x/y).
 */
struct Geometry {
    double s = 0.0;
    double x = 0.0;
    double y = 0.0;
    double hdg = 0.0;
    double length = 0.0;

    // The specific geometry definition
    std::variant<Line, Spiral, Arc, Poly3, ParamPoly3> type;
};

/**
 * @brief Contains the geometry definition of the reference line.
 * ASAM OpenDRIVE 1.7 - 8.1 Plan View
 */
struct PlanView {
    std::vector<Geometry> geometries;
};

} // namespace rc_opendrive::core