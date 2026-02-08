#include "BlockGeneratorGEOS.h"

#if RCG_USE_GEOS
#include <geos_c.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_set>
#include "Graph.h"
#include "PolygonUtil.h"

namespace BlockGeneratorGEOS
{

#if RCG_USE_GEOS

namespace
{
    // RAII wrapper for GEOS context
    struct GEOSContext
    {
        GEOSContextHandle_t handle{nullptr};

        GEOSContext()
        {
            handle = GEOS_init_r();
        }

        ~GEOSContext()
        {
            if (handle)
            {
                GEOS_finish_r(handle);
            }
        }

        operator GEOSContextHandle_t() const { return handle; }
    };

    // RAII wrapper for GEOS geometry
    struct GEOSGeomPtr
    {
        GEOSContextHandle_t ctx{nullptr};
        GEOSGeometry *geom{nullptr};

        GEOSGeomPtr(GEOSContextHandle_t c, GEOSGeometry *g) : ctx(c), geom(g) {}

        ~GEOSGeomPtr()
        {
            if (geom && ctx)
            {
                GEOSGeom_destroy_r(ctx, geom);
            }
        }

        GEOSGeometry *get() const { return geom; }
        GEOSGeometry *release()
        {
            auto tmp = geom;
            geom = nullptr;
            return tmp;
        }

        operator bool() const { return geom != nullptr; }
    };

    // Convert road polyline to GEOS LineString
    GEOSGeometry *polyline_to_linestring(GEOSContextHandle_t ctx, const std::vector<CityModel::Vec2> &points)
    {
        if (points.size() < 2)
        {
            return nullptr;
        }

        GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(points.size()), 2);
        if (!seq)
        {
            return nullptr;
        }

        for (std::size_t i = 0; i < points.size(); ++i)
        {
            GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), points[i].x);
            GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), points[i].y);
        }

        return GEOSGeom_createLineString_r(ctx, seq);
    }

    // Extract polygon rings from GEOS geometry
    bool extract_polygon_rings(GEOSContextHandle_t ctx,
                               const GEOSGeometry *geom,
                               std::vector<CityModel::Vec2> &outer,
                               std::vector<std::vector<CityModel::Vec2>> &holes)
    {
        if (!geom || GEOSGeomTypeId_r(ctx, geom) != GEOS_POLYGON)
        {
            return false;
        }

        // Extract exterior ring
        const GEOSGeometry *ext_ring = GEOSGetExteriorRing_r(ctx, geom);
        if (!ext_ring)
        {
            return false;
        }

        const GEOSCoordSequence *ext_seq = GEOSGeom_getCoordSeq_r(ctx, ext_ring);
        if (!ext_seq)
        {
            return false;
        }

        unsigned int ext_size = 0;
        GEOSCoordSeq_getSize_r(ctx, ext_seq, &ext_size);
        outer.clear();
        outer.reserve(ext_size);

        for (unsigned int i = 0; i < ext_size; ++i)
        {
            double x, y;
            GEOSCoordSeq_getX_r(ctx, ext_seq, i, &x);
            GEOSCoordSeq_getY_r(ctx, ext_seq, i, &y);
            outer.push_back(CityModel::Vec2{x, y});
        }

        // Extract holes
        int num_holes = GEOSGetNumInteriorRings_r(ctx, geom);
        if (num_holes < 0)
        {
            num_holes = 0;
        }

        holes.clear();
        holes.reserve(num_holes);

        for (int h = 0; h < num_holes; ++h)
        {
            const GEOSGeometry *hole_ring = GEOSGetInteriorRingN_r(ctx, geom, h);
            if (!hole_ring)
            {
                continue;
            }

            const GEOSCoordSequence *hole_seq = GEOSGeom_getCoordSeq_r(ctx, hole_ring);
            if (!hole_seq)
            {
                continue;
            }

            unsigned int hole_size = 0;
            GEOSCoordSeq_getSize_r(ctx, hole_seq, &hole_size);

            std::vector<CityModel::Vec2> hole_pts;
            hole_pts.reserve(hole_size);

            for (unsigned int i = 0; i < hole_size; ++i)
            {
                double x, y;
                GEOSCoordSeq_getX_r(ctx, hole_seq, i, &x);
                GEOSCoordSeq_getY_r(ctx, hole_seq, i, &y);
                hole_pts.push_back(CityModel::Vec2{x, y});
            }

            holes.push_back(std::move(hole_pts));
        }

        return true;
    }

    // Ensure ring is closed (first == last)
    void ensure_ring_closed(std::vector<CityModel::Vec2> &ring)
    {
        if (ring.size() >= 3)
        {
            const auto &first = ring.front();
            const auto &last = ring.back();
            const double dx = first.x - last.x;
            const double dy = first.y - last.y;
            const double dist_sq = dx * dx + dy * dy;

            if (dist_sq > 1e-10)
            {
                ring.push_back(first);
            }
        }
    }

} // anonymous namespace

#endif // RCG_USE_GEOS

