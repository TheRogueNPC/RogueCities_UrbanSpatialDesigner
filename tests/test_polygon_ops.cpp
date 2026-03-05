#include "RogueCity/Core/Geometry/PolygonOps.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

namespace {

using RogueCity::Core::Geometry::Polygon;
using RogueCity::Core::Geometry::PolygonRegion;
using RogueCity::Core::Geometry::PolygonOps;
using RogueCity::Core::Vec2;

double SignedArea(const Polygon& polygon) {
    if (polygon.vertices.size() < 3u) {
        return 0.0;
    }
    double area = 0.0;
    for (size_t i = 0; i < polygon.vertices.size(); ++i) {
        const Vec2& a = polygon.vertices[i];
        const Vec2& b = polygon.vertices[(i + 1u) % polygon.vertices.size()];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area * 0.5;
}

double TotalArea(const std::vector<Polygon>& polygons) {
    double area = 0.0;
    for (const auto& polygon : polygons) {
        area += SignedArea(polygon);
    }
    return std::abs(area);
}

double RegionArea(const PolygonRegion& region) {
    double area = std::abs(SignedArea(region.outer));
    for (const auto& hole : region.holes) {
        area -= std::abs(SignedArea(hole));
    }
    return std::max(0.0, area);
}

double TotalRegionArea(const std::vector<PolygonRegion>& regions) {
    double sum = 0.0;
    for (const auto& region : regions) {
        sum += RegionArea(region);
    }
    return sum;
}

bool NearlyEqual(double a, double b, double epsilon = 1e-3) {
    return std::abs(a - b) <= epsilon;
}

bool IsFinitePolygon(const Polygon& polygon) {
    for (const auto& vertex : polygon.vertices) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y)) {
            return false;
        }
    }
    return true;
}

Polygon MakeSquare(double min_x, double min_y, double max_x, double max_y) {
    Polygon square{};
    square.vertices = {
        Vec2(min_x, min_y),
        Vec2(max_x, min_y),
        Vec2(max_x, max_y),
        Vec2(min_x, max_y)
    };
    return square;
}

} // namespace

#define RC_EXPECT(expr)                                                       \
    do {                                                                      \
        if (!(expr)) {                                                        \
            std::cerr << "RC_EXPECT failed: " << #expr                        \
                      << " (line " << __LINE__ << ")" << std::endl;          \
            return 1;                                                         \
        }                                                                     \
    } while (false)

