#include "LotGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <numeric>

#include "DebugLog.hpp"
#include "BlockGenerator.h"
#include "FrontageProfiles.h"
#include "Graph.h"
#include "PolygonUtil.h"

namespace
{
    using CityModel::LotType;
    using CityModel::RoadType;
    using CityModel::Vec2;

    struct Scores
    {
        double A{0.0};
        double E{0.0};
        double S{0.0};
        double P{0.0};
    };

    double polyline_length(const std::vector<Vec2> &points)
    {
        if (points.size() < 2)
        {
            return 0.0;
        }
        double total = 0.0;
        for (std::size_t i = 0; i + 1 < points.size(); ++i)
        {
            total += CityModel::distance(points[i], points[i + 1]);
        }
        return total;
    }

    Vec2 direction_at_distance(const std::vector<Vec2> &points, double dist)
    {
        if (points.size() < 2)
        {
            return Vec2{1.0, 0.0};
        }
        double traveled = 0.0;
        for (std::size_t i = 0; i + 1 < points.size(); ++i)
        {
            const Vec2 &a = points[i];
            const Vec2 &b = points[i + 1];
            const double seg_len = CityModel::distance(a, b);
            if (traveled + seg_len >= dist)
            {
                Vec2 dir{b.x - a.x, b.y - a.y};
                return dir.normalize();
            }
            traveled += seg_len;
        }
        Vec2 dir{points.back().x - points[points.size() - 2].x,
                 points.back().y - points[points.size() - 2].y};
        return dir.normalize();
    }

    Vec2 point_at_distance(const std::vector<Vec2> &points, double dist)
    {
        if (points.size() < 2)
        {
            return points.empty() ? Vec2{0.0, 0.0} : points.front();
        }
        double traveled = 0.0;
        for (std::size_t i = 0; i + 1 < points.size(); ++i)
        {
            const Vec2 &a = points[i];
            const Vec2 &b = points[i + 1];
            const double seg_len = CityModel::distance(a, b);
            if (traveled + seg_len >= dist)
            {
                const double t = (seg_len <= 0.0) ? 0.0 : (dist - traveled) / seg_len;
                return CityModel::lerp(a, b, t);
            }
            traveled += seg_len;
        }
        return points.back();
    }

    bool in_bounds(const CityModel::Bounds &bounds, const Vec2 &p)
    {
        return p.x >= bounds.min.x && p.y >= bounds.min.y && p.x <= bounds.max.x && p.y <= bounds.max.y;
    }

    double distance_to_segment(const Vec2 &p, const Vec2 &a, const Vec2 &b)
    {
        const double vx = b.x - a.x;
        const double vy = b.y - a.y;
        const double wx = p.x - a.x;
        const double wy = p.y - a.y;
        const double c1 = wx * vx + wy * vy;
        if (c1 <= 0.0)
        {
            return CityModel::distance(p, a);
        }
        const double c2 = vx * vx + vy * vy;
        if (c2 <= c1)
        {
            return CityModel::distance(p, b);
        }
        const double t = c1 / c2;
        Vec2 proj{a.x + t * vx, a.y + t * vy};
        return CityModel::distance(p, proj);
    }

    bool point_inside_block_stub(const Vec2 &p,
                                 const CityModel::BlockPolygon &block)
    {
        if (!PolygonUtil::insidePolygon(p, block.outer))
        {
            return false;
        }
        for (const auto &hole : block.holes)
        {
            if (PolygonUtil::insidePolygon(p, hole))
            {
                return false;
            }
        }
        return true;
    }

    Scores compute_scores(RoadType primary, RoadType secondary, bool has_secondary)
    {
        const auto primary_profile = CityModel::FrontageProfiles::get(primary);
        const auto secondary_profile = has_secondary ? CityModel::FrontageProfiles::get(secondary) : CityModel::RoadFrontageProfile{};

        Scores s;
        s.A = 0.85 * primary_profile.access + 0.15 * secondary_profile.access;
        s.E = 0.90 * primary_profile.exposure + 0.10 * secondary_profile.exposure;
        s.S = 0.65 * primary_profile.serviceability + 0.35 * secondary_profile.serviceability;
        s.P = 0.80 * primary_profile.privacy + 0.20 * secondary_profile.privacy;
        return s;
    }

