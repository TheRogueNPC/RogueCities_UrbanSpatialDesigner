#include "RogueCity/Generators/Urban/LotGenerator.hpp"

#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include "RogueCity/Generators/Urban/FrontageProfiles.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <unordered_map>

namespace RogueCity::Generators::Urban {

    namespace {

        struct NearestRoads {
            Core::RoadType primary{ Core::RoadType::Street };
            Core::RoadType secondary{ Core::RoadType::Street };
            double d1{ 1e12 };
            double d2{ 1e12 };
            bool has_secondary{ false };
        };

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

        Core::LotType classifyLot(
            const Core::DistrictType district_type,
            const Generators::AESPClassifier::AESPScores& aesp) {
            if (aesp.S > 0.80f && aesp.A > 0.65f) {
                return Core::LotType::LogisticsIndustrial;
            }
            if (aesp.E > 0.82f && aesp.A > 0.60f) {
                return Core::LotType::RetailStrip;
            }
            if (aesp.P > 0.82f) {
                return Core::LotType::LuxuryScenic;
            }

            switch (district_type) {
                case Core::DistrictType::Industrial:
                    return Core::LotType::LogisticsIndustrial;
                case Core::DistrictType::Commercial:
                    return (aesp.P > 0.45f) ? Core::LotType::MixedUse : Core::LotType::RetailStrip;
                case Core::DistrictType::Civic:
                    return Core::LotType::CivicCultural;
                case Core::DistrictType::Residential:
                    return (aesp.A > 0.70f && aesp.P > 0.50f)
                        ? Core::LotType::RowhomeCompact
                        : Core::LotType::Residential;
                case Core::DistrictType::Mixed:
                default:
                    return Core::LotType::MixedUse;
            }
        }

    } // namespace

    std::vector<Core::LotToken> LotGenerator::generate(
        const fva::Container<Core::Road>& roads,
        const std::vector<Core::District>& districts,
        const std::vector<Core::BlockPolygon>& blocks,
        const Config& config,
        uint32_t /*seed*/) {
        std::vector<Core::LotToken> lots;
        lots.reserve(std::min<uint32_t>(config.max_lots, 8192));

        std::unordered_map<uint32_t, Core::DistrictType> district_types;
        for (const auto& d : districts) {
            district_types[d.id] = d.type;
        }

        const double lot_w = std::clamp((config.min_lot_width + config.max_lot_width) * 0.5f, 4.0f, 200.0f);
        const double lot_d = std::clamp((config.min_lot_depth + config.max_lot_depth) * 0.5f, 4.0f, 200.0f);

        uint32_t next_id = 1;
        for (const auto& block : blocks) {
            if (lots.size() >= config.max_lots || block.outer.size() < 3) {
                break;
            }

            const auto bbox = PolygonUtil::bounds(block.outer);
            for (double y = bbox.min.y + lot_d * 0.5; y < bbox.max.y; y += lot_d) {
                for (double x = bbox.min.x + lot_w * 0.5; x < bbox.max.x; x += lot_w) {
                    if (lots.size() >= config.max_lots) {
                        break;
                    }
                    const Core::Vec2 c{ x, y };
                    if (!PolygonUtil::insidePolygon(c, block.outer)) {
                        continue;
                    }
                    bool in_hole = false;
                    for (const auto& hole : block.holes) {
                        if (PolygonUtil::insidePolygon(c, hole)) {
                            in_hole = true;
                            break;
                        }
                    }
                    if (in_hole) {
                        continue;
                    }

                    const NearestRoads near_roads = nearestRoadTypes(roads, c);
                    const auto aesp = Generators::AESPClassifier::computeScores(
                        near_roads.primary,
                        near_roads.has_secondary ? near_roads.secondary : near_roads.primary);

                    Core::LotToken lot;
                    lot.id = next_id++;
                    lot.district_id = block.district_id;
                    lot.centroid = c;
                    lot.primary_road = near_roads.primary;
                    lot.secondary_road = near_roads.has_secondary ? near_roads.secondary : near_roads.primary;
                    lot.access = aesp.A;
                    lot.exposure = aesp.E;
                    lot.serviceability = aesp.S;
                    lot.privacy = aesp.P;
                    lot.area = static_cast<float>(lot_w * lot_d);
                    lot.boundary = {
                        { c.x - lot_w * 0.5, c.y - lot_d * 0.5 },
                        { c.x + lot_w * 0.5, c.y - lot_d * 0.5 },
                        { c.x + lot_w * 0.5, c.y + lot_d * 0.5 },
                        { c.x - lot_w * 0.5, c.y + lot_d * 0.5 },
                    };

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

