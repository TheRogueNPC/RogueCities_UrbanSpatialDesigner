#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <vector>

namespace RogueCity::Core::Geometry {

    struct Polygon {
        std::vector<Vec2> vertices;
    };

    class PolygonOps {
    public:
        // Core operations taking world coordinates, converting to internal Clipper precision,
        // computing operations, and returning world coordinates.

        [[nodiscard]] static std::vector<Polygon> InsetPolygon(const Polygon& poly, double insetAmount);
        [[nodiscard]] static std::vector<Polygon> ClipPolygons(const Polygon& subject, const Polygon& clip);
    };

} // namespace RogueCity::Core::Geometry