    double primary_bonus(LotType type, RoadType primary)
    {
        switch (type)
        {
        case LotType::LogisticsIndustrial:
            if (primary == RoadType::Highway || primary == RoadType::Arterial)
                return 0.25;
            if (primary == RoadType::Avenue)
                return 0.10;
            return 0.0;
        case LotType::RetailStrip:
            if (primary == RoadType::Arterial)
                return 0.30;
            if (primary == RoadType::Boulevard)
                return 0.20;
            if (primary == RoadType::Avenue)
                return 0.15;
            return 0.0;
        case LotType::MixedUse:
            if (primary == RoadType::Avenue || primary == RoadType::Boulevard || primary == RoadType::Street)
                return 0.15;
            return 0.0;
        case LotType::CivicCultural:
            if (primary == RoadType::Boulevard)
                return 0.30;
            if (primary == RoadType::Avenue)
                return 0.20;
            return 0.0;
        case LotType::Residential:
            if (primary == RoadType::Street)
                return 0.25;
            if (primary == RoadType::Lane)
                return 0.20;
            if (primary == RoadType::Drive)
                return 0.20;
            if (primary == RoadType::CulDeSac)
                return 0.15;
            return 0.0;
        case LotType::LuxuryScenic:
            if (primary == RoadType::Drive)
                return 0.30;
            if (primary == RoadType::Boulevard)
                return 0.25;
            if (primary == RoadType::CulDeSac)
                return 0.25;
            return 0.0;
        case LotType::RowhomeCompact:
            if (primary == RoadType::Lane)
                return 0.25;
            if (primary == RoadType::Street)
                return 0.20;
            return 0.0;
        case LotType::BufferStrip:
            if (primary == RoadType::Highway)
                return 1.0;
            return 0.0;
        default:
            return 0.0;
        }
    }

    double secondary_bonus(LotType type, RoadType secondary, bool has_secondary)
    {
        if (!has_secondary)
        {
            return 0.0;
        }
        switch (type)
        {
        case LotType::LogisticsIndustrial:
            if (secondary == RoadType::Alleyway)
                return 0.25;
            if (secondary == RoadType::Driveway)
                return 0.10;
            return 0.0;
        case LotType::RetailStrip:
            if (secondary == RoadType::Alleyway)
                return 0.20;
            return 0.0;
        case LotType::MixedUse:
            if (secondary == RoadType::Alleyway)
                return 0.20;
            if (secondary == RoadType::Lane)
                return 0.10;
            return 0.0;
        case LotType::CivicCultural:
            if (secondary == RoadType::Alleyway)
                return 0.10;
            return 0.0;
        case LotType::Residential:
            if (secondary == RoadType::Lane || secondary == RoadType::Driveway)
                return 0.10;
            return 0.0;
        case LotType::LuxuryScenic:
            if (secondary == RoadType::Lane || secondary == RoadType::Driveway)
                return 0.10;
            return 0.0;
        case LotType::RowhomeCompact:
            if (secondary == RoadType::Alleyway)
                return 0.20;
            if (secondary == RoadType::Driveway)
                return 0.10;
            return 0.0;
        default:
            return 0.0;
        }
    }

    double combo_bonus(LotType type, RoadType primary, RoadType secondary, bool has_secondary)
    {
        if (!has_secondary)
        {
            return 0.0;
        }
        switch (type)
        {
        case LotType::LogisticsIndustrial:
            if (primary == RoadType::Arterial && secondary == RoadType::Alleyway)
                return 0.35;
            if (primary == RoadType::Highway && secondary == RoadType::Alleyway)
                return 0.20;
            if (primary == RoadType::Avenue && secondary == RoadType::Alleyway)
                return 0.15;
            return 0.0;
        case LotType::RetailStrip:
            if (primary == RoadType::Arterial && secondary == RoadType::Alleyway)
                return 0.35;
            if (primary == RoadType::Boulevard && secondary == RoadType::Alleyway)
                return 0.25;
            if (primary == RoadType::Avenue && secondary == RoadType::Alleyway)
                return 0.20;
            return 0.0;
        case LotType::MixedUse:
            if (primary == RoadType::Avenue && secondary == RoadType::Alleyway)
                return 0.25;
            if (primary == RoadType::Boulevard && secondary == RoadType::Alleyway)
                return 0.20;
            if (primary == RoadType::Street && secondary == RoadType::Alleyway)
                return 0.15;
            return 0.0;
        case LotType::CivicCultural:
            if (primary == RoadType::Boulevard && secondary == RoadType::Alleyway)
                return 0.15;
            if (primary == RoadType::Avenue && secondary == RoadType::Alleyway)
                return 0.10;
            return 0.0;
        case LotType::Residential:
            if (primary == RoadType::Street && secondary == RoadType::Lane)
                return 0.15;
            if (primary == RoadType::Lane && secondary == RoadType::Driveway)
                return 0.15;
            if (primary == RoadType::Drive && secondary == RoadType::Lane)
                return 0.10;
            return 0.0;
        case LotType::LuxuryScenic:
            if (primary == RoadType::Drive && secondary == RoadType::Lane)
                return 0.15;
            if (primary == RoadType::Boulevard && secondary == RoadType::Driveway)
                return 0.10;
            if (primary == RoadType::CulDeSac && secondary == RoadType::Driveway)
                return 0.10;
            return 0.0;
        case LotType::RowhomeCompact:
            if (primary == RoadType::Lane && secondary == RoadType::Alleyway)
                return 0.25;
            if (primary == RoadType::Street && secondary == RoadType::Alleyway)
                return 0.20;
            return 0.0;
        default:
            return 0.0;
        }
    }

