#include "ExportSchema.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cmath>

#if RCG_USE_GEOS
#include <geos_c.h>
#endif

using CityModel::BlockPolygon;
using CityModel::Polyline;
using CityModel::Road;
using CityModel::Vec2;

namespace
{

    std::string vec2_to_json(const Vec2 &v)
    {
        std::ostringstream ss;
        ss << "[" << v.x << "," << v.y << "]";
        return ss.str();
    }

    std::string polyline_to_json(const Polyline &poly)
    {
        std::ostringstream ss;
        ss << "{\"points\":[";
        for (std::size_t i = 0; i < poly.points.size(); ++i)
        {
            ss << vec2_to_json(poly.points[i]);
            if (i + 1 < poly.points.size())
            {
                ss << ",";
            }
        }
        ss << "]}";
        return ss.str();
    }

    std::string road_segment_to_json(const Road &road)
    {
        std::ostringstream ss;
        ss << "{\"id\":" << road.id << ",\"points\":[";
        for (std::size_t i = 0; i < road.points.size(); ++i)
        {
            ss << vec2_to_json(road.points[i]);
            if (i + 1 < road.points.size())
            {
                ss << ",";
            }
        }
        ss << "]}";
        return ss.str();
    }

    std::vector<Vec2> lint_ring_points(const std::vector<Vec2> &points)
    {
        std::vector<Vec2> out;
        out.reserve(points.size() + 1);
        auto nearly_equal = [](const Vec2 &a, const Vec2 &b)
        {
            const double dx = a.x - b.x;
            const double dy = a.y - b.y;
            return (dx * dx + dy * dy) < 1e-10;
        };

        for (const auto &p : points)
        {
            if (!std::isfinite(p.x) || !std::isfinite(p.y))
            {
                continue;
            }
            if (!out.empty() && nearly_equal(out.back(), p))
            {
                continue;
            }
            out.push_back(p);
        }

        if (out.size() >= 2 && nearly_equal(out.front(), out.back()))
        {
            out.pop_back();
        }
        if (out.size() >= 3)
        {
            out.push_back(out.front());
        }
        return out;
    }

    std::string ring_points_to_json(const std::vector<Vec2> &ring)
    {
        std::ostringstream ss;
        ss << "[";
        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            ss << vec2_to_json(ring[i]);
            if (i + 1 < ring.size())
            {
                ss << ",";
            }
        }
        ss << "]";
        return ss.str();
    }

    std::string polygon_rings_to_json(const std::vector<std::vector<Vec2>> &rings)
    {
        if (rings.empty())
        {
            return "[]";
        }
        if (rings.size() == 1)
        {
            return ring_points_to_json(rings.front());
        }
        std::ostringstream ss;
        ss << "[";
        for (std::size_t i = 0; i < rings.size(); ++i)
        {
            ss << ring_points_to_json(rings[i]);
            if (i + 1 < rings.size())
            {
                ss << ",";
            }
        }
        ss << "]";
        return ss.str();
    }

#if RCG_USE_GEOS
    GEOSGeometry *make_geos_polygon(GEOSContextHandle_t ctx, const std::vector<Vec2> &ring)
    {
        if (ring.size() < 4)
        {
            return nullptr;
        }
        GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, ring.size(), 2);
        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            GEOSCoordSeq_setX_r(ctx, seq, i, ring[i].x);
            GEOSCoordSeq_setY_r(ctx, seq, i, ring[i].y);
        }
        GEOSGeometry *shell = GEOSGeom_createLinearRing_r(ctx, seq);
        if (!shell)
        {
            GEOSCoordSeq_destroy_r(ctx, seq);
            return nullptr;
        }
        return GEOSGeom_createPolygon_r(ctx, shell, nullptr, 0);
    }

    std::vector<Vec2> geos_ring_to_vec(GEOSContextHandle_t ctx, const GEOSGeometry *ring)
    {
        std::vector<Vec2> out;
        const GEOSCoordSequence *seq = GEOSGeom_getCoordSeq_r(ctx, ring);
        if (!seq)
        {
            return out;
        }
        unsigned int size = 0;
        GEOSCoordSeq_getSize_r(ctx, seq, &size);
        out.reserve(size);
        for (unsigned int i = 0; i < size; ++i)
        {
            double x = 0.0;
            double y = 0.0;
            GEOSCoordSeq_getX_r(ctx, seq, i, &x);
            GEOSCoordSeq_getY_r(ctx, seq, i, &y);
            out.push_back(Vec2{x, y});
        }
        return lint_ring_points(out);
    }

    void append_geos_polygon_rings(GEOSContextHandle_t ctx,
                                   const GEOSGeometry *poly,
                                   std::vector<std::vector<std::vector<Vec2>>> &out)
    {
        if (!poly)
        {
            return;
        }
        std::vector<std::vector<Vec2>> rings;
        const GEOSGeometry *shell = GEOSGetExteriorRing_r(ctx, poly);
        if (shell)
        {
            rings.push_back(geos_ring_to_vec(ctx, shell));
        }
        const int hole_count = GEOSGetNumInteriorRings_r(ctx, poly);
        for (int i = 0; i < hole_count; ++i)
        {
            const GEOSGeometry *hole = GEOSGetInteriorRingN_r(ctx, poly, i);
            if (hole)
            {
                rings.push_back(geos_ring_to_vec(ctx, hole));
            }
        }
        if (!rings.empty() && rings.front().size() >= 4)
        {
            out.push_back(std::move(rings));
        }
    }
