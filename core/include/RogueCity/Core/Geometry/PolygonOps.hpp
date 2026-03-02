#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <vector>

namespace RogueCity::Core::Geometry {

    struct Polygon {
        std::vector<Vec2> vertices;
    };

    struct PolygonRegion {
        Polygon outer;
        std::vector<Polygon> holes;
    };

    class PolygonOps {
    public:
        // Core operations taking world coordinates, converting to internal Clipper precision,
        // computing operations, and returning world coordinates.

        [[nodiscard]] static std::vector<Polygon> InsetPolygon(const Polygon& poly, double insetAmount);
        [[nodiscard]] static std::vector<Polygon> ClipPolygons(const Polygon& subject, const Polygon& clip);
        [[nodiscard]] static std::vector<Polygon> ClipPolygons(const Polygon& subject, const PolygonRegion& clipRegion);
        [[nodiscard]] static std::vector<PolygonRegion> ClipRegions(const PolygonRegion& subjectRegion, const PolygonRegion& clipRegion);
        [[nodiscard]] static std::vector<Polygon> DifferencePolygons(const Polygon& subject, const Polygon& clip);
        [[nodiscard]] static std::vector<Polygon> DifferencePolygons(const Polygon& subject, const PolygonRegion& clipRegion);
        [[nodiscard]] static std::vector<PolygonRegion> DifferenceRegions(const PolygonRegion& subjectRegion, const PolygonRegion& clipRegion);
        [[nodiscard]] static std::vector<Polygon> UnionPolygons(const std::vector<Polygon>& polygons);
        [[nodiscard]] static std::vector<PolygonRegion> UnionRegions(const std::vector<PolygonRegion>& regions);
        [[nodiscard]] static Polygon SimplifyPolygon(const Polygon& poly, double epsilon = 1e-6);
        [[nodiscard]] static PolygonRegion SimplifyRegion(const PolygonRegion& region, double epsilon = 1e-6);
        [[nodiscard]] static bool IsValidPolygon(const Polygon& poly, double epsilon = 1e-6);
        [[nodiscard]] static bool IsValidRegion(const PolygonRegion& region, double epsilon = 1e-6);
    };

} // namespace RogueCity::Core::Geometry