    bool thresholds_pass(LotType type, const Scores &s)
    {
        switch (type)
        {
        case LotType::LogisticsIndustrial:
            return s.S >= 0.80 && s.A >= 0.70;
        case LotType::RetailStrip:
            return s.E >= 0.80 && s.A >= 0.60;
        case LotType::MixedUse:
            return s.E >= 0.70 && s.A >= 0.60 && s.P >= 0.30;
        case LotType::CivicCultural:
            return s.E >= 0.80 && s.P >= 0.40;
        case LotType::Residential:
            return s.P >= 0.60 && s.A >= 0.55;
        case LotType::LuxuryScenic:
            return s.P >= 0.80 && s.A >= 0.45;
        case LotType::RowhomeCompact:
            return s.A >= 0.55 && s.P >= 0.50 && s.E <= 0.60;
        case LotType::BufferStrip:
            return true;
        default:
            return false;
        }
    }

    double weighted_score(LotType type, const Scores &s)
    {
        switch (type)
        {
        case LotType::LogisticsIndustrial:
            return 0.35 * s.A + 0.05 * s.E + 0.55 * s.S + 0.05 * s.P;
        case LotType::RetailStrip:
            return 0.35 * s.A + 0.55 * s.E + 0.05 * s.S + 0.05 * s.P;
        case LotType::MixedUse:
            return 0.30 * s.A + 0.45 * s.E + 0.15 * s.S + 0.10 * s.P;
        case LotType::CivicCultural:
            return 0.10 * s.A + 0.60 * s.E + 0.05 * s.S + 0.25 * s.P;
        case LotType::Residential:
            return 0.25 * s.A + 0.05 * s.E + 0.10 * s.S + 0.60 * s.P;
        case LotType::LuxuryScenic:
            return 0.20 * s.A + 0.10 * s.E + 0.10 * s.S + 0.60 * s.P;
        case LotType::RowhomeCompact:
            return 0.35 * s.A + (-0.15) * s.E + 0.10 * s.S + 0.70 * s.P;
        case LotType::BufferStrip:
            return 0.0;
        default:
            return 0.0;
        }
    }

    double district_multiplier(CityModel::DistrictType district_type, LotType type)
    {
        switch (district_type)
        {
        case CityModel::DistrictType::Residential:
            return (type == LotType::Residential || type == LotType::RowhomeCompact) ? 0.20 : 0.0;
        case CityModel::DistrictType::Commercial:
            return (type == LotType::RetailStrip || type == LotType::MixedUse) ? 0.20 : 0.0;
        case CityModel::DistrictType::Civic:
            return (type == LotType::CivicCultural) ? 0.25 : 0.0;
        case CityModel::DistrictType::Industrial:
            return (type == LotType::LogisticsIndustrial) ? 0.25 : 0.0;
        case CityModel::DistrictType::Mixed:
        default:
            return 0.0;
        }
    }

    double road_spacing_multiplier(RoadType type)
    {
        switch (type)
        {
        case RoadType::Highway:
            return 2.0;
        case RoadType::Arterial:
            return 1.6;
        case RoadType::Avenue:
            return 1.3;
        case RoadType::Boulevard:
            return 1.2;
        case RoadType::Street:
            return 0.75;
        case RoadType::Lane:
            return 0.55;
        case RoadType::Alleyway:
            return 0.5;
        case RoadType::CulDeSac:
            return 0.6;
        case RoadType::Drive:
            return 0.6;
        case RoadType::Driveway:
            return 0.5;
        default:
            return 1.0;
        }
    }

    bool too_close_to_existing(const std::vector<CityModel::LotToken> &lots, const Vec2 &pos, double radius)
    {
        const double r2 = radius * radius;
        for (const auto &lot : lots)
        {
            if (pos.distanceToSquared(lot.centroid) <= r2)
            {
                return true;
            }
        }
        return false;
    }

    LotType classify_lot(RoadType primary, RoadType secondary, bool has_secondary,
                         CityModel::DistrictType district_type,
                         double &out_score)
    {
        if (primary == RoadType::Highway)
        {
            if (!has_secondary || !(secondary == RoadType::Arterial || secondary == RoadType::Avenue ||
                                    secondary == RoadType::Boulevard || secondary == RoadType::Street || secondary == RoadType::Lane))
            {
                out_score = 0.0;
                return LotType::BufferStrip;
            }
        }

        const Scores s = compute_scores(primary, secondary, has_secondary);

        std::array<LotType, 8> types = {LotType::Residential,
                                        LotType::RowhomeCompact,
                                        LotType::RetailStrip,
                                        LotType::MixedUse,
                                        LotType::LogisticsIndustrial,
                                        LotType::CivicCultural,
                                        LotType::LuxuryScenic,
                                        LotType::BufferStrip};

        LotType best_type = LotType::BufferStrip;
        double best_score = -1e9;
        bool any_passed = false;

        for (const auto type : types)
        {
            if (!thresholds_pass(type, s))
            {
                continue;
            }
            any_passed = true;
            double score = weighted_score(type, s);
            score += primary_bonus(type, primary);
            score += secondary_bonus(type, secondary, has_secondary);
            score += combo_bonus(type, primary, secondary, has_secondary);
            score += district_multiplier(district_type, type);
            if (score > best_score)
            {
                best_score = score;
                best_type = type;
            }
        }

        if (!any_passed)
        {
            if (s.E >= 0.75)
            {
                best_type = LotType::RetailStrip;
            }
            else if (s.P >= 0.60)
            {
                best_type = LotType::Residential;
            }
            else
            {
                best_type = LotType::MixedUse;
            }
            best_score = weighted_score(best_type, s);
            best_score += primary_bonus(best_type, primary);
            best_score += district_multiplier(district_type, best_type);
        }

        out_score = best_score;
        return best_type;
    }

