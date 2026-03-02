#include "RogueCity/Generators/Urban/LotGenerator.hpp"

#include "RogueCity/Core/Geometry/PolygonOps.hpp"
#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"
#include "RogueCity/Generators/Urban/FrontageProfiles.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace RogueCity::Generators::Urban {

    namespace {

        using GeoPolygon = Core::Geometry::Polygon;
        using GeoRegion = Core::Geometry::PolygonRegion;
        using GeoOps = Core::Geometry::PolygonOps;

        constexpr double kGeomEpsilon = 1e-6;

        // Tracks nearest and second-nearest road types for AESP-style lot context.
        struct NearestRoads {
            Core::RoadType primary{ Core::RoadType::Street };
            Core::RoadType secondary{ Core::RoadType::Street };
            double d1{ 1e12 };
            double d2{ 1e12 };
            bool has_secondary{ false };
        };

        // Standard point-to-segment distance helper.
        double distanceToSegment(const Core::Vec2& p, const Core::Vec2& a, const Core::Vec2& b) {
            const double vx = b.x - a.x;
            const double vy = b.y - a.y;
            const double wx = p.x - a.x;
            const double wy = p.y - a.y;
            const double c1 = wx * vx + wy * vy;
            if (c1 <= 0.0) {
                return p.distanceTo(a);
            }
            const double c2 = vx * vx + vy * vy;
            if (c2 <= c1) {
                return p.distanceTo(b);
            }
            const double t = c1 / c2;
            Core::Vec2 proj{ a.x + t * vx, a.y + t * vy };
            return p.distanceTo(proj);
        }

        // Returns nearest two road types to point p (by minimal segment distance).
        NearestRoads nearestRoadTypes(const fva::Container<Core::Road>& roads, const Core::Vec2& p) {
            NearestRoads out{};
            for (const auto& road : roads) {
                if (road.points.size() < 2) {
                    continue;
                }
                double best_this_road = 1e12;
                for (size_t i = 0; i + 1 < road.points.size(); ++i) {
                    best_this_road = std::min(best_this_road, distanceToSegment(p, road.points[i], road.points[i + 1]));
                }
                if (best_this_road < out.d1) {
                    out.d2 = out.d1;
                    out.secondary = out.primary;
                    out.d1 = best_this_road;
                    out.primary = road.type;
                } else if (best_this_road < out.d2) {
                    out.d2 = best_this_road;
                    out.secondary = road.type;
                }
            }
            out.has_secondary = out.d2 < 1e11;
            return out;
        }

        // Maps district context + AESP features into concrete lot-use type.
        Core::LotType classifyLot(
            const Core::DistrictType district_type,
            const Generators::RogueProfiler::Scores& aesp) {
            if (aesp.serviceability > 0.80f && aesp.access > 0.65f) {
                return Core::LotType::LogisticsIndustrial;
            }
            if (aesp.exposure > 0.82f && aesp.access > 0.60f) {
                return Core::LotType::RetailStrip;
            }
            if (aesp.privacy > 0.82f) {
                return Core::LotType::LuxuryScenic;
            }

            switch (district_type) {
                case Core::DistrictType::Industrial:
                    return Core::LotType::LogisticsIndustrial;
                case Core::DistrictType::Commercial:
                    return (aesp.privacy > 0.45f) ? Core::LotType::MixedUse : Core::LotType::RetailStrip;
                case Core::DistrictType::Civic:
                    return Core::LotType::CivicCultural;
                case Core::DistrictType::Residential:
                    return (aesp.access > 0.70f && aesp.privacy > 0.50f)
                        ? Core::LotType::RowhomeCompact
                        : Core::LotType::Residential;
                case Core::DistrictType::Mixed:
                default:
                    return Core::LotType::MixedUse;
            }
        }

        [[nodiscard]] bool IsFinite(const Core::Vec2& point) {
            return std::isfinite(point.x) && std::isfinite(point.y);
        }

        [[nodiscard]] GeoPolygon ToGeoPolygon(const std::vector<Core::Vec2>& ring) {
            GeoPolygon poly{};
            poly.vertices = ring;
            return poly;
        }

        [[nodiscard]] bool BuildBuildableRegion(const Core::BlockPolygon& block, GeoRegion& out_region) {
            GeoRegion raw{};
            raw.outer = ToGeoPolygon(block.outer);
            raw.holes.reserve(block.holes.size());
            for (const auto& hole_ring : block.holes) {
                raw.holes.push_back(ToGeoPolygon(hole_ring));
            }
            out_region = GeoOps::SimplifyRegion(raw, kGeomEpsilon);
            return GeoOps::IsValidRegion(out_region, kGeomEpsilon);
        }

        [[nodiscard]] bool PointInsideBuildable(
            const Core::Vec2& point,
            const GeoRegion& region) {
            if (!PolygonUtil::insidePolygon(point, region.outer.vertices)) {
                return false;
            }
            for (const auto& hole : region.holes) {
                if (PolygonUtil::insidePolygon(point, hole.vertices)) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] GeoPolygon CandidateRect(const Core::Vec2& center, double width, double depth) {
            GeoPolygon candidate{};
            candidate.vertices = {
                { center.x - width * 0.5, center.y - depth * 0.5 },
                { center.x + width * 0.5, center.y - depth * 0.5 },
                { center.x + width * 0.5, center.y + depth * 0.5 },
                { center.x - width * 0.5, center.y + depth * 0.5 },
            };
            return candidate;
        }

        [[nodiscard]] bool ExtractLargestIntersection(
            const GeoPolygon& candidate,
            const GeoRegion& buildable_region,
            GeoPolygon& out_polygon,
            double& out_area_abs) {
            out_area_abs = 0.0;
            out_polygon.vertices.clear();

            GeoRegion candidate_region{};
            candidate_region.outer = candidate;
            const auto clipped_regions = GeoOps::ClipRegions(candidate_region, buildable_region);
            for (const auto& region : clipped_regions) {
                if (!region.holes.empty()) {
                    continue;
                }
                const GeoPolygon& poly = region.outer;
                if (!GeoOps::IsValidPolygon(poly, kGeomEpsilon)) {
                    continue;
                }
                const double area_abs = std::abs(PolygonUtil::area(poly.vertices));
                if (area_abs <= out_area_abs + kGeomEpsilon) {
                    continue;
                }
                out_polygon = poly;
                out_area_abs = area_abs;
            }

            return out_area_abs > kGeomEpsilon;
        }

    } // namespace

    // Generates lot tokens by packing candidate rectangles within block polygons.
    // Each accepted lot is enriched with road-context features and typed accordingly.
    std::vector<Core::LotToken> LotGenerator::generate(
        const fva::Container<Core::Road>& roads,
        const std::vector<Core::District>& districts,
        const std::vector<Core::BlockPolygon>& blocks,
        const Config& config,
        uint32_t /*seed*/) {
        std::vector<Core::LotToken> lots;
        lots.reserve(std::min<uint32_t>(config.max_lots, 8192));

        // Build district-id -> district-type lookup for lot typing.
        std::unordered_map<uint32_t, Core::DistrictType> district_types;
        for (const auto& d : districts) {
            district_types[d.id] = d.type;
        }

        // Current strategy uses midpoint dimensions as fixed tiling step.
        const double lot_w = std::clamp((config.min_lot_width + config.max_lot_width) * 0.5f, 4.0f, 200.0f);
        const double lot_d = std::clamp((config.min_lot_depth + config.max_lot_depth) * 0.5f, 4.0f, 200.0f);

        uint32_t next_id = 1;
        for (const auto& block : blocks) {
            if (lots.size() >= config.max_lots || block.outer.size() < 3) {
                break;
            }

            GeoRegion buildable_region{};
            if (!BuildBuildableRegion(block, buildable_region)) {
                continue;
            }

            const auto bbox = PolygonUtil::bounds(block.outer);
            for (double y = bbox.min.y + lot_d * 0.5; y < bbox.max.y; y += lot_d) {
                for (double x = bbox.min.x + lot_w * 0.5; x < bbox.max.x; x += lot_w) {
                    if (lots.size() >= config.max_lots) {
                        break;
                    }
                    const Core::Vec2 c{ x, y };
                    if (!PointInsideBuildable(c, buildable_region)) {
                        continue;
                    }

                    const GeoPolygon candidate = CandidateRect(c, lot_w, lot_d);
                    GeoPolygon clipped_lot{};
                    double clipped_area = 0.0;
                    if (!ExtractLargestIntersection(candidate, buildable_region, clipped_lot, clipped_area)) {
                        continue;
                    }

                    const double target_tile_area = lot_w * lot_d;
                    const double min_area = std::min(static_cast<double>(config.min_lot_area), target_tile_area);
                    const double max_area = std::max(static_cast<double>(config.max_lot_area), target_tile_area);
                    if (clipped_area + kGeomEpsilon < min_area ||
                        clipped_area > max_area + kGeomEpsilon) {
                        continue;
                    }

                    // Compute road-context features and classify lot type.
                    const NearestRoads near_roads = nearestRoadTypes(roads, c);
                    const auto aesp = Generators::RogueProfiler::computeScores(
                        near_roads.primary,
                        near_roads.has_secondary ? near_roads.secondary : near_roads.primary);

                    Core::LotToken lot;
                    lot.id = next_id++;
                    lot.district_id = block.district_id;
                    lot.centroid = PolygonUtil::centroid(clipped_lot.vertices);
                    if (!IsFinite(lot.centroid) ||
                        !PolygonUtil::insidePolygon(lot.centroid, clipped_lot.vertices)) {
                        lot.centroid = c;
                    }
                    lot.primary_road = near_roads.primary;
                    lot.secondary_road = near_roads.has_secondary ? near_roads.secondary : near_roads.primary;
                    lot.access = aesp.access;
                    lot.exposure = aesp.exposure;
                    lot.serviceability = aesp.serviceability;
                    lot.privacy = aesp.privacy;
                    lot.area = static_cast<float>(clipped_area);
                    lot.boundary = std::move(clipped_lot.vertices);

                    // Blend district archetype with AESP to pick final lot program.
                    auto it = district_types.find(lot.district_id);
                    const Core::DistrictType district_type =
                        (it != district_types.end()) ? it->second : Core::DistrictType::Mixed;
                    lot.lot_type = classifyLot(district_type, aesp);
                    lots.push_back(std::move(lot));
                }
            }
        }

        return lots;
    }

} // namespace RogueCity::Generators::Urban