void generate(const CityParams &params,
              const CityModel::City &city,
              const CityModel::UserPlacedInputs &user_inputs,
              const DistrictGenerator::DistrictField &field,
              std::vector<CityModel::BlockPolygon> &out_polygons,
              std::vector<CityModel::Polygon> *out_faces,
              BlockGenerator::Stats *out_stats,
              const BlockGenerator::Settings &settings)
{
#if RCG_USE_GEOS
    out_polygons.clear();
    if (out_faces)
    {
        out_faces->clear();
    }

    BlockGenerator::Stats stats{};

    // Collect all roads that act as block barriers
    std::vector<std::vector<CityModel::Vec2>> all_roads;
    for (std::size_t type_idx = 0; type_idx < CityModel::road_type_count; ++type_idx)
    {
        if (!params.block_barrier[type_idx])
        {
            continue;
        }

        const auto &roads = city.roads_by_type[type_idx];
        for (const auto &road : roads)
        {
            if (road.points.size() >= 2)
            {
                all_roads.push_back(road.points);
            }
        }
    }

    // Add user-placed roads
    for (const auto &user_road : user_inputs.roads)
    {
        if (user_road.points.size() >= 2)
        {
            all_roads.push_back(user_road.points);
        }
    }

    stats.road_inputs = all_roads.size();

    if (all_roads.empty())
    {
        if (out_stats)
        {
            *out_stats = stats;
        }
        printf("[BlockGeneratorGEOS] No roads to polygonize\n");
        return;
    }

    // Initialize GEOS context
    GEOSContext ctx;
    if (!ctx.handle)
    {
        printf("[BlockGeneratorGEOS] Failed to initialize GEOS context\n");
        if (out_stats)
        {
            *out_stats = stats;
        }
        return;
    }

    // Convert roads to GEOS LineStrings
    std::vector<GEOSGeometry *> linestrings;
    linestrings.reserve(all_roads.size());

    for (const auto &road : all_roads)
    {
        GEOSGeometry *ls = polyline_to_linestring(ctx, road);
        if (ls)
        {
            linestrings.push_back(ls);
        }
    }

    stats.segments = linestrings.size();

    if (linestrings.empty())
    {
        if (out_stats)
        {
            *out_stats = stats;
        }
        printf("[BlockGeneratorGEOS] No valid linestrings created\n");
        return;
    }

    // Create GeometryCollection from linestrings
    GEOSGeomPtr geom_collection(ctx, GEOSGeom_createCollection_r(
                                         ctx.handle,
                                         GEOS_GEOMETRYCOLLECTION,
                                         linestrings.data(),
                                         static_cast<unsigned int>(linestrings.size())));

    if (!geom_collection)
    {
        // Clean up linestrings manually since collection creation failed
        for (auto *ls : linestrings)
        {
            GEOSGeom_destroy_r(ctx, ls);
        }
        if (out_stats)
        {
            *out_stats = stats;
        }
        printf("[BlockGeneratorGEOS] Failed to create geometry collection\n");
        return;
    }

    // Union all linestrings to get a single noded linework
    GEOSGeomPtr unioned(ctx, GEOSUnaryUnion_r(ctx, geom_collection.get()));
    if (!unioned)
    {
        if (out_stats)
        {
            *out_stats = stats;
        }
        printf("[BlockGeneratorGEOS] Failed to union linestrings\n");
        return;
    }

    // Polygonize the noded linework
    GEOSGeometry *polygonize_result = GEOSPolygonize_r(ctx, &unioned.geom, 1);
    GEOSGeomPtr polygons(ctx, polygonize_result);

    if (!polygons)
    {
        if (out_stats)
        {
            *out_stats = stats;
        }
        printf("[BlockGeneratorGEOS] Polygonize failed\n");
        return;
    }

    // Extract polygons from the result
    int num_geoms = GEOSGetNumGeometries_r(ctx, polygons.get());
    if (num_geoms < 0)
    {
        num_geoms = 0;
    }

    stats.faces_found = num_geoms;
    printf("[BlockGeneratorGEOS] Polygonize produced %d polygons\n", num_geoms);

    // Process each polygon
    for (int i = 0; i < num_geoms; ++i)
    {
        const GEOSGeometry *poly = GEOSGetGeometryN_r(ctx, polygons.get(), i);
        if (!poly)
        {
            continue;
        }

        // Check if it's a valid polygon
        if (!GEOSisValid_r(ctx, poly))
        {
            continue;
        }

        // Extract rings
        std::vector<CityModel::Vec2> outer;
        std::vector<std::vector<CityModel::Vec2>> holes;

        if (!extract_polygon_rings(ctx, poly, outer, holes))
        {
            continue;
        }

        // Check area constraints
        double area = PolygonUtil::polygonArea(outer);
        if (area < settings.min_area || area > settings.max_area)
        {
            continue;
        }

        // Ensure outer ring is closed
        ensure_ring_closed(outer);

        // Ensure holes are closed
        for (auto &hole : holes)
        {
            ensure_ring_closed(hole);
        }

        // Determine district ID (simple centroid-based assignment)
        CityModel::Vec2 centroid = PolygonUtil::averagePoint(outer);
        uint32_t district_id = 0;

        for (const auto &district : city.districts)
        {
            if (district.border.size() >= 3)
            {
                if (PolygonUtil::insidePolygon(centroid, district.border))
                {
                    district_id = district.id;
                    break;
                }
            }
        }

        // Create block polygon
        CityModel::BlockPolygon block;
        block.outer = std::move(outer);
        block.holes = std::move(holes);
        block.district_id = district_id;

        out_polygons.push_back(std::move(block));

        // Also add to faces if requested (outer ring only, as a simple polygon)
        if (out_faces)
        {
            CityModel::Polygon face;
            face.points = block.outer;
            face.district_id = district_id;
            out_faces->push_back(std::move(face));
        }
    }

    stats.valid_blocks = out_polygons.size();
    printf("[BlockGeneratorGEOS] Generated %zu valid blocks\n", stats.valid_blocks);

    if (out_stats)
    {
        *out_stats = stats;
    }

#else
    // GEOS not available, return empty
    out_polygons.clear();
    if (out_faces)
    {
        out_faces->clear();
    }
    if (out_stats)
    {
        *out_stats = BlockGenerator::Stats{};
    }
    printf("[BlockGeneratorGEOS] GEOS not enabled at compile time\n");
#endif
}

} // namespace BlockGeneratorGEOS