    struct RoadCandidate
    {
        RoadType type{RoadType::Street};
        double distance{1e9};
        bool valid{false};
    };

    void check_polyline(const std::vector<Vec2> &points, RoadType type,
                        RoadCandidate &primary, RoadCandidate &secondary, const Vec2 &pos)
    {
        if (points.size() < 2)
        {
            return;
        }
        double best_dist = 1e9;
        for (std::size_t i = 0; i + 1 < points.size(); ++i)
        {
            best_dist = std::min(best_dist, distance_to_segment(pos, points[i], points[i + 1]));
        }
        if (best_dist < primary.distance)
        {
            secondary = primary;
            primary.type = type;
            primary.distance = best_dist;
            primary.valid = true;
        }
        else if (best_dist < secondary.distance)
        {
            secondary.type = type;
            secondary.distance = best_dist;
            secondary.valid = true;
        }
    }

    RoadCandidate nearest_road(const CityModel::City &city, const Vec2 &pos, RoadCandidate &out_secondary)
    {
        RoadCandidate primary{};
        RoadCandidate secondary{};

        // Check segment roads first (graph-processed edges)
        for (auto type : CityModel::generated_road_order)
        {
            const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
            for (const auto &road : segments)
            {
                check_polyline(road.points, type, primary, secondary, pos);
            }
        }

        // Also check polyline roads (original streamlines - may have more coverage)
        for (auto type : CityModel::generated_road_order)
        {
            const auto &polylines = city.roads_by_type[CityModel::road_type_index(type)];
            for (const auto &poly : polylines)
            {
                check_polyline(poly.points, type, primary, secondary, pos);
            }
        }

        out_secondary = secondary;
        return primary;
    }

} // namespace

namespace LotGenerator
{
    void generate(const CityParams &params,
                  const std::vector<AxiomInput> &axioms,
                  const std::vector<CityModel::District> &districts,
                  const DistrictGenerator::DistrictField &field,
                  const TensorField::TensorField &tensor_field,
                  CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs)
    {
        (void)axioms;
        (void)tensor_field;
        std::size_t segment_road_count = 0;
        std::size_t polyline_road_count = 0;
        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            segment_road_count += city.segment_roads_by_type[i].size();
            polyline_road_count += city.roads_by_type[i].size();
        }
        const char *road_def_label = params.roadDefinitionMode == RoadDefinitionMode::BySegment ? "segment" : "polyline";
        RCG::DebugLog::printf("[LotGen] start segment_roads=%zu polyline_roads=%zu user_roads=%zu mode=%s max_total=%u max_lots=%u\n",
                              segment_road_count,
                              polyline_road_count,
                              user_inputs.roads.size(),
                              road_def_label,
                              params.maxTotalRoads,
                              (params.maxTotalRoads > 0 ? params.maxTotalRoads / 2 : 0));
        city.lots.clear();

        const double texture_scale = std::clamp(std::sqrt((params.width * params.height) / (1000.0 * 1000.0)), 0.5, 2.0);
        const double base_spacing = 80.0 * texture_scale * params.lotSpacingMultiplier;
        const double min_spacing = 10.0 * texture_scale;
        const double base_depth = 50.0 * texture_scale;
        const int min_lots_per_side = std::clamp(params.minLotsPerRoadSide, 1, 10);

        const uint32_t max_lots = params.maxTotalRoads > 0 ? params.maxTotalRoads / 2 : 0;
        uint32_t next_id = 1;
        bool max_lots_reached = false;
        auto reached_max_lots = [&]()
        {
            return max_lots > 0 && city.lots.size() >= max_lots;
        };

        // Add user-placed lots first
        for (const auto &user_lot : user_inputs.lots)
        {
            CityModel::LotToken lot;
            lot.id = next_id++;
            lot.centroid = user_lot.position;
            lot.lot_type = user_lot.lot_type;
            lot.is_user_placed = true;
            lot.locked_type = user_inputs.lock_user_types || user_lot.locked_type;
            // Assign district by sampling field
            lot.district_id = field.sample_id(user_lot.position);
            city.lots.push_back(lot);
        }

        auto district_type_for = [&](uint32_t district_id)
        {
            if (district_id == 0 || district_id > districts.size())
            {
                return CityModel::DistrictType::Mixed;
            }
            return districts[district_id - 1].type;
        };