#endif

    const char *lot_type_key(CityModel::LotType type)
    {
        switch (type)
        {
        case CityModel::LotType::None:
            return "none";
        case CityModel::LotType::Residential:
            return "residential";
        case CityModel::LotType::RowhomeCompact:
            return "rowhome_compact";
        case CityModel::LotType::RetailStrip:
            return "retail_strip";
        case CityModel::LotType::MixedUse:
            return "mixed_use";
        case CityModel::LotType::LogisticsIndustrial:
            return "logistics_industrial";
        case CityModel::LotType::CivicCultural:
            return "civic_cultural";
        case CityModel::LotType::LuxuryScenic:
            return "luxury_scenic";
        case CityModel::LotType::BufferStrip:
            return "buffer_strip";
        default:
            return "unknown";
        }
    }

    const char *district_type_key(CityModel::DistrictType type)
    {
        switch (type)
        {
        case CityModel::DistrictType::Mixed:
            return "mixed";
        case CityModel::DistrictType::Residential:
            return "residential";
        case CityModel::DistrictType::Commercial:
            return "commercial";
        case CityModel::DistrictType::Civic:
            return "civic";
        case CityModel::DistrictType::Industrial:
            return "industrial";
        default:
            return "mixed";
        }
    }

    const char *building_type_key(CityModel::BuildingType type)
    {
        switch (type)
        {
        case CityModel::BuildingType::None:
            return "none";
        case CityModel::BuildingType::Residential:
            return "residential";
        case CityModel::BuildingType::Rowhome:
            return "rowhome";
        case CityModel::BuildingType::Retail:
            return "retail";
        case CityModel::BuildingType::MixedUse:
            return "mixed_use";
        case CityModel::BuildingType::Industrial:
            return "industrial";
        case CityModel::BuildingType::Civic:
            return "civic";
        case CityModel::BuildingType::Luxury:
            return "luxury";
        case CityModel::BuildingType::Utility:
            return "utility";
        default:
            return "none";
        }
    }

    std::string district_name(const CityModel::District &district)
    {
        std::ostringstream ss;
        ss << "A" << district.primary_axiom_id;
        if (district.secondary_axiom_id >= 0)
        {
            ss << "+A" << district.secondary_axiom_id;
        }
        return ss.str();
    }

    uint64_t hash_combine(uint64_t seed, uint64_t value)
    {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }

    std::string building_key(uint32_t district_id, uint32_t lot_id, uint32_t idx)
    {
        uint64_t h = 0;
        h = hash_combine(h, district_id);
        h = hash_combine(h, lot_id);
        h = hash_combine(h, idx);
        static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::string out(4, '0');
        for (int i = 3; i >= 0; --i)
        {
            out[i] = digits[h % 36];
            h /= 36;
        }
        return out;
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

    struct NearestRoad
    {
        bool found{false};
        CityModel::RoadType type{CityModel::RoadType::Street};
        uint32_t road_id{0};
        int endpoint_index{0};
        double distance{1e9};
    };

    NearestRoad find_nearest_road(const CityModel::City &city, const Vec2 &pos, bool want_major)
    {
        NearestRoad best;
        for (auto type : CityModel::generated_road_order)
        {
            const bool is_major = CityModel::is_major_group(type);
            if (is_major != want_major)
            {
                continue;
            }
            const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
            for (const auto &road : segments)
            {
                if (road.points.size() < 2)
                {
                    continue;
                }
                double best_dist = 1e9;
                for (std::size_t i = 0; i + 1 < road.points.size(); ++i)
                {
                    best_dist = std::min(best_dist, distance_to_segment(pos, road.points[i], road.points[i + 1]));
                }
                if (best_dist < best.distance)
                {
                    best.found = true;
                    best.distance = best_dist;
                    best.type = type;
                    best.road_id = road.id;
                    double d0 = CityModel::distance(pos, road.points.front());
                    double d1 = CityModel::distance(pos, road.points.back());
                    best.endpoint_index = (d1 < d0) ? static_cast<int>(road.points.size() - 1) : 0;
                }
            }
        }
        return best;
    }

    std::string lot_token_to_json(const CityModel::City &city, const CityModel::LotToken &lot)
    {
        std::ostringstream ss;
        NearestRoad major = find_nearest_road(city, lot.centroid, true);
        NearestRoad minor = find_nearest_road(city, lot.centroid, false);
        std::string building = building_key(lot.district_id, lot.id, 0);
        std::string district_label = "-";
        if (lot.district_id > 0 && lot.district_id <= city.districts.size())
        {
            district_label = district_name(city.districts[lot.district_id - 1]);
        }

        ss << "{\"id\":" << lot.id
           << ",\"district_id\":" << lot.district_id
           << ",\"district_name\":\"" << district_label << "\""
           << ",\"centroid\":" << vec2_to_json(lot.centroid)
           << ",\"lot_type\":\"" << lot_type_key(lot.lot_type) << "\""
           << ",\"primary_road\":\"" << CityModel::road_type_key(lot.primary_road) << "\""
           << ",\"secondary_road\":\"" << CityModel::road_type_key(lot.secondary_road) << "\""
           << ",\"access\":" << lot.access
           << ",\"exposure\":" << lot.exposure
           << ",\"serviceability\":" << lot.serviceability
           << ",\"privacy\":" << lot.privacy
           << ",\"building_key\":\"" << building << "\"";
        if (major.found)
        {
            ss << ",\"nearest_major\":{"
               << "\"road_type\":\"" << CityModel::road_type_key(major.type) << "\","
               << "\"road_id\":" << major.road_id << ","
               << "\"endpoint_index\":" << major.endpoint_index << "}";
        }
        else
        {
            ss << ",\"nearest_major\":null";
        }
        if (minor.found)
        {
            ss << ",\"nearest_minor\":{"
               << "\"road_type\":\"" << CityModel::road_type_key(minor.type) << "\","
               << "\"road_id\":" << minor.road_id << ","
               << "\"endpoint_index\":" << minor.endpoint_index << "}";
        }
        else
        {
            ss << ",\"nearest_minor\":null";
        }
        ss << "}";
        return ss.str();
    }

    std::string building_site_to_json(const CityModel::BuildingSite &site)
    {
        std::ostringstream ss;
        ss << "{\"id\":" << site.id
           << ",\"lot_id\":" << site.lot_id
           << ",\"district_id\":" << site.district_id
           << ",\"position\":" << vec2_to_json(site.position)
           << ",\"type\":\"" << building_type_key(site.type) << "\"}";
        return ss.str();
    }

} // namespace

