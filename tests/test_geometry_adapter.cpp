#include "RogueCity/Generators/Geometry/GeometryAdapter.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using RogueCity::Core::Bounds;
using RogueCity::Core::Vec2;
using RogueCity::Generators::Geometry::GeometryAdapter;

int main() {
    std::cout << "Geometry backend: " << GeometryAdapter::backendName() << "\n";

    const double d = GeometryAdapter::distance(Vec2(0.0, 0.0), Vec2(3.0, 4.0));
    assert(std::abs(d - 5.0) < 1e-9);

    const Bounds aabb_a{ Vec2(0.0, 0.0), Vec2(10.0, 10.0) };
    const Bounds aabb_b{ Vec2(9.0, 9.0), Vec2(20.0, 20.0) };
    const Bounds aabb_c{ Vec2(11.0, 11.0), Vec2(20.0, 20.0) };

    assert(GeometryAdapter::intersects(aabb_a, aabb_b));
    assert(!GeometryAdapter::intersects(aabb_a, aabb_c));

    const GeometryAdapter::Ring square_a{
        Vec2(0.0, 0.0), Vec2(10.0, 0.0), Vec2(10.0, 10.0), Vec2(0.0, 10.0)
    };
    const GeometryAdapter::Ring square_b{
        Vec2(8.0, 8.0), Vec2(12.0, 8.0), Vec2(12.0, 12.0), Vec2(8.0, 12.0)
    };
    const GeometryAdapter::Ring square_c{
        Vec2(20.0, 20.0), Vec2(22.0, 20.0), Vec2(22.0, 22.0), Vec2(20.0, 22.0)
    };

    assert(GeometryAdapter::intersects(square_a, square_b));
    assert(!GeometryAdapter::intersects(square_a, square_c));

    const GeometryAdapter::Ring cloud{
        Vec2(0.0, 0.0), Vec2(1.0, 0.0), Vec2(2.0, 0.0),
        Vec2(2.0, 2.0), Vec2(1.0, 1.0), Vec2(0.0, 2.0)
    };
    const auto hull = GeometryAdapter::convexHull(cloud);
    assert(hull.size() >= 4);

    const GeometryAdapter::Ring dense_polyline{
        Vec2(0.0, 0.0), Vec2(1.0, 0.1), Vec2(2.0, -0.1), Vec2(3.0, 0.05), Vec2(4.0, 0.0)
    };
    const auto simplified = GeometryAdapter::simplify(dense_polyline, 0.15);
    assert(!simplified.empty());
    assert(simplified.size() <= dense_polyline.size());

    std::cout << "Geometry adapter tests PASSED\n";
    return 0;
}
