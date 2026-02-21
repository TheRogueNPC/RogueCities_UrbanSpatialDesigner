#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <cmath>

namespace RogueCity::Generators::Urban::PolygonUtil {

    // Signed area (shoelace formula). Positive/negative indicates winding.
    double area(const std::vector<Core::Vec2>& ring) {
        if (ring.size() < 3) {
            return 0.0;
        }
        double a = 0.0;
        for (size_t i = 0; i < ring.size(); ++i) {
            const auto& p0 = ring[i];
            const auto& p1 = ring[(i + 1) % ring.size()];
            a += p0.x * p1.y - p1.x * p0.y;
        }
        return 0.5 * a;
    }

    // Polygon centroid. Falls back to arithmetic mean for degenerate rings.
    Core::Vec2 centroid(const std::vector<Core::Vec2>& ring) {
        if (ring.empty()) {
            return {};
        }

        const double a = area(ring);
        if (std::abs(a) < 1e-8) {
            Core::Vec2 c{};
            for (const auto& p : ring) {
                c += p;
            }
            return c / static_cast<double>(ring.size());
        }

        double cx = 0.0;
        double cy = 0.0;
        for (size_t i = 0; i < ring.size(); ++i) {
            const auto& p0 = ring[i];
            const auto& p1 = ring[(i + 1) % ring.size()];
            const double cross = p0.x * p1.y - p1.x * p0.y;
            cx += (p0.x + p1.x) * cross;
            cy += (p0.y + p1.y) * cross;
        }

        const double factor = 1.0 / (6.0 * a);
        return { cx * factor, cy * factor };
    }

    // Ray-casting point-in-polygon test.
    bool insidePolygon(const Core::Vec2& p, const std::vector<Core::Vec2>& ring) {
        if (ring.size() < 3) {
            return false;
        }

        bool inside = false;
        for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
            const auto& pi = ring[i];
            const auto& pj = ring[j];
            const bool intersects = ((pi.y > p.y) != (pj.y > p.y)) &&
                (p.x < (pj.x - pi.x) * (p.y - pi.y) / ((pj.y - pi.y) + 1e-12) + pi.x);
            if (intersects) {
                inside = !inside;
            }
        }
        return inside;
    }

    // Ensures ring is explicitly closed (first point repeated as last).
    std::vector<Core::Vec2> closed(const std::vector<Core::Vec2>& ring) {
        if (ring.empty()) {
            return ring;
        }
        std::vector<Core::Vec2> out = ring;
        if (!out.front().equals(out.back())) {
            out.push_back(out.front());
        }
        return out;
    }

    // Axis-aligned bounding box of a polygon ring.
    Core::Bounds bounds(const std::vector<Core::Vec2>& ring) {
        Core::Bounds out{};
        if (ring.empty()) {
            return out;
        }
        out.min = out.max = ring.front();
        for (const auto& p : ring) {
            out.min.x = std::min(out.min.x, p.x);
            out.min.y = std::min(out.min.y, p.y);
            out.max.x = std::max(out.max.x, p.x);
            out.max.y = std::max(out.max.y, p.y);
        }
        return out;
    }

} // namespace RogueCity::Generators::Urban::PolygonUtil