        auto district_scales = [&](CityModel::DistrictType type, double &spacing_scale, double &depth_scale)
        {
            switch (type)
            {
            case CityModel::DistrictType::Residential:
                spacing_scale = 0.75;
                depth_scale = 1.05;
                break;
            case CityModel::DistrictType::Commercial:
                spacing_scale = 0.65;
                depth_scale = 0.95;
                break;
            case CityModel::DistrictType::Civic:
                spacing_scale = 0.85;
                depth_scale = 1.10;
                break;
            case CityModel::DistrictType::Industrial:
                spacing_scale = 1.20;
                depth_scale = 1.30;
                break;
            case CityModel::DistrictType::Mixed:
            default:
                spacing_scale = 1.0;
                depth_scale = 1.0;
                break;
            }
        };

        const bool segment_spacing = params.roadDefinitionMode == RoadDefinitionMode::BySegment;

        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            const auto type = static_cast<RoadType>(i);
            if (type == RoadType::M_Major || type == RoadType::M_Minor)
            {
                continue;
            }

            // Always use polylines (original streamlines) for lot generation coverage,
            // but optionally distribute spacing per segment to respect segment-based road mode.
            const auto &paths = city.roads_by_type[i];

            auto emit_lots_for_segment = [&](const Vec2 &a, const Vec2 &b)
            {
                const double length = CityModel::distance(a, b);
                if (length <= 0.0)
                {
                    return;
                }

                const Vec2 mid = CityModel::lerp(a, b, 0.5);
                const uint32_t district_id = field.sample_id(mid);
                const auto district_type = district_type_for(district_id);
                double spacing_scale = 1.0;
                double depth_scale = 1.0;
                district_scales(district_type, spacing_scale, depth_scale);
                const double spacing_multiplier = road_spacing_multiplier(type);
                double lot_spacing = std::max(min_spacing, base_spacing * spacing_scale * spacing_multiplier);
                double lot_depth = std::max(15.0, base_depth * depth_scale);

                // Ensure minimum lots per side: adjust spacing if needed
                const int desired_lots = std::max(min_lots_per_side, 1);
                const double max_spacing_for_min_lots = length / static_cast<double>(desired_lots);
                if (lot_spacing > max_spacing_for_min_lots && length > min_spacing)
                {
                    lot_spacing = std::max(min_spacing, max_spacing_for_min_lots);
                }

                const Vec2 dir = Vec2{b.x - a.x, b.y - a.y}.normalize();
                const Vec2 normal{-dir.y, dir.x};

                const double start = (length < lot_spacing) ? length * 0.5 : lot_spacing * 0.5;
                for (double dist = start; dist <= length; dist += lot_spacing)
                {
                    if (reached_max_lots())
                    {
                        max_lots_reached = true;
                        break;
                    }

                    const double t = (length <= 0.0) ? 0.0 : (dist / length);
                    const Vec2 pos = CityModel::lerp(a, b, t);

                    for (double side : {1.0, -1.0})
                    {
                        Vec2 centroid{pos.x + normal.x * (lot_depth * 0.5 * side),
                                      pos.y + normal.y * (lot_depth * 0.5 * side)};

                        if (!in_bounds(city.bounds, centroid))
                        {
                            continue;
                        }

                        RoadCandidate secondary_candidate{};
                        RoadCandidate primary_candidate = nearest_road(city, centroid, secondary_candidate);
                        if (!primary_candidate.valid)
                        {
                            primary_candidate.type = type;
                            primary_candidate.valid = true;
                        }
                        bool has_secondary = secondary_candidate.valid &&
                                             secondary_candidate.distance <= primary_candidate.distance * 2.25;

                        const RoadType primary = primary_candidate.type;
                        const RoadType secondary = has_secondary ? secondary_candidate.type : primary_candidate.type;

                        Scores scores = compute_scores(primary, secondary, has_secondary);
                        CityModel::LotToken lot;
                        lot.id = next_id++;
                        lot.centroid = centroid;
                        lot.district_id = field.sample_id(centroid);
                        lot.primary_road = primary;
                        lot.secondary_road = secondary;
                        lot.access = static_cast<float>(scores.A);
                        lot.exposure = static_cast<float>(scores.E);
                        lot.serviceability = static_cast<float>(scores.S);
                        lot.privacy = static_cast<float>(scores.P);

                        double score = 0.0;
                        const auto district_type_for_lot = district_type_for(lot.district_id);
                        lot.lot_type = classify_lot(primary, secondary, has_secondary, district_type_for_lot, score);

                        city.lots.push_back(lot);
                        if (reached_max_lots())
                        {
                            max_lots_reached = true;
                            break;
                        }
                    }
                    if (max_lots_reached)
                    {
                        break;
                    }
                }
                if (max_lots_reached)
                {
                    return;
                }
            };

