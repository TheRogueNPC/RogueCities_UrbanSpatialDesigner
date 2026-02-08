#include "SiteGenerator.h"

#include <algorithm>
#include <random>

#include "DebugLog.hpp"
namespace
{
    using CityModel::BuildingSite;
    using CityModel::BuildingType;
    using CityModel::LotToken;
    using CityModel::LotType;
    using CityModel::Road;
    using CityModel::Vec2;

    struct OrientedRect
    {
        Vec2 center{};
        Vec2 dir{1.0, 0.0};
        Vec2 normal{0.0, 1.0};
        double half_width{10.0};
        double half_depth{10.0};
    };

    double clamp(double v, double lo, double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    double distance_to_segment(const Vec2 &p, const Vec2 &a, const Vec2 &b, double &out_t)
    {
        const double vx = b.x - a.x;
        const double vy = b.y - a.y;
        const double wx = p.x - a.x;
        const double wy = p.y - a.y;
        const double c1 = wx * vx + wy * vy;
        if (c1 <= 0.0)
        {
            out_t = 0.0;
            return CityModel::distance(p, a);
        }
        const double c2 = vx * vx + vy * vy;
        if (c2 <= c1)
        {
            out_t = 1.0;
            return CityModel::distance(p, b);
        }
        out_t = c1 / c2;
        Vec2 proj{a.x + out_t * vx, a.y + out_t * vy};
        return CityModel::distance(p, proj);
    }

    bool nearest_road_direction(const CityModel::City &city, const Vec2 &pos, Vec2 &out_dir)
    {
        double best = 1e12;
        Vec2 best_dir{1.0, 0.0};
        bool found = false;

        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            const auto &segments = city.segment_roads_by_type[i];
            for (const auto &road : segments)
            {
                if (road.points.size() < 2)
                {
                    continue;
                }
                for (std::size_t p = 0; p + 1 < road.points.size(); ++p)
                {
                    double t = 0.0;
                    const Vec2 &a = road.points[p];
                    const Vec2 &b = road.points[p + 1];
                    const double d = distance_to_segment(pos, a, b, t);
                    if (d < best)
                    {
                        Vec2 dir{b.x - a.x, b.y - a.y};
                        if (dir.length() > 0.0)
                        {
                            dir.normalize();
                            best = d;
                            best_dir = dir;
                            found = true;
                        }
                    }
                }
            }
        }

        if (found)
        {
            out_dir = best_dir;
        }
        return found;
    }

    bool is_minor_road(CityModel::RoadType type)
    {
        switch (type)
        {
        case CityModel::RoadType::Lane:
        case CityModel::RoadType::Alleyway:
        case CityModel::RoadType::CulDeSac:
        case CityModel::RoadType::Drive:
        case CityModel::RoadType::Driveway:
            return true;
        default:
            return false;
        }
    }

    double desirability_score(const LotToken &lot)
    {
        return 0.50 * lot.access + 0.35 * lot.exposure + 0.15 * lot.serviceability;
    }

    BuildingType building_type_for(const LotToken &lot)
    {
        const bool minor = is_minor_road(lot.primary_road);
        const double desirability = desirability_score(lot);
        const bool allow_major = !minor || desirability >= 0.75;

        if (!allow_major)
        {
            switch (lot.lot_type)
            {
            case LotType::RowhomeCompact:
                return BuildingType::Rowhome;
            case LotType::BufferStrip:
                return BuildingType::Utility;
            default:
                return BuildingType::Residential;
            }
        }

        switch (lot.lot_type)
        {
        case LotType::Residential:
            return BuildingType::Residential;
        case LotType::RowhomeCompact:
            return BuildingType::Rowhome;
        case LotType::RetailStrip:
            return BuildingType::Retail;
        case LotType::MixedUse:
            return BuildingType::MixedUse;
        case LotType::LogisticsIndustrial:
            return BuildingType::Industrial;
        case LotType::CivicCultural:
            return BuildingType::Civic;
        case LotType::LuxuryScenic:
            return BuildingType::Luxury;
        case LotType::BufferStrip:
            return BuildingType::Utility;
        default:
            return BuildingType::None;
        }
    }

    void base_dimensions_for(LotType type, double &out_width, double &out_depth)
    {
        switch (type)
        {
        case LotType::Residential:
            out_width = 26.0;
            out_depth = 34.0;
            break;
        case LotType::RowhomeCompact:
            out_width = 18.0;
            out_depth = 28.0;
            break;
        case LotType::RetailStrip:
            out_width = 40.0;
            out_depth = 30.0;
            break;
        case LotType::MixedUse:
            out_width = 32.0;
            out_depth = 36.0;
            break;
        case LotType::LogisticsIndustrial:
            out_width = 60.0;
            out_depth = 50.0;
            break;
        case LotType::CivicCultural:
            out_width = 46.0;
            out_depth = 38.0;
            break;
        case LotType::LuxuryScenic:
            out_width = 34.0;
            out_depth = 42.0;
            break;
        case LotType::BufferStrip:
            out_width = 20.0;
            out_depth = 20.0;
            break;
        default:
            out_width = 24.0;
            out_depth = 30.0;
            break;
        }
    }