bool export_city_to_json(const CityModel::City &city, const std::string &path)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    std::ofstream out(path);
    if (!out.is_open())
    {
        return false;
    }

    out << "{";
    out << "\"version\":2,";
    out << "\"bounds\":{"
        << "\"min\":" << vec2_to_json(city.bounds.min) << ","
        << "\"max\":" << vec2_to_json(city.bounds.max) << "},";

    // Blender addon compatibility: emit polygon features for block/zones
    std::vector<std::vector<std::vector<Vec2>>> export_rings;
    export_rings.reserve(city.block_polygons.size());
    for (const auto &poly : city.block_polygons)
    {
        std::vector<std::vector<Vec2>> rings;
        auto outer = lint_ring_points(poly.outer);
        if (outer.size() >= 4)
        {
            rings.push_back(std::move(outer));
            for (const auto &hole : poly.holes)
            {
                auto hole_ring = lint_ring_points(hole);
                if (hole_ring.size() >= 4)
                {
                    rings.push_back(std::move(hole_ring));
                }
            }
            export_rings.push_back(std::move(rings));
        }
    }

    if (export_rings.empty())
    {
        export_rings.reserve(city.districts.size());
        for (const auto &district : city.districts)
        {
            auto ring = lint_ring_points(district.border);
            if (ring.size() >= 4)
            {
                export_rings.push_back({std::move(ring)});
            }
        }
    }

    out << "\"features\":[";
    for (std::size_t i = 0; i < export_rings.size(); ++i)
    {
        out << "{\"feature_id\":\"zones\",\"object_id\":\"block_" << i
            << "\",\"geom_type\":\"POLYGON\",\"coords\":"
            << polygon_rings_to_json(export_rings[i])
            << ",\"meta\":{}}";
        if (i + 1 < export_rings.size())
        {
            out << ",";
        }
    }
    out << "],";

    out << "\"water\":[";
    for (std::size_t i = 0; i < city.water.size(); ++i)
    {
        out << polyline_to_json(city.water[i]);
        if (i + 1 < city.water.size())
        {
            out << ",";
        }
    }
    out << "],";

    auto write_roads = [&](const std::vector<Polyline> &roads)
    {
        for (std::size_t i = 0; i < roads.size(); ++i)
        {
            out << polyline_to_json(roads[i]);
            if (i + 1 < roads.size())
            {
                out << ",";
            }
        }
    };

    out << "\"roads_by_type\":{";
    bool first = true;
    for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
    {
        auto type = static_cast<CityModel::RoadType>(i);
        if (!first)
        {
            out << ",";
        }
        first = false;
        out << "\"" << CityModel::road_type_key(type) << "\":[";
        write_roads(city.roads_by_type[i]);
        out << "]";
    }
    out << "},";

    auto write_segments = [&](const std::vector<Road> &roads)
    {
        for (std::size_t i = 0; i < roads.size(); ++i)
        {
            out << road_segment_to_json(roads[i]);
            if (i + 1 < roads.size())
            {
                out << ",";
            }
        }
    };

    out << "\"road_segments_by_type\":{";
    first = true;
    for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
    {
        auto type = static_cast<CityModel::RoadType>(i);
        if (!first)
        {
            out << ",";
        }
        first = false;
        out << "\"" << CityModel::road_type_key(type) << "\":[";
        write_segments(city.segment_roads_by_type[i]);
        out << "]";
    }
    out << "},";

    out << "\"districts\":[";
    for (std::size_t i = 0; i < city.districts.size(); ++i)
    {
        const auto &district = city.districts[i];
        out << "{\"id\":" << district.id
            << ",\"name\":\"" << district_name(district) << "\""
            << ",\"primary_axiom_id\":" << district.primary_axiom_id
            << ",\"secondary_axiom_id\":" << district.secondary_axiom_id
            << ",\"type\":\"" << district_type_key(district.type) << "\""
            << ",\"orientation\":" << vec2_to_json(district.orientation)
            << ",\"border\":[";
        for (std::size_t p = 0; p < district.border.size(); ++p)
        {
            out << vec2_to_json(district.border[p]);
            if (p + 1 < district.border.size())
            {
                out << ",";
            }
        }
        out << "]}";
        if (i + 1 < city.districts.size())
        {
            out << ",";
        }
    }
    out << "],";

    out << "\"lots\":[";
    for (std::size_t i = 0; i < city.lots.size(); ++i)
    {
        out << lot_token_to_json(city, city.lots[i]);
        if (i + 1 < city.lots.size())
        {
            out << ",";
        }
    }
    out << "],";

    out << "\"building_sites\":[";
    for (std::size_t i = 0; i < city.building_sites.size(); ++i)
    {
        out << building_site_to_json(city.building_sites[i]);
        if (i + 1 < city.building_sites.size())
        {
            out << ",";
        }
    }
    out << "]";

    out << "}";
    out.close();
    return true;
}
