#include "RogueCity/Generators/Geometry/GeometryAdapter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#endif

namespace RogueCity::Generators::Geometry {

namespace {

using Ring = GeometryAdapter::Ring;

[[nodiscard]] Ring normalizeRing(const Ring& ring) {
    if (ring.size() < 3) {
        return {};
    }
    Ring out = ring;
    if (out.front().equals(out.back())) {
        out.pop_back();
    }
    if (out.size() < 3) {
        return {};
    }
    return out;
}

[[nodiscard]] Core::Bounds ringBounds(const Ring& ring) {
    Core::Bounds bounds{};
    if (ring.empty()) {
        return bounds;
    }

    bounds.min = ring.front();
    bounds.max = ring.front();
    for (const auto& p : ring) {
        bounds.min.x = std::min(bounds.min.x, p.x);
        bounds.min.y = std::min(bounds.min.y, p.y);
        bounds.max.x = std::max(bounds.max.x, p.x);
        bounds.max.y = std::max(bounds.max.y, p.y);
    }
    return bounds;
}

[[nodiscard]] bool pointInsideRing(const Core::Vec2& p, const Ring& ring) {
    if (ring.size() < 3) {
        return false;
    }
    bool inside = false;
    for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const auto& pi = ring[i];
        const auto& pj = ring[j];
        const bool hit = ((pi.y > p.y) != (pj.y > p.y)) &&
            (p.x < (pj.x - pi.x) * (p.y - pi.y) / ((pj.y - pi.y) + 1e-12) + pi.x);
        if (hit) {
            inside = !inside;
        }
    }
    return inside;
}

[[nodiscard]] double orient2d(const Core::Vec2& a, const Core::Vec2& b, const Core::Vec2& c) {
    return (b - a).cross(c - a);
}

[[nodiscard]] bool onSegment(const Core::Vec2& a, const Core::Vec2& b, const Core::Vec2& p) {
    constexpr double eps = 1e-9;
    if (std::abs(orient2d(a, b, p)) > eps) {
        return false;
    }
    const double min_x = std::min(a.x, b.x) - eps;
    const double max_x = std::max(a.x, b.x) + eps;
    const double min_y = std::min(a.y, b.y) - eps;
    const double max_y = std::max(a.y, b.y) + eps;
    return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
}

[[nodiscard]] bool segmentsIntersect(
    const Core::Vec2& a0,
    const Core::Vec2& a1,
    const Core::Vec2& b0,
    const Core::Vec2& b1) {
    constexpr double eps = 1e-9;

    const double o1 = orient2d(a0, a1, b0);
    const double o2 = orient2d(a0, a1, b1);
    const double o3 = orient2d(b0, b1, a0);
    const double o4 = orient2d(b0, b1, a1);

    if (((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps)) &&
        ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps))) {
        return true;
    }

    return onSegment(a0, a1, b0) ||
        onSegment(a0, a1, b1) ||
        onSegment(b0, b1, a0) ||
        onSegment(b0, b1, a1);
}

[[nodiscard]] bool legacyRingIntersects(const Ring& a, const Ring& b) {
    const Ring na = normalizeRing(a);
    const Ring nb = normalizeRing(b);
    if (na.empty() || nb.empty()) {
        return false;
    }

    if (!GeometryAdapter::intersects(ringBounds(na), ringBounds(nb))) {
        return false;
    }

    for (size_t i = 0; i < na.size(); ++i) {
        const Core::Vec2 a0 = na[i];
        const Core::Vec2 a1 = na[(i + 1) % na.size()];
        for (size_t j = 0; j < nb.size(); ++j) {
            const Core::Vec2 b0 = nb[j];
            const Core::Vec2 b1 = nb[(j + 1) % nb.size()];
            if (segmentsIntersect(a0, a1, b0, b1)) {
                return true;
            }
        }
    }

    return pointInsideRing(na.front(), nb) || pointInsideRing(nb.front(), na);
}

#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
using BoostPoint = boost::geometry::model::d2::point_xy<double>;
using BoostBox = boost::geometry::model::box<BoostPoint>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;

[[nodiscard]] BoostPolygon toBoostPolygon(const Ring& ring) {
    BoostPolygon poly{};
    const Ring normalized = normalizeRing(ring);
    if (normalized.empty()) {
        return poly;
    }

    auto& outer = poly.outer();
    outer.reserve(normalized.size() + 1);
    for (const auto& p : normalized) {
        outer.emplace_back(p.x, p.y);
    }
    outer.emplace_back(normalized.front().x, normalized.front().y);

    boost::geometry::correct(poly);
    return poly;
}
#endif

} // namespace

Backend GeometryAdapter::backend() noexcept {
#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
    return Backend::BoostGeometry;
#else
    return Backend::LegacyGeos;
#endif
}

const char* GeometryAdapter::backendName() noexcept {
#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
    return "Boost.Geometry";
#elif defined(ROGUECITY_HAS_GEOS)
    return "GEOS";
#else
    return "LegacyShim";
#endif
}

double GeometryAdapter::distance(const Core::Vec2& a, const Core::Vec2& b) noexcept {
#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
    const BoostPoint pa(a.x, a.y);
    const BoostPoint pb(b.x, b.y);
    return boost::geometry::distance(pa, pb);
#else
    return a.distanceTo(b);
#endif
}

bool GeometryAdapter::intersects(const Core::Bounds& a, const Core::Bounds& b) noexcept {
    const double a_min_x = std::min(a.min.x, a.max.x);
    const double a_min_y = std::min(a.min.y, a.max.y);
    const double a_max_x = std::max(a.min.x, a.max.x);
    const double a_max_y = std::max(a.min.y, a.max.y);
    const double b_min_x = std::min(b.min.x, b.max.x);
    const double b_min_y = std::min(b.min.y, b.max.y);
    const double b_max_x = std::max(b.min.x, b.max.x);
    const double b_max_y = std::max(b.min.y, b.max.y);

#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
    const BoostBox ba(BoostPoint(a_min_x, a_min_y), BoostPoint(a_max_x, a_max_y));
    const BoostBox bb(BoostPoint(b_min_x, b_min_y), BoostPoint(b_max_x, b_max_y));
    return boost::geometry::intersects(ba, bb);
#else
    return a_min_x <= b_max_x &&
        a_max_x >= b_min_x &&
        a_min_y <= b_max_y &&
        a_max_y >= b_min_y;
#endif
}

bool GeometryAdapter::intersects(const Ring& a, const Ring& b) {
#if defined(ROGUECITY_USE_BOOST_GEOMETRY)
    const BoostPolygon pa = toBoostPolygon(a);
    const BoostPolygon pb = toBoostPolygon(b);
    if (pa.outer().size() < 4 || pb.outer().size() < 4) {
        return false;
    }
    return boost::geometry::intersects(pa, pb);
#else
    return legacyRingIntersects(a, b);
#endif
}

} // namespace RogueCity::Generators::Geometry