    OrientedRect implied_lot_rect(const CityModel::City &city, const LotToken &lot)
    {
        OrientedRect rect;
        rect.center = lot.centroid;
        Vec2 dir{1.0, 0.0};
        if (nearest_road_direction(city, lot.centroid, dir))
        {
            rect.dir = dir;
        }
        rect.normal = Vec2{-rect.dir.y, rect.dir.x};
        rect.normal.normalize();

        double base_w = 24.0;
        double base_d = 30.0;
        base_dimensions_for(lot.lot_type, base_w, base_d);

        const double frontage_factor = 0.6 + 0.6 * lot.access + 0.4 * lot.exposure;
        const double depth_factor = 0.6 + 0.6 * lot.privacy + 0.2 * lot.serviceability;
        const double width = clamp(base_w * frontage_factor, 12.0, 120.0);
        const double depth = clamp(base_d * depth_factor, 12.0, 120.0);

        rect.half_width = width * 0.5;
        rect.half_depth = depth * 0.5;
        return rect;
    }

    CityModel::RNG make_rng(const CityParams &params, uint32_t lot_id)
    {
        if (params.randomizeSites)
        {
            std::random_device rd;
            const auto seed = static_cast<unsigned int>(rd() ^ (rd() << 16));
            return CityModel::RNG(seed);
        }
        const uint64_t mixed = static_cast<uint64_t>(params.seed) ^ (static_cast<uint64_t>(lot_id) * 0x9E3779B97F4A7C15ULL);
        return CityModel::RNG(static_cast<unsigned int>(mixed & 0xFFFFFFFFu));
    }

    int sites_per_lot(const CityParams &params, LotType type, CityModel::RNG &rng)
    {
        switch (type)
        {
        case LotType::Residential:
            return rng.uniformInt(1, 2);
        case LotType::RowhomeCompact:
            return rng.uniformInt(2, 6);
        case LotType::RetailStrip:
            return rng.uniformInt(1, 3);
        case LotType::MixedUse:
            return rng.uniformInt(1, 2);
        case LotType::LogisticsIndustrial:
            return 1;
        case LotType::CivicCultural:
            return 1;
        case LotType::LuxuryScenic:
            return rng.uniformInt(1, 2);
        case LotType::BufferStrip:
            return (rng.uniform() < params.bufferUtilityChance) ? 1 : 0;
        default:
            return 0;
        }
    }

    Vec2 sample_point(const OrientedRect &rect, CityModel::RNG &rng)
    {
        const double x = rng.uniform(-rect.half_width, rect.half_width);
        const double y = rng.uniform(-rect.half_depth, rect.half_depth);
        Vec2 pos = rect.center;
        pos.x += rect.dir.x * x + rect.normal.x * y;
        pos.y += rect.dir.y * x + rect.normal.y * y;
        return pos;
    }

} // namespace

namespace SiteGenerator
{
    void generate(const CityParams &params,
                  CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs)
    {
        RCG::DebugLog::printf("[SiteGen] start lots=%zu user_buildings=%zu\n",
                              city.lots.size(), user_inputs.buildings.size());
        city.building_sites.clear();
        uint32_t next_id = 1;

        // Add user-placed buildings first
        for (const auto &user_building : user_inputs.buildings)
        {
            BuildingSite site;
            site.id = next_id++;
            site.lot_id = 0; // User-placed buildings may not be associated with lots
            site.district_id = 0;
            site.position = user_building.position;
            site.type = user_building.building_type;
            site.is_user_placed = true;
            site.locked_type = user_inputs.lock_user_types || user_building.locked_type;
            city.building_sites.push_back(site);
        }

        for (const auto &lot : city.lots)
        {
            if (lot.lot_type == LotType::None)
            {
                continue;
            }

            CityModel::RNG rng = make_rng(params, lot.id);
            const int count = sites_per_lot(params, lot.lot_type, rng);
            if (count <= 0)
            {
                continue;
            }

            const OrientedRect rect = implied_lot_rect(city, lot);
            const BuildingType type = building_type_for(lot);

            for (int i = 0; i < count; ++i)
            {
                BuildingSite site;
                site.id = next_id++;
                site.lot_id = lot.id;
                site.district_id = lot.district_id;
                site.position = sample_point(rect, rng);
                site.type = type;
                site.is_user_placed = false;
                site.locked_type = false;
                city.building_sites.push_back(site);
            }
        }

        RCG::DebugLog::printf("[SiteGen] done sites=%zu\n", city.building_sites.size());
    }
}