int main() {
    {
        Polygon noisy{};
        noisy.vertices = {
            Vec2(0.0, 0.0),
            Vec2(10.0, 0.0),
            Vec2(10.0, 0.0), // duplicate
            Vec2(10.0, 5.0),
            Vec2(10.0, 10.0), // collinear
            Vec2(0.0, 10.0),
            Vec2(0.0, 0.0) // closed duplicate
        };
        const Polygon simplified = PolygonOps::SimplifyPolygon(noisy);
        RC_EXPECT(PolygonOps::IsValidPolygon(simplified));
        RC_EXPECT(simplified.vertices.size() == 4u);
    }

    {
        const Polygon base = MakeSquare(0.0, 0.0, 10.0, 10.0);
        const auto inset = PolygonOps::InsetPolygon(base, 1.0);
        RC_EXPECT(!inset.empty());
        RC_EXPECT(NearlyEqual(TotalArea(inset), 64.0));
    }

    {
        const Polygon a = MakeSquare(0.0, 0.0, 10.0, 10.0);
        const Polygon b = MakeSquare(5.0, 5.0, 15.0, 15.0);

        const auto clipped = PolygonOps::ClipPolygons(a, b);
        RC_EXPECT(!clipped.empty());
        RC_EXPECT(NearlyEqual(TotalArea(clipped), 25.0));

        const auto diff = PolygonOps::DifferencePolygons(a, b);
        RC_EXPECT(!diff.empty());
        RC_EXPECT(NearlyEqual(TotalArea(diff), 75.0));

        const auto unified = PolygonOps::UnionPolygons({ a, b });
        RC_EXPECT(!unified.empty());
        RC_EXPECT(NearlyEqual(TotalArea(unified), 175.0));
    }

    {
        const Polygon outer = MakeSquare(0.0, 0.0, 100.0, 100.0);
        const Polygon hole = MakeSquare(30.0, 30.0, 70.0, 70.0);
        const auto ring = PolygonOps::DifferencePolygons(outer, hole);
        RC_EXPECT(!ring.empty());
        RC_EXPECT(NearlyEqual(TotalArea(ring), 8400.0, 1.0));
    }

    {
        PolygonRegion region{};
        region.outer = MakeSquare(0.0, 0.0, 100.0, 100.0);
        region.holes.push_back(MakeSquare(30.0, 30.0, 70.0, 70.0));
        RC_EXPECT(PolygonOps::IsValidRegion(region));

        const Polygon subject = MakeSquare(20.0, 20.0, 80.0, 80.0);
        PolygonRegion subject_region{};
        subject_region.outer = subject;

        const auto clipped_regions = PolygonOps::ClipRegions(subject_region, region);
        RC_EXPECT(!clipped_regions.empty());
        RC_EXPECT(clipped_regions.size() == 1u);
        RC_EXPECT(clipped_regions.front().holes.size() == 1u);
        RC_EXPECT(NearlyEqual(TotalRegionArea(clipped_regions), 2000.0, 1.0));

        const auto clipped = PolygonOps::ClipPolygons(subject, region);
        RC_EXPECT(!clipped.empty());
        RC_EXPECT(NearlyEqual(TotalArea(clipped), 2000.0, 1.0));

        const auto diff_regions = PolygonOps::DifferenceRegions(subject_region, region);
        RC_EXPECT(!diff_regions.empty());
        RC_EXPECT(diff_regions.size() == 1u);
        RC_EXPECT(diff_regions.front().holes.empty());
        RC_EXPECT(NearlyEqual(TotalRegionArea(diff_regions), 1600.0, 1.0));

        const auto diff = PolygonOps::DifferencePolygons(subject, region);
        RC_EXPECT(!diff.empty());
        RC_EXPECT(NearlyEqual(TotalArea(diff), 1600.0, 1.0));

        const auto union_regions = PolygonOps::UnionRegions({subject_region, region});
        RC_EXPECT(!union_regions.empty());
        RC_EXPECT(union_regions.size() == 1u);
        RC_EXPECT(union_regions.front().holes.empty());
        RC_EXPECT(NearlyEqual(TotalRegionArea(union_regions), 10000.0, 1.0));
    }

    {
        Polygon invalid{};
        invalid.vertices = {
            Vec2(0.0, 0.0),
            Vec2(1.0, 0.0),
            Vec2(2.0, 0.0)
        };
        RC_EXPECT(!PolygonOps::IsValidPolygon(invalid));

        Polygon non_finite{};
        non_finite.vertices = {
            Vec2(0.0, 0.0),
            Vec2(std::numeric_limits<double>::quiet_NaN(), 1.0),
            Vec2(1.0, 0.0)
        };
        RC_EXPECT(!PolygonOps::IsValidPolygon(non_finite));

        PolygonRegion invalid_region{};
        invalid_region.outer = MakeSquare(0.0, 0.0, 100.0, 100.0);
        invalid_region.holes.push_back(MakeSquare(95.0, 95.0, 120.0, 120.0));
        RC_EXPECT(!PolygonOps::IsValidRegion(invalid_region));
    }

    {
        const Polygon poly = MakeSquare(0.0, 0.0, 10.0, 10.0);
        RC_EXPECT(PolygonOps::ContainsPoint(poly, Vec2(5.0, 5.0)));
        RC_EXPECT(PolygonOps::ContainsPoint(poly, Vec2(0.1, 0.1)));
        RC_EXPECT(!PolygonOps::ContainsPoint(poly, Vec2(11.0, 5.0)));
        RC_EXPECT(!PolygonOps::ContainsPoint(poly, Vec2(-1.0, -1.0)));

        PolygonRegion region{};
        region.outer = MakeSquare(0.0, 0.0, 10.0, 10.0);
        region.holes.push_back(MakeSquare(2.0, 2.0, 8.0, 8.0));

        RC_EXPECT(PolygonOps::ContainsPoint(region, Vec2(1.0, 1.0)));
        RC_EXPECT(PolygonOps::ContainsPoint(region, Vec2(9.0, 9.0)));
        RC_EXPECT(!PolygonOps::ContainsPoint(region, Vec2(5.0, 5.0))); // In hole
        RC_EXPECT(!PolygonOps::ContainsPoint(region, Vec2(11.0, 11.0))); // Outside
    }

    {
        const Polygon base = MakeSquare(0.0, 0.0, 100.0, 100.0);
        constexpr double kBaseArea = 10000.0;

        std::mt19937 rng(0x5a17u);
        std::uniform_real_distribution<double> min_dist(-30.0, 90.0);
        std::uniform_real_distribution<double> size_dist(1.0, 80.0);

        for (int i = 0; i < 200; ++i) {
            const double min_x = min_dist(rng);
            const double min_y = min_dist(rng);
            const double width = size_dist(rng);
            const double height = size_dist(rng);
            const Polygon clip = MakeSquare(min_x, min_y, min_x + width, min_y + height);

            const auto inter = PolygonOps::ClipPolygons(base, clip);
            const auto diff = PolygonOps::DifferencePolygons(base, clip);
            const auto uni = PolygonOps::UnionPolygons({ base, clip });

            for (const auto& polygon : inter) {
                RC_EXPECT(PolygonOps::IsValidPolygon(polygon));
                RC_EXPECT(IsFinitePolygon(polygon));
            }
            for (const auto& polygon : diff) {
                RC_EXPECT(PolygonOps::IsValidPolygon(polygon));
                RC_EXPECT(IsFinitePolygon(polygon));
            }
            for (const auto& polygon : uni) {
                RC_EXPECT(PolygonOps::IsValidPolygon(polygon));
                RC_EXPECT(IsFinitePolygon(polygon));
            }

            const double inter_area = TotalArea(inter);
            const double diff_area = TotalArea(diff);
            const double union_area = TotalArea(uni);
            RC_EXPECT(NearlyEqual(inter_area + diff_area, kBaseArea, 2.0));

            const double expected_union = kBaseArea + (width * height) - inter_area;
            RC_EXPECT(NearlyEqual(union_area, expected_union, 2.0));
        }
    }

    return 0;
}

#undef RC_EXPECT