            for (const auto &road : paths)
            {
                const auto &points = road.points;
                if (points.size() < 2)
                {
                    continue;
                }

                if (segment_spacing)
                {
                    for (std::size_t p = 0; p + 1 < points.size(); ++p)
                    {
                        emit_lots_for_segment(points[p], points[p + 1]);
                        if (max_lots_reached)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    const double length = polyline_length(points);
                    if (length <= 0.0)
                    {
                        continue;
                    }

                    const Vec2 mid = point_at_distance(points, length * 0.5);
                    const uint32_t district_id = field.sample_id(mid);
                    const auto district_type = district_type_for(district_id);
                    double spacing_scale = 1.0;
                    double depth_scale = 1.0;
                    district_scales(district_type, spacing_scale, depth_scale);
                    const double spacing_multiplier = road_spacing_multiplier(type);
                    double lot_spacing = std::max(min_spacing, base_spacing * spacing_scale * spacing_multiplier);
                    double lot_depth = std::max(15.0, base_depth * depth_scale);

                    // Ensure minimum lots per side: adjust spacing if needed
                    const int desired_lots = std::max(min_lots_per_side, 1);
                    const double max_spacing_for_min_lots = length / static_cast<double>(desired_lots);
                    if (lot_spacing > max_spacing_for_min_lots && length > min_spacing)
                    {
                        lot_spacing = std::max(min_spacing, max_spacing_for_min_lots);
                    }

                    const double start = (length < lot_spacing) ? length * 0.5 : lot_spacing * 0.5;
                    for (double dist = start; dist <= length; dist += lot_spacing)
                    {
                        if (reached_max_lots())
                        {
                            max_lots_reached = true;
                            break;
                        }

                        const Vec2 pos = point_at_distance(points, dist);
                        const Vec2 dir = direction_at_distance(points, dist);
                        const Vec2 normal{-dir.y, dir.x};

                        for (double side : {1.0, -1.0})
                        {
                            Vec2 centroid{pos.x + normal.x * (lot_depth * 0.5 * side),
                                          pos.y + normal.y * (lot_depth * 0.5 * side)};

                            if (!in_bounds(city.bounds, centroid))
                            {
                                continue;
                            }

                            RoadCandidate secondary_candidate{};
                            RoadCandidate primary_candidate = nearest_road(city, centroid, secondary_candidate);
                            if (!primary_candidate.valid)
                            {
                                primary_candidate.type = type;
                                primary_candidate.valid = true;
                            }
                            bool has_secondary = secondary_candidate.valid &&
                                                 secondary_candidate.distance <= primary_candidate.distance * 2.25;

                            const RoadType primary = primary_candidate.type;
                            const RoadType secondary = has_secondary ? secondary_candidate.type : primary_candidate.type;

                            Scores scores = compute_scores(primary, secondary, has_secondary);
                            CityModel::LotToken lot;
                            lot.id = next_id++;
                            lot.centroid = centroid;
                            lot.district_id = field.sample_id(centroid);
                            lot.primary_road = primary;
                            lot.secondary_road = secondary;
                            lot.access = static_cast<float>(scores.A);
                            lot.exposure = static_cast<float>(scores.E);
                            lot.serviceability = static_cast<float>(scores.S);
                            lot.privacy = static_cast<float>(scores.P);

                            double score = 0.0;
                            const auto district_type_for_lot = district_type_for(lot.district_id);
                            lot.lot_type = classify_lot(primary, secondary, has_secondary, district_type_for_lot, score);

                            city.lots.push_back(lot);
                            if (reached_max_lots())
                            {
                                max_lots_reached = true;
                                break;
                            }
                        }
                        if (max_lots_reached)
                        {
                            break;
                        }
                    }
                }
                if (max_lots_reached)
                {
                    break;
                }
            }
        }

        if (max_lots > 0 && city.lots.size() >= max_lots)
        {
            RCG::DebugLog::printf("[LotGen] Max lots reached (%zu/%u). Continuing to block generation.\n",
                                  city.lots.size(), max_lots);
        }

        std::vector<std::vector<Vec2>> all_roads;
        for (auto type : CityModel::generated_road_order)
        {
            const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
            for (const auto &road : segments)
            {
                all_roads.push_back(road.points);
            }
        }
        for (const auto &road : user_inputs.roads)
        {
            if (road.points.size() >= 2)
            {
                all_roads.push_back(road.points);
            }
        }
        if (all_roads.empty())
        {
            for (auto type : CityModel::generated_road_order)
            {
                const auto &roads = city.roads_by_type[CityModel::road_type_index(type)];
                for (const auto &road : roads)
                {
                    all_roads.push_back(road.points);
                }
            }
        }

