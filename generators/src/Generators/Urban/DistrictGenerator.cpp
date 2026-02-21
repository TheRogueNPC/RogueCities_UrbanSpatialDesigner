#include "RogueCity/Generators/Urban/DistrictGenerator.hpp"

#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"

#include <algorithm>
#include <limits>

namespace RogueCity::Generators::Urban {

    namespace {

        // Nearest-road query result used to map a candidate district cell to road context.
        struct NearestRoad {
            Core::RoadType type{ Core::RoadType::Street };
            Core::Vec2 direction{ 1.0, 0.0 };
            double distance{ std::numeric_limits<double>::max() };
            bool valid{ false };
        };

        // Point-to-segment distance with returned segment direction unit vector.
        double distanceToSegment(
            const Core::Vec2& p,
            const Core::Vec2& a,
            const Core::Vec2& b,
            Core::Vec2& out_dir) {
            const double vx = b.x - a.x;
            const double vy = b.y - a.y;
            const double wx = p.x - a.x;
            const double wy = p.y - a.y;

            const double c1 = wx * vx + wy * vy;
            if (c1 <= 0.0) {
                out_dir = (b - a);
                if (out_dir.lengthSquared() > 0.0) out_dir.normalize();
                return p.distanceTo(a);
            }

            const double c2 = vx * vx + vy * vy;
            if (c2 <= c1) {
                out_dir = (b - a);
                if (out_dir.lengthSquared() > 0.0) out_dir.normalize();
                return p.distanceTo(b);
            }

            const double t = c1 / c2;
            Core::Vec2 projection{ a.x + t * vx, a.y + t * vy };
            out_dir = (b - a);
            if (out_dir.lengthSquared() > 0.0) out_dir.normalize();
            return p.distanceTo(projection);
        }

        // Brute-force nearest-road lookup over all road segments.
        NearestRoad findNearest(const fva::Container<Core::Road>& roads, const Core::Vec2& p) {
            NearestRoad best{};
            for (const auto& road : roads) {
                if (road.points.size() < 2) {
                    continue;
                }
                for (size_t i = 0; i + 1 < road.points.size(); ++i) {
                    Core::Vec2 dir{};
                    const double d = distanceToSegment(p, road.points[i], road.points[i + 1], dir);
                    if (d < best.distance) {
                        best.distance = d;
                        best.type = road.type;
                        best.direction = dir;
                        best.valid = true;
                    }
                }
            }
            return best;
        }

        // Derives bounds from road geometry if caller does not provide usable bounds.
        Core::Bounds deriveBounds(const fva::Container<Core::Road>& roads) {
            Core::Bounds b{};
            bool has_any = false;
            for (const auto& road : roads) {
                for (const auto& p : road.points) {
                    if (!has_any) {
                        b.min = b.max = p;
                        has_any = true;
                    } else {
                        b.min.x = std::min(b.min.x, p.x);
                        b.min.y = std::min(b.min.y, p.y);
                        b.max.x = std::max(b.max.x, p.x);
                        b.max.y = std::max(b.max.y, p.y);
                    }
                }
            }
            if (!has_any) {
                b.min = Core::Vec2(0.0, 0.0);
                b.max = Core::Vec2(2000.0, 2000.0);
            }
            return b;
        }

    } // namespace

    // Convenience overload with default configuration.
    std::vector<Core::District> DistrictGenerator::generate(
        const fva::Container<Core::Road>& roads,
        const Core::Bounds& bounds) {
        return generate(roads, bounds, Config{});
    }

    // Grid-based district synthesis:
    // - partition bounds into coarse cells
    // - keep cells near road influence
    // - classify district type via AESP-derived scores
    std::vector<Core::District> DistrictGenerator::generate(
        const fva::Container<Core::Road>& roads,
        const Core::Bounds& bounds,
        const Config& config) {
        std::vector<Core::District> out;

        Core::Bounds b = bounds;
        if (b.width() <= 0.0 || b.height() <= 0.0) {
            b = deriveBounds(roads);
        }

        const int n = std::clamp(config.grid_resolution, 2, 32);
        const double cell_w = std::max(1.0, b.width() / static_cast<double>(n));
        const double cell_h = std::max(1.0, b.height() / static_cast<double>(n));

        out.reserve(static_cast<size_t>(n * n));
        uint32_t id = 1;

        for (int gy = 0; gy < n; ++gy) {
            for (int gx = 0; gx < n; ++gx) {
                if (id > config.max_districts) {
                    return out;
                }

                const Core::Vec2 center{
                    b.min.x + (static_cast<double>(gx) + 0.5) * cell_w,
                    b.min.y + (static_cast<double>(gy) + 0.5) * cell_h
                };

                const NearestRoad nearest = findNearest(roads, center);
                if (!nearest.valid || nearest.distance > config.road_influence_radius) {
                    continue;
                }

                // Use nearest road type as local context for district classification.
                const auto scores = RogueProfiler::computeScores(nearest.type, nearest.type);
                const Core::DistrictType type = RogueProfiler::classifyDistrict(scores);

                Core::District district;
                district.id = id++;
                district.type = type;
                district.desirability = static_cast<float>(1.0 / (1.0 + nearest.distance));
                district.orientation = nearest.direction;
                // Emit axis-aligned cell rectangle as the district polygon footprint.
                district.border = {
                    { b.min.x + gx * cell_w, b.min.y + gy * cell_h },
                    { b.min.x + (gx + 1) * cell_w, b.min.y + gy * cell_h },
                    { b.min.x + (gx + 1) * cell_w, b.min.y + (gy + 1) * cell_h },
                    { b.min.x + gx * cell_w, b.min.y + (gy + 1) * cell_h }
                };
                out.push_back(std::move(district));
            }
        }

        return out;
    }

} // namespace RogueCity::Generators::Urban