        city.block_polygons.clear();
        city.block_faces.clear();
        city.block_stats = CityModel::BlockDebugStats{};
        if (!all_roads.empty())
        {
            RCG::DebugLog::printf("[LotGen] road counts by type (polylines/segments):\n");
            for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
            {
                const auto type = static_cast<RoadType>(i);
                RCG::DebugLog::printf("  %s: polylines=%zu segments=%zu\n",
                                      CityModel::road_type_label(type),
                                      city.roads_by_type[i].size(),
                                      city.segment_roads_by_type[i].size());
            }
            // Use larger merge radius (20.0) for better node connectivity at intersections
            Graph graph(all_roads, 20.0, true);

            BlockGenerator::Settings block_settings;
            // Adaptive minimum block area based on merge_radius to match geometry scale.
            // Reasoning: production default of 100.0 assumes larger coordinate units.
            // Compute adaptive = 0.02 * mr^2 and enforce a sensible floor to eliminate dust.
            const double mr = std::max(0.0, params.merge_radius);
            const double adaptive_min = 0.02 * mr * mr; // mr=20 -> 8.0, mr=5 -> 0.5, mr=1 -> 0.02
            block_settings.min_area = std::max(0.05, adaptive_min);
            block_settings.max_area = 1e8;
            block_settings.merge_radius = params.merge_radius;
            RCG::DebugLog::printf("[LotGenerator] Adaptive block min_area=%.6f (merge_radius=%.6f adaptive=%.6f)\n",
                                  block_settings.min_area, mr, adaptive_min);
            BlockGenerator::Stats block_stats{};
            BlockGenerator::generate(params, city, user_inputs, field, city.block_polygons, &city.block_faces, &block_stats, block_settings);
            city.block_stats.road_inputs = static_cast<uint32_t>(block_stats.road_inputs);
            city.block_stats.faces_found = static_cast<uint32_t>(block_stats.faces_found);
            city.block_stats.valid_blocks = static_cast<uint32_t>(block_stats.valid_blocks);
            city.block_stats.intersections = static_cast<uint32_t>(block_stats.intersections);
            city.block_stats.segments = static_cast<uint32_t>(block_stats.segments);

            if (city.block_polygons.empty())
            {
                RCG::DebugLog::printf("[LotGen] Blocks empty: roads=%zu segments=%zu intersections=%zu faces=%zu valid=%zu\n",
                                      block_stats.road_inputs, block_stats.segments, block_stats.intersections,
                                      block_stats.faces_found, block_stats.valid_blocks);
            }

            if (city.block_polygons.empty())
            {
                city.block_polygons.reserve(city.districts.size());
                for (const auto &district : city.districts)
                {
                    if (district.border.size() < 3)
                    {
                        continue;
                    }
                    CityModel::BlockPolygon polygon;
                    polygon.district_id = district.id;
                    polygon.outer = district.border;
                    if (!polygon.outer.empty() && !polygon.outer.front().equals(polygon.outer.back()))
                    {
                        polygon.outer.push_back(polygon.outer.front());
                    }
                    city.block_polygons.push_back(std::move(polygon));
                }
                city.block_stats.valid_blocks = static_cast<uint32_t>(city.block_polygons.size());
            }

            // Use a local RNG for dispersion
            CityModel::RNG rng(params.seed + 9999);

            for (const auto &polygon : city.block_polygons)
            {
                if (reached_max_lots())
                {
                    max_lots_reached = true;
                    break;
                }

                const auto &poly = polygon.outer;
                double area = PolygonUtil::polygonArea(poly);
                Vec2 polyCenter = PolygonUtil::averagePoint(poly);
                uint32_t district_id = polygon.district_id;
                if (district_id == 0)
                {
                    district_id = field.sample_id(polyCenter);
                }
                auto d_type = district_type_for(district_id);

                // Calculate target density for this district
                double spacing_scale = 1.0, depth_scale = 1.0;
                district_scales(d_type, spacing_scale, depth_scale);

                // Infill is generally less dense than road frontage
                double target_spacing = std::max(min_spacing, base_spacing * spacing_scale * 1.5);

                // Simple area-based estimation for number of potential lots
                int num_points = static_cast<int>(area / (target_spacing * target_spacing));
                if (num_points <= 0)
                    continue;

                // Limit max points per block to avoid freezing on huge blocks
                num_points = std::min(num_points, 500);

                // Generate candidates using Poisson-disk-like rejection sampling
                std::vector<Vec2> candidates;
                CityModel::Bounds polyBounds = {{1e9, 1e9}, {-1e9, -1e9}};
                for (const auto &v : poly)
                {
                    polyBounds.min.x = std::min(polyBounds.min.x, v.x);
                    polyBounds.min.y = std::min(polyBounds.min.y, v.y);
                    polyBounds.max.x = std::max(polyBounds.max.x, v.x);
                    polyBounds.max.y = std::max(polyBounds.max.y, v.y);
                }

                int attempts = num_points * 10;
                for (int k = 0; k < attempts && candidates.size() < (size_t)num_points; ++k)
                {
                    Vec2 p;
                    p.x = rng.uniform(polyBounds.min.x, polyBounds.max.x);
                    p.y = rng.uniform(polyBounds.min.y, polyBounds.max.y);

                    if (!point_inside_block_stub(p, polygon))
                        continue;

                    // Check against existing road-based lots
                    if (too_close_to_existing(city.lots, p, target_spacing))
                        continue;

                    // Check against other candidates
                    bool close = false;
                    for (const auto &c : candidates)
                    {
                        if (p.distanceToSquared(c) < target_spacing * target_spacing)
                        {
                            close = true;
                            break;
                        }
                    }
                    if (close)
                        continue;

                    candidates.push_back(p);
                }

                // Convert candidates to lots
                for (const auto &p : candidates)
                {
                    RoadCandidate sec_cand{};
                    RoadCandidate prim_cand = nearest_road(city, p, sec_cand);
                    if (!prim_cand.valid)
                        continue;

                    CityModel::LotToken lot;
                    lot.id = next_id++;
                    lot.centroid = p;
                    lot.district_id = district_id;
                    lot.primary_road = prim_cand.type;

                    // Infill lots might not have a clear secondary road, or we treat them as
                    // having the same access as primary but further away.
                    lot.secondary_road = prim_cand.type;

                    Scores scores = compute_scores(prim_cand.type, prim_cand.type, false);

                    // Backlots have higher privacy but lower access/exposure
                    scores.A *= 0.5;
                    scores.E *= 0.4;
                    scores.P = std::min(1.0, scores.P * 1.5);

                    lot.access = static_cast<float>(scores.A);
                    lot.exposure = static_cast<float>(scores.E);
                    lot.serviceability = static_cast<float>(scores.S);
                    lot.privacy = static_cast<float>(scores.P);

                    double score_val = 0.0;
                    lot.lot_type = classify_lot(lot.primary_road, lot.secondary_road, false, d_type, score_val);

                    // If classification failed to find a good specific type, default to Residential or whatever fits P
                    if (lot.lot_type == LotType::BufferStrip && scores.P > 0.5)
                    {
                        lot.lot_type = LotType::Residential;
                    }

                    city.lots.push_back(lot);
                    if (reached_max_lots())
                    {
                        max_lots_reached = true;
                        break;
                    }
                }
                if (max_lots_reached)
                {
                    break;
                }
            }

            // -------------------------------------------------------------------------
            // 3. Intersection Logic (Existing)
            // -------------------------------------------------------------------------
            const double intersection_radius = std::max(10.0, base_spacing * 0.25);
            for (const auto &node : graph.nodes)
            {
                if (node.adj.size() < 3)
                {
                    continue;
                }
                if (reached_max_lots())
                {
                    max_lots_reached = true;
                    break;
                }

                std::vector<Vec2> ring;
                ring.reserve(node.adj.size());
                for (int idx : node.adj)
                {
                    Vec2 dir = graph.nodes[idx].value.clone().sub(node.value);
                    if (dir.lengthSquared() < 1e-9)
                    {
                        continue;
                    }
                    dir.normalize();
                    ring.push_back(node.value.clone().add(Vec2{dir.x * intersection_radius, dir.y * intersection_radius}));
                }

                if (ring.size() < 3)
                {
                    continue;
                }

                std::sort(ring.begin(), ring.end(), [&](const Vec2 &a, const Vec2 &b)
                          {
                              const double angA = std::atan2(a.y - node.value.y, a.x - node.value.x);
                              const double angB = std::atan2(b.y - node.value.y, b.x - node.value.x);
                              return angA < angB; });

                const double area = PolygonUtil::polygonArea(ring);
                if (area < 60.0)
                {
                    continue;
                }

                const Vec2 centroid = PolygonUtil::averagePoint(ring);
                if (!in_bounds(city.bounds, centroid))
                {
                    continue;
                }
                if (too_close_to_existing(city.lots, centroid, intersection_radius * 0.6))
                {
                    continue;
                }

                RoadCandidate secondary_candidate{};
                RoadCandidate primary_candidate = nearest_road(city, centroid, secondary_candidate);
                if (!primary_candidate.valid)
                {
                    continue;
                }
                bool has_secondary = secondary_candidate.valid &&
                                     secondary_candidate.distance <= primary_candidate.distance * 2.25;

                Scores scores = compute_scores(primary_candidate.type,
                                               has_secondary ? secondary_candidate.type : primary_candidate.type,
                                               has_secondary);
                CityModel::LotToken lot;
                lot.id = next_id++;
                lot.centroid = centroid;
                lot.district_id = field.sample_id(centroid);
                lot.primary_road = primary_candidate.type;
                lot.secondary_road = has_secondary ? secondary_candidate.type : primary_candidate.type;
                lot.access = static_cast<float>(scores.A);
                lot.exposure = static_cast<float>(scores.E);
                lot.serviceability = static_cast<float>(scores.S);
                lot.privacy = static_cast<float>(scores.P);

                double score = 0.0;
                const auto district_type = district_type_for(lot.district_id);
                lot.lot_type = classify_lot(lot.primary_road, lot.secondary_road, has_secondary, district_type, score);

                city.lots.push_back(lot);
                if (reached_max_lots())
                {
                    max_lots_reached = true;
                    break;
                }
            }
        }

        RCG::DebugLog::printf("[LotGen] done lots=%zu blocks=%zu faces=%zu\n",
                              city.lots.size(), city.block_polygons.size(), city.block_faces.size());
    }
} // namespace LotGenerator
