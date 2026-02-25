#include "BlockGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <filesystem>
#include <fstream>

#include "DebugLog.hpp"
#include "PolygonUtil.h"
#include "BlockGeneratorGEOS.h"
#include "DebugFlags.hpp"
#if RCG_USE_GEOS
#include <geos_c.h>
#endif

namespace
{
    using CityModel::RoadType;
    using CityModel::Vec2;

    struct RoadInput
    {
        std::vector<Vec2> points;
        RoadType type{RoadType::Street};
        bool closure{true};
#if RCG_USE_GEOS
        std::vector<unsigned char> geos_wkb;
#endif
    };

    struct Segment
    {
        Vec2 a;
        Vec2 b;
        RoadType type{RoadType::Street};
        bool closure{true};
    };

    struct Node
    {
        Vec2 pos;
        std::vector<int> adj;
    };

    struct EdgeData
    {
        bool closure{true};
        RoadType type{RoadType::Street};
    };

    // signed_area: figures out the polygon's area; sign tells which way the points go around.
    double signed_area(const std::vector<Vec2> &poly)
    {
        if (poly.size() < 3)
        {
            return 0.0;
        }
        double total = 0.0;
        for (std::size_t i = 0; i < poly.size(); ++i)
        {
            const std::size_t j = (i + 1) % poly.size();
            total += poly[i].x * poly[j].y - poly[j].x * poly[i].y;
        }
        return 0.5 * total;
    }

    // point_distance_sq: distance between two points, but without the square root (faster).
    double point_distance_sq(const Vec2 &a, const Vec2 &b)
    {
        const double dx = a.x - b.x;
        const double dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    std::vector<Vec2> sanitize_polyline(const std::vector<Vec2> &points, double eps)
    {
        std::vector<Vec2> out;
        if (points.empty())
        {
            return out;
        }

        const double eps_sq = eps * eps;
        out.reserve(points.size());
        for (const auto &p : points)
        {
            if (out.empty())
            {
                out.push_back(p);
                continue;
            }

            if (point_distance_sq(out.back(), p) <= eps_sq)
            {
                continue;
            }

            if (out.size() >= 2)
            {
                const Vec2 &a = out[out.size() - 2];
                const Vec2 &b = out[out.size() - 1];

                if (point_distance_sq(a, p) <= eps_sq)
                {
                    out.pop_back();
                    continue;
                }

                const double abx = b.x - a.x;
                const double aby = b.y - a.y;
                const double bcx = p.x - b.x;
                const double bcy = p.y - b.y;
                const double cross = abx * bcy - aby * bcx;
                const double ab_len = std::sqrt(abx * abx + aby * aby);
                const double bc_len = std::sqrt(bcx * bcx + bcy * bcy);
                const double denom = std::max(1e-6, ab_len * bc_len);
                if (std::abs(cross) / denom <= 0.01)
                {
                    out.back() = p;
                    continue;
                }
            }

            out.push_back(p);
        }

        return out;
    }

    // distance_to_segment: how far a point is from a line segment (shortest distance).
    double distance_to_segment(const Vec2 &p, const Vec2 &a, const Vec2 &b)
    {
        const double vx = b.x - a.x;
        const double vy = b.y - a.y;
        const double wx = p.x - a.x;
        const double wy = p.y - a.y;
        const double c1 = wx * vx + wy * vy;
        if (c1 <= 0.0)
        {
            return std::sqrt(point_distance_sq(p, a));
        }
        const double c2 = vx * vx + vy * vy;
        if (c2 <= c1)
        {
            return std::sqrt(point_distance_sq(p, b));
        }
        const double t = c1 / c2;
        Vec2 proj{a.x + t * vx, a.y + t * vy};
        return std::sqrt(point_distance_sq(p, proj));
    }

    Vec2 project_point_to_segment(const Vec2 &p, const Vec2 &a, const Vec2 &b)
    {
        const double vx = b.x - a.x;
        const double vy = b.y - a.y;
        const double len_sq = vx * vx + vy * vy;
        if (len_sq <= 0.0)
        {
            return a;
        }
        const double wx = p.x - a.x;
        const double wy = p.y - a.y;
        double t = (wx * vx + wy * vy) / len_sq;
        t = std::clamp(t, 0.0, 1.0);
        return Vec2{a.x + t * vx, a.y + t * vy};
    }

    std::size_t snap_endpoints_to_segments(std::vector<Segment> &segments,
                                           double tolerance,
                                           double merge_radius)
    {
        if (segments.empty())
        {
            return 0;
        }
        const double tol_sq = tolerance * tolerance;
        const double merge_sq = merge_radius * merge_radius;
        std::size_t adjusted = 0;

        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            for (Vec2 *endpoint : {&segments[i].a, &segments[i].b})
            {
                Vec2 best = *endpoint;
                double best_dist_sq = tol_sq + 1.0;
                bool found = false;

                for (std::size_t j = 0; j < segments.size(); ++j)
                {
                    if (i == j)
                    {
                        continue;
                    }
                    const double dist = distance_to_segment(*endpoint, segments[j].a, segments[j].b);
                    const double dist_sq = dist * dist;
                    if (dist_sq > tol_sq || dist_sq >= best_dist_sq)
                    {
                        continue;
                    }

                    Vec2 proj = project_point_to_segment(*endpoint, segments[j].a, segments[j].b);
                    if (point_distance_sq(proj, segments[j].a) <= merge_sq)
                    {
                        proj = segments[j].a;
                    }
                    else if (point_distance_sq(proj, segments[j].b) <= merge_sq)
                    {
                        proj = segments[j].b;
                    }

                    best = proj;
                    best_dist_sq = dist_sq;
                    found = true;
                }

                if (found && point_distance_sq(*endpoint, best) > 1e-6)
                {
                    *endpoint = best;
                    ++adjusted;
                }
            }
        }

        return adjusted;
    }

    // segment_intersection: checks if two line segments cross and returns the crossing point.
    bool segment_intersection(const Vec2 &a, const Vec2 &b,
                              const Vec2 &c, const Vec2 &d,
                              Vec2 &out)
    {
        const double s1_x = b.x - a.x;
        const double s1_y = b.y - a.y;
        const double s2_x = d.x - c.x;
        const double s2_y = d.y - c.y;

        const double denom = (-s2_x * s1_y + s1_x * s2_y);
        if (std::abs(denom) < 1e-9)
            return false;
        const double s = (-s1_y * (a.x - c.x) + s1_x * (a.y - c.y)) / denom;
        const double t = (s2_x * (a.y - c.y) - s2_y * (a.x - c.x)) / denom;
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            out.x = a.x + (t * s1_x);
            out.y = a.y + (t * s1_y);
            return true;
        }
        return false;
    }

    // point_on_segment: checks if a point lies on a segment, with a small tolerance.
    bool point_on_segment(const Vec2 &p, const Vec2 &a, const Vec2 &b, double eps)
    {
        const double cross = (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
        if (std::abs(cross) > eps)
        {
            return false;
        }
        const double dot = (p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y);
        if (dot < -eps)
        {
            return false;
        }
        const double len_sq = point_distance_sq(a, b);
        if (dot > len_sq + eps)
        {
            return false;
        }
        return true;
    }

    // add_unique_point: adds a point only if it's not already very close to one we have.
    void add_unique_point(std::vector<Vec2> &points, const Vec2 &p, double eps)
    {
        const double eps_sq = eps * eps;
        for (const auto &existing : points)
        {
            if (point_distance_sq(existing, p) <= eps_sq)
            {
                return;
            }
        }
        points.push_back(p);
    }

    // snap_point: reuse a nearby point if one already exists, otherwise add a new one.
    Vec2 snap_point(const Vec2 &p, std::vector<Vec2> &snap_points, double eps)
    {
        const double eps_sq = eps * eps;
        for (const auto &existing : snap_points)
        {
            if (point_distance_sq(existing, p) <= eps_sq)
            {
                return existing;
            }
        }
        snap_points.push_back(p);
        return p;
    }

    // snap_segment_endpoints: merge nearby endpoints so GEOS sees closed loops.
    std::size_t snap_segment_endpoints(std::vector<Segment> &segments, double eps)
    {
        std::vector<Vec2> snap_points;
        snap_points.reserve(segments.size() * 2);
        for (auto &seg : segments)
        {
            seg.a = snap_point(seg.a, snap_points, eps);
            seg.b = snap_point(seg.b, snap_points, eps);
        }
        return snap_points.size();
    }

    // make_closed_ring: ensures the loop is closed once (first point repeated at the end).
    std::vector<Vec2> make_closed_ring(std::vector<Vec2> ring)
    {
        if (ring.size() >= 2 && ring.front().equals(ring.back()))
        {
            ring.pop_back();
        }
        if (ring.size() >= 3)
        {
            ring.push_back(ring.front());
        }
        return ring;
    }

    // add_or_get_node: reuse an existing node if it's close enough; otherwise add a new one.
    int add_or_get_node(std::vector<Node> &nodes, const Vec2 &p, double merge_radius_sq)
    {
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            if (point_distance_sq(nodes[i].pos, p) <= merge_radius_sq)
            {
                return static_cast<int>(i);
            }
        }
        nodes.push_back(Node{p, {}});
        return static_cast<int>(nodes.size() - 1);
    }

    // edge_key: makes a stable ID for an edge, regardless of direction.
    uint64_t edge_key(int a, int b)
    {
        const uint32_t lo = static_cast<uint32_t>(std::min(a, b));
        const uint32_t hi = static_cast<uint32_t>(std::max(a, b));
        return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
    }

    // angle_of: gives the angle from one point to another.
    double angle_of(const Vec2 &from, const Vec2 &to)
    {
        return std::atan2(to.y - from.y, to.x - from.x);
    }

    // sort_neighbors_by_angle: orders connected neighbors around a node like a clock face.
    std::vector<int> sort_neighbors_by_angle(const Node &node, const std::vector<Node> &nodes)
    {
        std::vector<std::pair<double, int>> angles;
        angles.reserve(node.adj.size());
        for (int neighbor : node.adj)
        {
            angles.push_back({angle_of(node.pos, nodes[neighbor].pos), neighbor});
        }
        std::sort(angles.begin(), angles.end(),
                  [](const auto &a, const auto &b)
                  { return a.first < b.first; });
        std::vector<int> result;
        result.reserve(angles.size());
        for (const auto &entry : angles)
        {
            result.push_back(entry.second);
        }
        return result;
    }

    // next_edge_ccw: when walking around a face, pick the next edge by turning left.
    int next_edge_ccw(int from, int current,
                      const std::vector<std::vector<int>> &sorted_adj)
    {
        const auto &neighbors = sorted_adj[current];
        if (neighbors.empty())
        {
            return -1;
        }
        if (neighbors.size() == 1)
        {
            return neighbors[0];
        }
        int from_idx = -1;
        for (std::size_t i = 0; i < neighbors.size(); ++i)
        {
            if (neighbors[i] == from)
            {
                from_idx = static_cast<int>(i);
                break;
            }
        }
        if (from_idx == -1)
        {
            return neighbors[0];
        }
        const int next_idx = (from_idx - 1 + static_cast<int>(neighbors.size())) % static_cast<int>(neighbors.size());
        return neighbors[next_idx];
    }

    // Find near-miss T-junctions: points near segments
    void find_near_miss_points(const std::vector<Segment> &segments,
                               std::vector<Vec2> &out_points,
                               double tolerance,
                               double merge_radius)
    {
        const double eps = merge_radius * 0.1;
        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            const Vec2 &seg_a = segments[i].a;
            const Vec2 &seg_b = segments[i].b;

            for (std::size_t j = 0; j < segments.size(); ++j)
            {
                if (i == j)
                    continue;

                // Check if endpoints of segment j are near segment i (but not at its endpoints)
                for (const Vec2 *pt : {&segments[j].a, &segments[j].b})
                {
                    const double dist = distance_to_segment(*pt, seg_a, seg_b);
                    if (dist < tolerance && dist > eps)
                    {
                        // Check it's not too close to endpoints of segment i
                        const double dist_a = std::sqrt(point_distance_sq(*pt, seg_a));
                        const double dist_b = std::sqrt(point_distance_sq(*pt, seg_b));
                        if (dist_a > merge_radius && dist_b > merge_radius)
                        {
                            // Project point onto segment i
                            const double vx = seg_b.x - seg_a.x;
                            const double vy = seg_b.y - seg_a.y;
                            const double wx = pt->x - seg_a.x;
                            const double wy = pt->y - seg_a.y;
                            const double c1 = wx * vx + wy * vy;
                            const double c2 = vx * vx + vy * vy;
                            if (c1 > 0.0 && c1 < c2)
                            {
                                const double t = c1 / c2;
                                Vec2 proj{seg_a.x + t * vx, seg_a.y + t * vy};
                                add_unique_point(out_points, proj, eps);
                            }
                        }
                    }
                }
            }
        }
    }

    // Split segments at near-miss points
    std::vector<Segment> split_segments_at_points(const std::vector<Segment> &segments,
                                                  const std::vector<Vec2> &split_points,
                                                  double merge_radius)
    {
        std::vector<Segment> result;
        result.reserve(segments.size() * 2);
        const double eps = merge_radius * 0.5;

        for (const auto &seg : segments)
        {
            std::vector<std::pair<double, Vec2>> points_on_seg;
            points_on_seg.push_back({0.0, seg.a});
            points_on_seg.push_back({1.0, seg.b});

            const double vx = seg.b.x - seg.a.x;
            const double vy = seg.b.y - seg.a.y;
            const double len_sq = vx * vx + vy * vy;

            if (len_sq > 0.0)
            {
                for (const Vec2 &pt : split_points)
                {
                    if (point_on_segment(pt, seg.a, seg.b, eps))
                    {
                        const double wx = pt.x - seg.a.x;
                        const double wy = pt.y - seg.a.y;
                        const double t = (wx * vx + wy * vy) / len_sq;
                        if (t > 0.01 && t < 0.99)
                        {
                            points_on_seg.push_back({t, pt});
                        }
                    }
                }
            }

            std::sort(points_on_seg.begin(), points_on_seg.end(),
                      [](const auto &a, const auto &b)
                      { return a.first < b.first; });

            for (std::size_t i = 0; i + 1 < points_on_seg.size(); ++i)
            {
                const Vec2 &a = points_on_seg[i].second;
                const Vec2 &b = points_on_seg[i + 1].second;
                if (point_distance_sq(a, b) > eps * eps)
                {
                    result.push_back(Segment{a, b, seg.type, seg.closure});
                }
            }
        }

        return result;
    }

    // Check if a polygon touches or is near the boundary
    bool polygon_touches_bounds(const std::vector<Vec2> &poly,
                                const CityParams &params,
                                double tolerance)
    {
        const double bounds_x = params.width * 0.5;
        const double bounds_y = params.height * 0.5;

        for (const Vec2 &pt : poly)
        {
            if (std::abs(pt.x - bounds_x) < tolerance ||
                std::abs(pt.x + bounds_x) < tolerance ||
                std::abs(pt.y - bounds_y) < tolerance ||
                std::abs(pt.y + bounds_y) < tolerance)
            {
                return true;
            }
        }
        return false;
    }
#if RCG_USE_GEOS

    std::vector<Vec2> read_ring(GEOSContextHandle_t ctx, const GEOSGeometry *ring)
    {
        std::vector<Vec2> out;
        if (!ring)
        {
            return out;
        }
        const GEOSCoordSequence *coords = GEOSGeom_getCoordSeq_r(ctx, ring);
        if (!coords)
        {
            return out;
        }
        unsigned int count = 0;
        if (!GEOSCoordSeq_getSize_r(ctx, coords, &count) || count < 3)
        {
            return out;
        }
        out.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            double x = 0.0, y = 0.0;
            GEOSCoordSeq_getX_r(ctx, coords, i, &x);
            GEOSCoordSeq_getY_r(ctx, coords, i, &y);
            out.push_back(Vec2{x, y});
        }
        if (out.size() >= 2 && out.front().equals(out.back()))
        {
            out.pop_back();
        }
        return out;
    }

    GEOSGeometry *create_geos_polygon(GEOSContextHandle_t ctx, const std::vector<Vec2> &ring)
    {
        if (ring.size() < 3)
        {
            return nullptr;
        }
        GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(ring.size()), 2);
        if (!seq)
        {
            return nullptr;
        }
        for (size_t i = 0; i < ring.size(); ++i)
        {
            GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), ring[i].x);
            GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), ring[i].y);
        }
        GEOSGeometry *linear_ring = GEOSGeom_createLinearRing_r(ctx, seq);
        if (!linear_ring)
        {
            return nullptr;
        }
        return GEOSGeom_createPolygon_r(ctx, linear_ring, nullptr, 0);
    }

    bool polygonize_with_geos(const BlockGenerator::Settings &settings,
                              const DistrictGenerator::DistrictField &field,
                              const std::vector<Segment> &segments,
                              const std::vector<CityModel::District> &districts,
                              std::vector<CityModel::BlockPolygon> &out_polygons,
                              std::vector<CityModel::Polygon> *out_faces,
                              BlockGenerator::Stats &stats)
    {
        if (segments.empty())
        {
            return false;
        }

        GEOSContextHandle_t ctx = GEOS_init_r();
        if (!ctx)
        {
            return false;
        }

        // Create GEOS polygons for district boundaries for clipping
        std::unordered_map<int, GEOSGeometry *> district_poly_map;
        for (const auto &district : districts)
        {
            if (district.border.size() < 3)
            {
                continue;
            }
            std::vector<Vec2> ring = district.border;
            if (!ring.empty() && !ring.front().equals(ring.back()))
            {
                ring.push_back(ring.front());
            }
            GEOSGeometry *poly = create_geos_polygon(ctx, ring);
            if (poly)
            {
                district_poly_map[district.id] = poly;
            }
        }

        const bool verbose = settings.verbose_geos_diagnostics;
        const double snap_tolerance = std::max(1e-6, settings.merge_radius * settings.snap_tolerance_factor);
        auto snap_coord = [snap_tolerance](double v)
        {
            return std::round(v / snap_tolerance) * snap_tolerance;
        };

        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS] Stage 1: Snap-rounding %zu segments (tolerance=%.3f)\n",
                                  segments.size(), snap_tolerance);
        }

        std::vector<GEOSGeometry *> line_geoms;
        line_geoms.reserve(segments.size());
        const double min_length_sq = snap_tolerance * snap_tolerance;
        stats.input_lines = segments.size();
        std::size_t degenerate_skipped = 0;

        for (const auto &seg : segments)
        {
            Vec2 a{snap_coord(seg.a.x), snap_coord(seg.a.y)};
            Vec2 b{snap_coord(seg.b.x), snap_coord(seg.b.y)};
            if (point_distance_sq(a, b) < min_length_sq)
            {
                ++degenerate_skipped;
                continue;
            }
            GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, 2, 2);
            if (!seq)
            {
                continue;
            }
            GEOSCoordSeq_setX_r(ctx, seq, 0, a.x);
            GEOSCoordSeq_setY_r(ctx, seq, 0, a.y);
            GEOSCoordSeq_setX_r(ctx, seq, 1, b.x);
            GEOSCoordSeq_setY_r(ctx, seq, 1, b.y);
            GEOSGeometry *line = GEOSGeom_createLineString_r(ctx, seq);
            if (!line)
            {
                GEOSCoordSeq_destroy_r(ctx, seq);
                continue;
            }
            line_geoms.push_back(line);
        }

        stats.snapped_lines = line_geoms.size();
        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS]   -> Kept %zu lines, skipped %zu degenerate\n",
                                  stats.snapped_lines, degenerate_skipped);
        }

        if (line_geoms.empty())
        {
            GEOS_finish_r(ctx);
            return false;
        }

        GEOSGeometry *collection = GEOSGeom_createCollection_r(
            ctx, GEOS_MULTILINESTRING,
            line_geoms.data(),
            static_cast<unsigned int>(line_geoms.size()));
        if (!collection)
        {
            for (GEOSGeometry *line : line_geoms)
            {
                GEOSGeom_destroy_r(ctx, line);
            }
            GEOS_finish_r(ctx);
            return false;
        }

        std::size_t face_count = 0;
        stats.valid_blocks = 0;

        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS] Stage 2: Noding with GEOSUnaryUnion...\n");
        }

        GEOSGeometry *noded = GEOSUnaryUnion_r(ctx, collection);
        if (!noded)
        {
            noded = GEOSNode_r(ctx, collection);
        }

        GEOSGeometry *cleaned = nullptr;
        std::size_t healed_count = 0;
        if (noded)
        {
            if (verbose)
            {
                RCG::DebugLog::printf("[GEOS] Stage 3: Topology healing (buffer-unbuffer)...\n");
            }
            const double heal_buffer = snap_tolerance * 0.5;
            GEOSGeometry *buffered = GEOSBuffer_r(ctx, noded, heal_buffer, 8);
            if (buffered)
            {
                GEOSGeometry *unbuffered = GEOSBuffer_r(ctx, buffered, -heal_buffer, 8);
                if (unbuffered)
                {
                    GEOSGeometry *boundary = GEOSBoundary_r(ctx, unbuffered);
                    if (boundary)
                    {
                        cleaned = GEOSLineMerge_r(ctx, boundary);
                        if (cleaned)
                        {
                            const int geom_type = GEOSGeomTypeId_r(ctx, cleaned);
                            healed_count = (geom_type == GEOS_LINESTRING) ? 1 : GEOSGetNumGeometries_r(ctx, cleaned);
                        }
                        GEOSGeom_destroy_r(ctx, boundary);
                    }
                    GEOSGeom_destroy_r(ctx, unbuffered);
                }
                GEOSGeom_destroy_r(ctx, buffered);
            }
        }
        if (!cleaned && noded)
        {
            cleaned = GEOSLineMerge_r(ctx, noded);
            if (cleaned)
            {
                const int geom_type = GEOSGeomTypeId_r(ctx, cleaned);
                healed_count = (geom_type == GEOS_LINESTRING) ? 1 : GEOSGetNumGeometries_r(ctx, cleaned);
            }
        }
        stats.healed_lines = healed_count;
        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS]   -> Healed to %zu lines\n", stats.healed_lines);
        }

        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS] Stage 4: Pruning dangles (degree-1 endpoints)...\n");
        }
        GEOSGeometry *polys = nullptr;
        GEOSGeometry *dangles = nullptr;
        GEOSGeometry *cut_edges = nullptr;
        GEOSGeometry *invalid_rings = nullptr;
        GEOSGeometry *final_collection = nullptr;
        std::vector<GEOSGeometry *> pruned_lines;
        std::vector<GEOSGeometry *> removed_sample;
        std::size_t dangles_removed = 0;
        if (cleaned && !RCG::g_disable_geometry_healing)
        {
            std::unordered_map<std::string, int> endpoint_degree;
            endpoint_degree.reserve(line_geoms.size() * 2);
            auto point_key = [snap_tolerance](double x, double y)
            {
                const double sx = std::round(x / snap_tolerance) * snap_tolerance;
                const double sy = std::round(y / snap_tolerance) * snap_tolerance;
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%.6f,%.6f", sx, sy);
                return std::string(buf);
            };

            std::vector<GEOSGeometry *> all_lines;
            const int geom_type = GEOSGeomTypeId_r(ctx, cleaned);
            const int num_geoms = (geom_type == GEOS_LINESTRING) ? 1 : GEOSGetNumGeometries_r(ctx, cleaned);
            all_lines.reserve(static_cast<std::size_t>(std::max(1, num_geoms)));

            for (int i = 0; i < num_geoms; ++i)
            {
                const GEOSGeometry *line = (num_geoms == 1) ? cleaned : GEOSGetGeometryN_r(ctx, cleaned, i);
                if (!line || GEOSGeomGetNumPoints_r(ctx, line) < 2)
                {
                    continue;
                }
                double length = 0.0;
                GEOSLength_r(ctx, line, &length);
                if (length < snap_tolerance)
                {
                    continue;
                }

                GEOSGeometry *start_pt = GEOSGeomGetStartPoint_r(ctx, line);
                GEOSGeometry *end_pt = GEOSGeomGetEndPoint_r(ctx, line);

                double sx = 0.0, sy = 0.0, ex = 0.0, ey = 0.0;
                GEOSGeomGetX_r(ctx, start_pt, &sx);
                GEOSGeomGetY_r(ctx, start_pt, &sy);
                GEOSGeomGetX_r(ctx, end_pt, &ex);
                GEOSGeomGetY_r(ctx, end_pt, &ey);

                endpoint_degree[point_key(sx, sy)]++;
                endpoint_degree[point_key(ex, ey)]++;

                all_lines.push_back(GEOSGeom_clone_r(ctx, line));

                GEOSGeom_destroy_r(ctx, start_pt);
                GEOSGeom_destroy_r(ctx, end_pt);
            }

            pruned_lines.reserve(all_lines.size());
            for (auto *line : all_lines)
            {
                GEOSGeometry *start_pt = GEOSGeomGetStartPoint_r(ctx, line);
                GEOSGeometry *end_pt = GEOSGeomGetEndPoint_r(ctx, line);

                double sx = 0.0, sy = 0.0, ex = 0.0, ey = 0.0;
                GEOSGeomGetX_r(ctx, start_pt, &sx);
                GEOSGeomGetY_r(ctx, start_pt, &sy);
                GEOSGeomGetX_r(ctx, end_pt, &ex);
                GEOSGeomGetY_r(ctx, end_pt, &ey);

                const int start_deg = endpoint_degree[point_key(sx, sy)];
                const int end_deg = endpoint_degree[point_key(ex, ey)];

                GEOSGeom_destroy_r(ctx, start_pt);
                GEOSGeom_destroy_r(ctx, end_pt);

                if (start_deg >= 2 && end_deg >= 2)
                {
                    pruned_lines.push_back(line);
                }
                else
                {
                    ++dangles_removed;
                    GEOSGeom_destroy_r(ctx, line);
                }
            }
        }
        else if (cleaned && RCG::g_disable_geometry_healing)
        {
            // Minimal pipeline: keep cleaned/noded geometry as-is without pruning dangles
            final_collection = GEOSGeom_clone_r(ctx, cleaned);
            stats.pruned_lines = GEOSGetNumGeometries_r(ctx, final_collection);
            if (verbose)
            {
                RCG::DebugLog::printf("[GEOS]   -> Minimal pipeline kept %zu lines (no dangle prune)\n", stats.pruned_lines);
            }
        }
        stats.pruned_lines = pruned_lines.size();
        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS]   -> Kept %zu lines, removed %zu dangles\n",
                                  stats.pruned_lines, dangles_removed);
        }

        if (!pruned_lines.empty())
        {
            final_collection = GEOSGeom_createCollection_r(
                ctx, GEOS_MULTILINESTRING,
                pruned_lines.data(),
                static_cast<unsigned int>(pruned_lines.size()));
        }

        if (final_collection)
        {
            const int geom_count = GEOSGetNumGeometries_r(ctx, final_collection);
            RCG::DebugLog::printf("[BlockGenerator] GEOS polygonize input: geoms=%d snap=%.3f merged=yes\n",
                                  geom_count,
                                  snap_tolerance);

            // If minimal geometry pipeline is requested, skip aggressive healing/pruning steps
            if (settings.add_district_borders && RCG::g_disable_geometry_healing)
            {
                RCG::DebugLog::printf("[BlockGenerator] Minimal pipeline: skipping district border injection and other healing\n");
            }

            // Preprocess: remove degenerate (zero-length) LineStrings before polygonize
            {
                const unsigned int before_count = GEOSGetNumGeometries_r(ctx, final_collection);
                std::vector<GEOSGeometry *> kept;
                kept.reserve(before_count);

                for (unsigned int i = 0; i < before_count; ++i)
                {
                    const GEOSGeometry *g = GEOSGetGeometryN_r(ctx, final_collection, i);
                    if (!g)
                        continue;

                    // Clone geometry so we can safely build a new collection
                    GEOSGeometry *gclone = GEOSGeom_clone_r(ctx, g);
                    if (!gclone)
                        continue;

                    // Use GEOS length as the degenerate test (safer than tiny coordinate diffs)
                    double geom_len = 0.0;
                    if (!GEOSLength_r(ctx, gclone, &geom_len) || geom_len <= 0.0)
                    {
                        GEOSGeom_destroy_r(ctx, gclone);
                        continue;
                    }

                    // Keep initial set; further prefilters applied below
                    kept.push_back(gclone);
                }

                // Prefilter 1: remove tiny segments (absolute threshold)
                const double tiny_thresh = 0.001; // length units
                std::vector<GEOSGeometry *> after_tiny;
                after_tiny.reserve(kept.size());
                std::size_t tiny_removed = 0;
                for (GEOSGeometry *kg : kept)
                {
                    double len = 0.0;
                    if (!GEOSLength_r(ctx, kg, &len) || len <= 0.0 || len < tiny_thresh)
                    {
                        // record a small sample of removed geoms for inspection
                        if (removed_sample.size() < 16)
                        {
                            removed_sample.push_back(GEOSGeom_clone_r(ctx, kg));
                        }
                        GEOSGeom_destroy_r(ctx, kg);
                        ++tiny_removed;
                        continue;
                    }
                    after_tiny.push_back(kg);
                }

                // Prefilter 2: deduplicate undirected edges (A->B same as B->A)
                std::unordered_set<std::string> seen_edges;
                std::vector<GEOSGeometry *> deduped;
                deduped.reserve(after_tiny.size());
                auto make_edge_key = [&](GEOSGeometry *line) -> std::string
                {
                    GEOSGeometry *spt = GEOSGeomGetStartPoint_r(ctx, line);
                    GEOSGeometry *ept = GEOSGeomGetEndPoint_r(ctx, line);
                    double sx = 0.0, sy = 0.0, ex = 0.0, ey = 0.0;
                    GEOSGeomGetX_r(ctx, spt, &sx);
                    GEOSGeomGetY_r(ctx, spt, &sy);
                    GEOSGeomGetX_r(ctx, ept, &ex);
                    GEOSGeomGetY_r(ctx, ept, &ey);
                    GEOSGeom_destroy_r(ctx, spt);
                    GEOSGeom_destroy_r(ctx, ept);
                    // Normalize order
                    char buf[256];
                    if (sx < ex || (sx == ex && sy <= ey))
                    {
                        std::snprintf(buf, sizeof(buf), "%.6f,%.6f|%.6f,%.6f", sx, sy, ex, ey);
                    }
                    else
                    {
                        std::snprintf(buf, sizeof(buf), "%.6f,%.6f|%.6f,%.6f", ex, ey, sx, sy);
                    }
                    return std::string(buf);
                };

                std::size_t dup_removed = 0;
                for (GEOSGeometry *kg : after_tiny)
                {
                    std::string key = make_edge_key(kg);
                    if (seen_edges.find(key) != seen_edges.end())
                    {
                        // duplicate - drop
                        GEOSGeom_destroy_r(ctx, kg);
                        ++dup_removed;
                        continue;
                    }
                    seen_edges.insert(key);
                    deduped.push_back(kg);
                }

                // Rebuild collection with deduped geometries if something changed
                if (deduped.size() != before_count)
                {
                    GEOSGeometry *new_coll = GEOSGeom_createCollection_r(ctx, GEOS_MULTILINESTRING,
                                                                         deduped.empty() ? nullptr : deduped.data(),
                                                                         static_cast<unsigned int>(deduped.size()));
                    if (new_coll)
                    {
                        GEOSGeom_destroy_r(ctx, final_collection);
                        final_collection = new_coll;
                    }
                    else
                    {
                        for (GEOSGeometry *kg : deduped)
                            GEOSGeom_destroy_r(ctx, kg);
                    }
                }

                const unsigned int after_count = GEOSGetNumGeometries_r(ctx, final_collection);
                RCG::DebugLog::printf("[BlockGenerator] Preprocess: before=%u tiny_removed=%zu dup_removed=%zu after=%u\n",
                                      before_count, tiny_removed, dup_removed, after_count);

                // cleanup any cloned removed samples
                for (GEOSGeometry *rg : removed_sample)
                {
                    GEOSGeom_destroy_r(ctx, rg);
                }
                removed_sample.clear();
            }

            polys = GEOSPolygonize_full_r(ctx, final_collection, &dangles, &cut_edges, &invalid_rings);
            if (polys)
            {
                const int dangle_count = dangles ? GEOSGetNumGeometries_r(ctx, dangles) : 0;
                const int cut_count = cut_edges ? GEOSGetNumGeometries_r(ctx, cut_edges) : 0;
                const int invalid_count = invalid_rings ? GEOSGetNumGeometries_r(ctx, invalid_rings) : 0;
                RCG::DebugLog::printf("[BlockGenerator] GEOS polygonize full: dangles=%d cut_edges=%d invalid_rings=%d\n",
                                      dangle_count,
                                      cut_count,
                                      invalid_count);

                if (cut_count > 0 || invalid_count > 0)
                {
                    RCG::DebugLog::printf("[BlockGenerator][Warning] polygonize issues detected (cut_edges=%d, invalid_rings=%d)\n",
                                          cut_count,
                                          invalid_count);
                    if (RCG::DebugLog::is_enabled())
                    {
                        const std::filesystem::path alert_path = std::filesystem::path("tests") / "Debug_geos_alert.txt";
                        std::error_code alert_ec;
                        std::filesystem::create_directories(alert_path.parent_path(), alert_ec);
                        std::ofstream alert(alert_path, std::ios::app);
                        if (alert.is_open())
                        {
                            alert << "polygonize_issues cut_edges=" << cut_count
                                  << " invalid_rings=" << invalid_count << "\n";
                        }
                    }
                }

                if (RCG::DebugLog::is_enabled())
                {
                    const std::filesystem::path out_path = std::filesystem::path("tests") / "Debug_geos_polygonize.wkt";
                    std::error_code ec;
                    std::filesystem::create_directories(out_path.parent_path(), ec);
                    std::ofstream out(out_path, std::ios::app);
                    if (out.is_open())
                    {
                        auto write_geom = [&](const char *label, const GEOSGeometry *geom)
                        {
                            if (!geom)
                            {
                                return;
                            }
                            GEOSWKTWriter *writer = GEOSWKTWriter_create_r(ctx);
                            if (!writer)
                            {
                                return;
                            }
                            char *wkt = GEOSWKTWriter_write_r(ctx, writer, geom);
                            if (wkt)
                            {
                                out << "# " << label << "\n";
                                out << wkt << "\n";
                                GEOSFree_r(ctx, wkt);
                            }
                            GEOSWKTWriter_destroy_r(ctx, writer);
                        };

                        write_geom("polygonize_input", final_collection);
                        write_geom("dangles", dangles);
                        write_geom("cut_edges", cut_edges);
                        write_geom("invalid_rings", invalid_rings);
                        if (!removed_sample.empty())
                        {
                            for (std::size_t ri = 0; ri < removed_sample.size(); ++ri)
                            {
                                write_geom("removed_sample", removed_sample[ri]);
                            }
                        }
                    }
                    // cleanup any cloned removed samples
                    for (GEOSGeometry *rg : removed_sample)
                    {
                        GEOSGeom_destroy_r(ctx, rg);
                    }
                    removed_sample.clear();
                }
            }
            if (!polys)
            {
                polys = GEOSPolygonize_r(ctx, &final_collection, 1);
            }
        }

        if (verbose)
        {
            RCG::DebugLog::printf("[GEOS] Stage 5: Polygonizing...\n");
        }

        stats.invalid_polygons = 0;
        stats.repaired_polygons = 0;
        stats.skipped_polygons = 0;

        if (polys)
        {
            const int poly_count = GEOSGetNumGeometries_r(ctx, polys);
            const double eps = std::max(1.0, settings.merge_radius * 0.75);
            if (verbose)
            {
                RCG::DebugLog::printf("[GEOS]   -> Got %d candidate polygons\n", poly_count);
            }
            for (int i = 0; i < poly_count; ++i)
            {
                const GEOSGeometry *poly = GEOSGetGeometryN_r(ctx, polys, i);
                if (!poly)
                {
                    continue;
                }
                const GEOSGeometry *poly_to_use = poly;
                GEOSGeometry *valid_poly = nullptr;
                bool was_valid = GEOSisValid_r(ctx, poly);
                char *invalid_reason = nullptr;
                if (!was_valid)
                {
                    ++stats.invalid_polygons;
                    invalid_reason = GEOSisValidReason_r(ctx, poly);
                    // Attempt repair unless minimal pipeline requests no healing
                    if (!RCG::g_disable_geometry_healing)
                    {
                        valid_poly = GEOSMakeValid_r(ctx, poly);
                        if (!valid_poly || !GEOSisValid_r(ctx, valid_poly))
                        {
                            ++stats.skipped_polygons;
                            if (verbose)
                            {
                                RCG::DebugLog::printf("[GEOS]     Polygon %d invalid: %s -> repair failed, skipping\n",
                                                      i, invalid_reason ? invalid_reason : "unknown");
                            }
                            if (valid_poly)
                            {
                                GEOSGeom_destroy_r(ctx, valid_poly);
                            }
                            if (invalid_reason)
                            {
                                GEOSFree_r(ctx, invalid_reason);
                            }
                            continue;
                        }
                        ++stats.repaired_polygons;
                        if (verbose)
                        {
                            RCG::DebugLog::printf("[GEOS]     Polygon %d invalid: %s -> successfully repaired\n",
                                                  i, invalid_reason ? invalid_reason : "unknown");
                        }
                        poly_to_use = valid_poly;
                    }
                }

                const GEOSGeometry *ring = GEOSGetExteriorRing_r(ctx, poly_to_use);
                std::vector<Vec2> ring_pts = read_ring(ctx, ring);

                // Diagnostic: get GEOS area and log both GEOS area and our polygon area
                const int geos_type = GEOSGeomTypeId_r(ctx, poly_to_use);
                double geos_area = 0.0;
                GEOSArea_r(ctx, poly_to_use, &geos_area);
                RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS type=%d geos_area=%.12f ring_pts=%zu\n",
                                      i, geos_type, geos_area, ring_pts.size());

                // Dump WKT (if available) at debug level for inspection
                GEOSWKTWriter *writer = GEOSWKTWriter_create_r(ctx);
                if (writer)
                {
                    char *wkt = GEOSWKTWriter_write_r(ctx, writer, poly_to_use);
                    if (wkt)
                    {
                        RCG::DebugLog::printf("[BlockGenerator] Polygon %d WKT: %s\n", i, wkt);
                        GEOSFree_r(ctx, wkt);
                    }
                    GEOSWKTWriter_destroy_r(ctx, writer);
                }
                // Dump coordinate list for the polygon for finer inspection
                if (verbose)
                {
                    for (std::size_t pi = 0; pi < ring_pts.size(); ++pi)
                    {
                        RCG::DebugLog::printf("[BlockGenerator] Polygon %d pt %zu = %.12f, %.12f\n", i, pi, ring_pts[pi].x, ring_pts[pi].y);
                    }
                }
                std::string rejection_reason = "accepted";
                if (ring_pts.size() < 3)
                {
                    rejection_reason = "not_closed";
                    if (valid_poly)
                    {
                        GEOSGeom_destroy_r(ctx, valid_poly);
                    }
                    double poly_area = 0.0;
                    RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area=%.12f poly_area=%.12f isValid=%d reasonInvalid=%s rejection=%s\n",
                                          i, geos_area, poly_area, was_valid ? 1 : 0, (invalid_reason ? invalid_reason : ""), rejection_reason.c_str());
                    if (invalid_reason)
                    {
                        GEOSFree_r(ctx, invalid_reason);
                    }
                    continue;
                }
                double area = PolygonUtil::polygonArea(ring_pts);
                if (area < settings.min_area || area > settings.max_area)
                {
                    rejection_reason = (area < settings.min_area) ? "too_small" : "too_large";
                    RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area=%.12f poly_area=%.12f isValid=%d reasonInvalid=%s rejection=%s\n",
                                          i, geos_area, area, was_valid ? 1 : 0, (invalid_reason ? invalid_reason : ""), rejection_reason.c_str());
                    if (valid_poly)
                    {
                        GEOSGeom_destroy_r(ctx, valid_poly);
                    }
                    if (invalid_reason)
                    {
                        GEOSFree_r(ctx, invalid_reason);
                    }
                    continue;
                }

                CityModel::Polygon face;
                face.points = make_closed_ring(ring_pts);
                face.district_id = field.sample_id(PolygonUtil::averagePoint(ring_pts));
                if (face.district_id == 0)
                {
                    rejection_reason = "outside_boundary";
                    RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area=%.12f poly_area=%.12f isValid=%d reasonInvalid=%s rejection=%s\n",
                                          i, geos_area, area, was_valid ? 1 : 0, (invalid_reason ? invalid_reason : ""), rejection_reason.c_str());
                    if (valid_poly)
                    {
                        GEOSGeom_destroy_r(ctx, valid_poly);
                    }
                    if (invalid_reason)
                    {
                        GEOSFree_r(ctx, invalid_reason);
                    }
                    continue;
                }

                // Clip the polygon to the district boundary
                auto district_it = district_poly_map.find(static_cast<int>(face.district_id));
                if (district_it != district_poly_map.end())
                {
                    GEOSGeometry *clipped = GEOSIntersection_r(ctx, poly_to_use, district_it->second);
                    if (clipped)
                    {
                        const int clipped_type = GEOSGeomTypeId_r(ctx, clipped);
                        GEOSGeometry *chosen_poly = nullptr;
                        double chosen_area = 0.0;

                        if (clipped_type == GEOS_POLYGON)
                        {
                            // single polygon
                            chosen_poly = GEOSGeom_clone_r(ctx, clipped);
                            GEOSArea_r(ctx, chosen_poly, &chosen_area);
                        }
                        else if (clipped_type == GEOS_MULTIPOLYGON || clipped_type == GEOS_GEOMETRYCOLLECTION)
                        {
                            const int n = GEOSGetNumGeometries_r(ctx, clipped);
                            for (int gi = 0; gi < n; ++gi)
                            {
                                const GEOSGeometry *g = GEOSGetGeometryN_r(ctx, clipped, gi);
                                if (!g)
                                {
                                    continue;
                                }
                                if (GEOSGeomTypeId_r(ctx, g) != GEOS_POLYGON)
                                {
                                    continue;
                                }
                                double a = 0.0;
                                GEOSArea_r(ctx, g, &a);
                                if (a > chosen_area)
                                {
                                    if (chosen_poly)
                                    {
                                        GEOSGeom_destroy_r(ctx, chosen_poly);
                                    }
                                    chosen_area = a;
                                    chosen_poly = GEOSGeom_clone_r(ctx, g);
                                }
                            }
                        }

                        if (chosen_poly)
                        {
                            // Use the chosen polygon (largest) from the intersection result
                            if (valid_poly)
                            {
                                GEOSGeom_destroy_r(ctx, valid_poly);
                            }
                            // chosen_poly becomes our owned valid_poly
                            valid_poly = chosen_poly;
                            // point poly_to_use at the owned polygon for subsequent reads
                            poly_to_use = valid_poly;

                            // Update ring_pts from chosen polygon
                            const GEOSGeometry *new_ring = GEOSGetExteriorRing_r(ctx, poly_to_use);
                            ring_pts = read_ring(ctx, new_ring);
                            if (ring_pts.size() < 3)
                            {
                                rejection_reason = "clipped_invalid";
                                GEOSGeom_destroy_r(ctx, clipped);
                                if (valid_poly)
                                {
                                    GEOSGeom_destroy_r(ctx, valid_poly);
                                    valid_poly = nullptr;
                                }
                                poly_to_use = poly; // reset to original owned-by-polys
                                if (invalid_reason)
                                {
                                    GEOSFree_r(ctx, invalid_reason);
                                }
                                continue;
                            }

                            // Re-check area after clipping
                            const double new_area = PolygonUtil::polygonArea(ring_pts);
                            double geos_area_clipped = 0.0;
                            GEOSArea_r(ctx, poly_to_use, &geos_area_clipped);
                            if (new_area < settings.min_area || new_area > settings.max_area)
                            {
                                rejection_reason = (new_area < settings.min_area) ? "too_small" : "too_large";
                                RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area_after=%.12f poly_area_after=%.12f isValid=%d reasonInvalid=%s rejection=%s\n",
                                                      i, geos_area_clipped, new_area, was_valid ? 1 : 0, (invalid_reason ? invalid_reason : ""), rejection_reason.c_str());
                                GEOSGeom_destroy_r(ctx, clipped);
                                if (valid_poly)
                                {
                                    GEOSGeom_destroy_r(ctx, valid_poly);
                                    valid_poly = nullptr;
                                }
                                poly_to_use = poly; // reset to original owned-by-polys
                                if (invalid_reason)
                                {
                                    GEOSFree_r(ctx, invalid_reason);
                                }
                                continue;
                            }
                            area = new_area; // update for logging
                        }
                        else
                        {
                            // No usable polygon found in intersection
                            GEOSGeom_destroy_r(ctx, clipped);
                            rejection_reason = "clip_failed";
                            RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area=%.12f poly_area=%.12f isValid=%d reasonInvalid=%s rejection=%s\n",
                                                  i, geos_area, area, was_valid ? 1 : 0, (invalid_reason ? invalid_reason : ""), rejection_reason.c_str());
                            if (valid_poly)
                            {
                                GEOSGeom_destroy_r(ctx, valid_poly);
                            }
                            if (invalid_reason)
                            {
                                GEOSFree_r(ctx, invalid_reason);
                            }
                            continue;
                        }
                        GEOSGeom_destroy_r(ctx, clipped);
                    }
                }
                else
                {
                    // No district polygon found, skip
                    rejection_reason = "no_district_poly";
                    if (valid_poly)
                    {
                        GEOSGeom_destroy_r(ctx, valid_poly);
                    }
                    if (invalid_reason)
                    {
                        GEOSFree_r(ctx, invalid_reason);
                    }
                    continue;
                }

                if (out_faces)
                {
                    face.points = make_closed_ring(ring_pts); // update with clipped points
                    out_faces->push_back(face);
                }
                ++face_count;

                bool closable = true;
                for (std::size_t e = 0; e < ring_pts.size(); ++e)
                {
                    const Vec2 &a = ring_pts[e];
                    const Vec2 &b = ring_pts[(e + 1) % ring_pts.size()];
                    bool hit = false;
                    for (const auto &seg : segments)
                    {
                        if (!seg.closure)
                        {
                            continue;
                        }
                        const double dist_a = distance_to_segment(a, seg.a, seg.b);
                        const double dist_b = distance_to_segment(b, seg.a, seg.b);
                        Vec2 inter;
                        const bool intersects = segment_intersection(a, b, seg.a, seg.b, inter);
                        if (intersects || (dist_a < eps && dist_b < eps))
                        {
                            hit = true;
                            break;
                        }
                    }
                    if (!hit)
                    {
                        closable = false;
                        break;
                    }
                }

                if (closable)
                {
                    CityModel::BlockPolygon block;
                    block.outer = face.points;
                    block.district_id = face.district_id;

                    const int hole_count = GEOSGetNumInteriorRings_r(ctx, poly_to_use);
                    if (hole_count > 0)
                    {
                        block.holes.reserve(static_cast<std::size_t>(hole_count));
                        for (int h = 0; h < hole_count; ++h)
                        {
                            const GEOSGeometry *hole = GEOSGetInteriorRingN_r(ctx, poly_to_use, h);
                            std::vector<Vec2> hole_pts = read_ring(ctx, hole);
                            if (hole_pts.size() >= 3)
                            {
                                block.holes.push_back(make_closed_ring(std::move(hole_pts)));
                            }
                        }
                    }

                    out_polygons.push_back(std::move(block));
                    ++stats.valid_blocks;
                    // Log accepted polygon
                    RCG::DebugLog::printf("[BlockGenerator] Polygon %d GEOS_area=%.12f poly_area=%.12f isValid=%d rejection=%s\n",
                                          i, geos_area, area, was_valid ? 1 : 0, rejection_reason.c_str());
                }

                if (valid_poly)
                {
                    GEOSGeom_destroy_r(ctx, valid_poly);
                }
            }
            GEOSGeom_destroy_r(ctx, polys);
        }

        if (dangles)
        {
            GEOSGeom_destroy_r(ctx, dangles);
        }
        if (cut_edges)
        {
            GEOSGeom_destroy_r(ctx, cut_edges);
        }
        if (invalid_rings)
        {
            GEOSGeom_destroy_r(ctx, invalid_rings);
        }

        if (final_collection)
        {
            GEOSGeom_destroy_r(ctx, final_collection);
        }
        else
        {
            for (auto *line : pruned_lines)
            {
                GEOSGeom_destroy_r(ctx, line);
            }
        }
        if (cleaned)
        {
            GEOSGeom_destroy_r(ctx, cleaned);
        }
        if (noded)
        {
            GEOSGeom_destroy_r(ctx, noded);
        }
        if (collection)
        {
            GEOSGeom_destroy_r(ctx, collection);
        }
        // Clean up district polygons
        for (auto &pair : district_poly_map)
        {
            GEOSGeom_destroy_r(ctx, pair.second);
        }
        GEOS_finish_r(ctx);

        stats.faces_found = face_count;
        if (verbose || face_count == 0)
        {
            RCG::DebugLog::printf(
                "[GEOS] Summary: %zu faces, %zu blocks | Cleanup: %zu->%zu->%zu->%zu lines | Validation: %zu invalid, %zu repaired, %zu skipped\n",
                face_count, stats.valid_blocks,
                stats.input_lines, stats.snapped_lines, stats.healed_lines, stats.pruned_lines,
                stats.invalid_polygons, stats.repaired_polygons, stats.skipped_polygons);
        }
        if (face_count == 0 && stats.segments > 0)
        {
            RCG::DebugLog::printf(
                "[BlockGenerator] GEOS produced 0 faces despite %zu segments. Possible causes:\n"
                "  - No closed cycles in road network\n"
                "  - snap_tolerance=%.3f may be too large/small\n"
                "  - All polygons failed area/closure checks\n",
                stats.segments, snap_tolerance);
            if (RCG::DebugLog::is_enabled())
            {
                const std::filesystem::path alert_path = std::filesystem::path("tests") / "Debug_geos_alert.txt";
                std::error_code alert_ec;
                std::filesystem::create_directories(alert_path.parent_path(), alert_ec);
                std::ofstream alert(alert_path, std::ios::app);
                if (alert.is_open())
                {
                    alert << "zero_faces roads=" << stats.road_inputs
                          << " segments=" << stats.segments
                          << " intersections=" << stats.intersections
                          << " snap=" << snap_tolerance
                          << " minimal_pipeline=" << (RCG::g_disable_geometry_healing ? "1" : "0")
                          << "\n";
                }
            }
        }
        return face_count > 0;
    }
#endif
} // namespace

namespace BlockGenerator
{
    // Legacy implementation (original PolygonFinder-based approach)
    void generate_legacy(const CityParams &params,
                         const CityModel::City &city,
                         const CityModel::UserPlacedInputs &user_inputs,
                         const DistrictGenerator::DistrictField &field,
                         std::vector<CityModel::BlockPolygon> &out_polygons,
                         std::vector<CityModel::Polygon> *out_faces,
                         Stats *out_stats,
                         const Settings &settings)
    {
        // Goal: use roads to find enclosed areas, then keep the ones that can become blocks.
        out_polygons.clear();
        if (out_faces)
        {
            out_faces->clear();
        }
        Stats stats{};

        // Some user-placed roads hide generated ones; skip those so we don't double-count.
        std::unordered_set<int> hidden_generated;
        hidden_generated.reserve(user_inputs.roads.size());
        for (const auto &road : user_inputs.roads)
        {
            if (road.source_generated_id >= 0)
            {
                hidden_generated.insert(road.source_generated_id);
            }
        }

        // Collect all roads that should act as borders or barriers.
        std::vector<RoadInput> inputs;
        inputs.reserve(512);

        int road_id = 0;
        const bool use_segments = params.debug_use_segment_roads_for_blocks ||
                                  (params.roadDefinitionMode == RoadDefinitionMode::BySegment);
        RCG::DebugLog::printf("[BlockGenerator] Input source: %s\n",
                              use_segments ? "segments" : "polylines");
        for (auto type : CityModel::generated_road_order)
        {
            const auto type_index = CityModel::road_type_index(type);
            if (use_segments)
            {
                for (const auto &seg_road : city.segment_roads_by_type[type_index])
                {
                    if (seg_road.points.size() < 2)
                    {
                        continue;
                    }
                    if (hidden_generated.find(road_id) != hidden_generated.end())
                    {
                        road_id++;
                        continue;
                    }
                    const bool barrier = params.block_barrier[type_index];
                    const bool closure = params.block_closure[type_index];
                    if (!barrier && !closure)
                    {
                        road_id++;
                        continue;
                    }
                    RoadInput input;
                    input.points = seg_road.points;
                    input.type = type;
                    input.closure = closure;
#if RCG_USE_GEOS
                    input.geos_wkb = seg_road.geos_wkb;
#endif
                    inputs.push_back(std::move(input));
                    road_id++;
                }
            }
            else
            {
                for (const auto &poly : city.roads_by_type[type_index])
                {
                    if (hidden_generated.find(road_id) != hidden_generated.end())
                    {
                        road_id++;
                        continue;
                    }
                    // If a road isn't a barrier and doesn't close blocks, ignore it.
                    const bool barrier = params.block_barrier[type_index];
                    const bool closure = params.block_closure[type_index];
                    if (!barrier && !closure)
                    {
                        road_id++;
                        continue;
                    }
                    RoadInput input;
                    input.points = poly.points;
                    input.type = type;
                    input.closure = closure;
#if RCG_USE_GEOS
                    input.geos_wkb = poly.geos_wkb;
#endif
                    inputs.push_back(std::move(input));
                    road_id++;
                }
            }
        }

        for (const auto &road : user_inputs.roads)
        {
            if (road.points.size() < 2)
            {
                continue;
            }
            const auto type_index = CityModel::road_type_index(road.road_type);
            // Same filter for user roads.
            const bool barrier = params.block_barrier[type_index];
            const bool closure = params.block_closure[type_index];
            if (!barrier && !closure)
            {
                continue;
            }
            RoadInput input;
            input.points = road.points;
            input.type = road.road_type;
            input.closure = closure;
            inputs.push_back(std::move(input));
        }

        stats.road_inputs = inputs.size();
        if (inputs.empty())
        {
            if (out_stats)
            {
                *out_stats = stats;
            }
            return;
        }

        std::array<std::size_t, CityModel::road_type_count> input_counts{};
        for (const auto &input : inputs)
        {
            input_counts[CityModel::road_type_index(input.type)]++;
        }
        RCG::DebugLog::printf("[BlockGenerator] Road inputs by type (count, closure, barrier):\n");
        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            const auto type = static_cast<CityModel::RoadType>(i);
            RCG::DebugLog::printf(
                "  %s: count=%zu, closure=%s, barrier=%s\n",
                CityModel::road_type_label(type),
                input_counts[i],
                params.block_closure[i] ? "on" : "off",
                params.block_barrier[i] ? "on" : "off");
        }

        // Break each road into straight segments so we can intersect and trace them.
        std::vector<Segment> segments;
        segments.reserve(inputs.size() * 4);
        const double cleanup_eps = std::max(1e-3, settings.merge_radius * 0.02);
        for (const auto &road : inputs)
        {
            if (road.points.size() < 2)
            {
                continue;
            }
            auto cleaned = sanitize_polyline(road.points, cleanup_eps);
            if (cleaned.size() < 2)
            {
                continue;
            }
            for (std::size_t i = 0; i + 1 < cleaned.size(); ++i)
            {
                segments.push_back(Segment{cleaned[i], cleaned[i + 1], road.type, road.closure});
            }
        }
        stats.segments = segments.size();

        // Removed: district borders are no longer added to segments for polygonization.
        // Instead, polygons are clipped to district boundaries after polygonization.

        const double snap_eps = std::max(1.0, settings.merge_radius * 0.25);
        const std::size_t raw_endpoints = segments.size() * 2;
        const std::size_t snapped_unique = snap_segment_endpoints(segments, snap_eps);
        RCG::DebugLog::printf("[BlockGenerator] Endpoint snap: raw=%zu unique=%zu eps=%.3f\n",
                              raw_endpoints,
                              snapped_unique,
                              snap_eps);

        const std::size_t endpoint_adjusted = snap_endpoints_to_segments(segments,
                                                                         settings.near_miss_tolerance,
                                                                         settings.merge_radius);
        if (endpoint_adjusted > 0)
        {
            RCG::DebugLog::printf("[BlockGenerator] Endpoint projection: adjusted=%zu\n", endpoint_adjusted);
        }

        // Phase 1: Detect T-junctions and near-misses
        std::vector<Vec2> intersections;
        intersections.reserve(segments.size());
        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            for (std::size_t j = i + 1; j < segments.size(); ++j)
            {
                Vec2 inter;
                if (segment_intersection(segments[i].a, segments[i].b, segments[j].a, segments[j].b, inter))
                {
                    add_unique_point(intersections, inter, settings.merge_radius * 0.1);
                }
            }
        }

        // Add near-miss T-junction points
        if (settings.enable_near_miss_splitting)
        {
            find_near_miss_points(segments, intersections, settings.near_miss_tolerance, settings.merge_radius);
            // Split segments at near-miss points
            segments = split_segments_at_points(segments, intersections, settings.merge_radius);
        }

        // Remove duplicate (including reversed) segments after snapping/splitting.
        struct SegmentKey
        {
            std::uint64_t ax;
            std::uint64_t ay;
            std::uint64_t bx;
            std::uint64_t by;

            bool operator==(const SegmentKey &o) const
            {
                return ax == o.ax && ay == o.ay && bx == o.bx && by == o.by;
            }
        };

        struct SegmentKeyHash
        {
            std::size_t operator()(const SegmentKey &k) const
            {
                std::size_t h = std::hash<std::uint64_t>{}(k.ax);
                h ^= std::hash<std::uint64_t>{}(k.ay) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
                h ^= std::hash<std::uint64_t>{}(k.bx) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
                h ^= std::hash<std::uint64_t>{}(k.by) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
                return h;
            }
        };

        auto to_bits = [](double v)
        {
            std::uint64_t u = 0;
            std::memcpy(&u, &v, sizeof(double));
            return u;
        };

        auto make_key = [&](const Segment &s)
        {
            SegmentKey k1{to_bits(s.a.x), to_bits(s.a.y), to_bits(s.b.x), to_bits(s.b.y)};
            SegmentKey k2{to_bits(s.b.x), to_bits(s.b.y), to_bits(s.a.x), to_bits(s.a.y)};
            if (k2.ax < k1.ax || (k2.ax == k1.ax && (k2.ay < k1.ay || (k2.ay == k1.ay && (k2.bx < k1.bx || (k2.bx == k1.bx && k2.by < k1.by))))))
            {
                return k2;
            }
            return k1;
        };

        const std::size_t before_dedupe = segments.size();
        std::unordered_set<SegmentKey, SegmentKeyHash> seen;
        seen.reserve(segments.size() * 2);
        std::vector<Segment> unique_segments;
        unique_segments.reserve(segments.size());
        for (const auto &seg : segments)
        {
            SegmentKey key = make_key(seg);
            if (seen.insert(key).second)
            {
                unique_segments.push_back(seg);
            }
        }
        if (unique_segments.size() != segments.size())
        {
            RCG::DebugLog::printf("[BlockGenerator] Deduped segments: %zu -> %zu\n",
                                  before_dedupe, unique_segments.size());
        }
        segments = std::move(unique_segments);

        stats.intersections = intersections.size();

#if RCG_USE_GEOS
        if (polygonize_with_geos(settings, field, segments, city.districts, out_polygons, out_faces, stats))
        {
            if (out_stats)
            {
                *out_stats = stats;
            }
            return;
        }
        if (stats.segments > 0)
        {
            RCG::DebugLog::printf(
                "[BlockGenerator] GEOS polygonizer ran but produced 0 faces (roads=%zu, segments=%zu, intersections=%zu)\n",
                stats.road_inputs, stats.segments, stats.intersections);
        }
#endif

// Fallback path (non-GEOS): WARNING - does not handle holes in blocks
#ifndef RCG_USE_GEOS
        static bool warning_shown = false;
        if (!warning_shown)
        {
            RCG::DebugLog::printf("Warning: Using fallback block generation (GEOS not available). Interior holes in blocks are not supported.\n");
            warning_shown = true;
        }
#endif

        std::vector<Node> nodes;
        std::unordered_map<uint64_t, EdgeData> edge_data;
        const double merge_radius_sq = settings.merge_radius * settings.merge_radius;

        auto add_edge = [&](int a, int b, const Segment &seg)
        {
            if (a == b)
            {
                return;
            }
            // Track connections in both directions and keep closure info per edge.
            if (std::find(nodes[a].adj.begin(), nodes[a].adj.end(), b) == nodes[a].adj.end())
            {
                nodes[a].adj.push_back(b);
            }
            if (std::find(nodes[b].adj.begin(), nodes[b].adj.end(), a) == nodes[b].adj.end())
            {
                nodes[b].adj.push_back(a);
            }
            const uint64_t key = edge_key(a, b);
            auto it = edge_data.find(key);
            if (it == edge_data.end())
            {
                edge_data[key] = EdgeData{seg.closure, seg.type};
            }
            else
            {
                it->second.closure = it->second.closure && seg.closure;
            }
        };

        for (const auto &seg : segments)
        {
            // Split segments at every intersection so edges are cleanly connected.
            std::vector<Vec2> pts;
            pts.reserve(4 + intersections.size());
            pts.push_back(seg.a);
            pts.push_back(seg.b);
            for (const auto &inter : intersections)
            {
                if (point_on_segment(inter, seg.a, seg.b, 1e-6))
                {
                    add_unique_point(pts, inter, settings.merge_radius * 0.1);
                }
            }
            const Vec2 dir{seg.b.x - seg.a.x, seg.b.y - seg.a.y};
            const double len_sq = dir.x * dir.x + dir.y * dir.y;
            // Sort points along the segment so we connect them in order.
            std::sort(pts.begin(), pts.end(), [&](const Vec2 &p1, const Vec2 &p2)
                      {
                          const double t1 = (len_sq > 0.0) ? ((p1.x - seg.a.x) * dir.x + (p1.y - seg.a.y) * dir.y) : 0.0;
                          const double t2 = (len_sq > 0.0) ? ((p2.x - seg.a.x) * dir.x + (p2.y - seg.a.y) * dir.y) : 0.0;
                          return t1 < t2; });
            for (std::size_t i = 0; i + 1 < pts.size(); ++i)
            {
                int a = add_or_get_node(nodes, pts[i], merge_radius_sq);
                int b = add_or_get_node(nodes, pts[i + 1], merge_radius_sq);
                add_edge(a, b, seg);
            }
        }

        if (nodes.size() < 3)
        {
            if (out_stats)
            {
                *out_stats = stats;
            }
            return;
        }

        std::vector<std::vector<int>> sorted_adj(nodes.size());
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            // Sort neighbors around each node to make face-walking deterministic.
            sorted_adj[i] = sort_neighbors_by_angle(nodes[i], nodes);
        }

        std::unordered_set<uint64_t> used_edges;
        used_edges.reserve(edge_data.size() * 2);

        struct FaceRecord
        {
            std::vector<Vec2> poly;
            double area{0.0};
            bool closable{false};
        };

        std::vector<FaceRecord> faces;
        faces.reserve(nodes.size());

        for (std::size_t start = 0; start < nodes.size(); ++start)
        {
            for (int neighbor : sorted_adj[start])
            {
                const uint64_t start_key = (static_cast<uint64_t>(start) << 32) | static_cast<uint64_t>(neighbor);
                if (used_edges.find(start_key) != used_edges.end())
                {
                    continue;
                }

                std::vector<int> face;
                face.push_back(static_cast<int>(start));
                int prev = static_cast<int>(start);
                int curr = neighbor;
                int steps = 0;
                const int max_steps = static_cast<int>(nodes.size()) * 4;

                // Walk around the boundary by always taking the next left turn.
                while (curr != static_cast<int>(start) && steps < max_steps)
                {
                    face.push_back(curr);
                    used_edges.insert((static_cast<uint64_t>(prev) << 32) | static_cast<uint64_t>(curr));
                    int next = next_edge_ccw(prev, curr, sorted_adj);
                    if (next == -1)
                    {
                        break;
                    }
                    prev = curr;
                    curr = next;
                    ++steps;
                }

                if (curr == static_cast<int>(start) && face.size() >= 3)
                {
                    // We found a loop; convert it into a polygon and keep valid sizes.
                    used_edges.insert((static_cast<uint64_t>(prev) << 32) | static_cast<uint64_t>(curr));
                    std::vector<Vec2> poly;
                    poly.reserve(face.size());
                    for (int idx : face)
                    {
                        poly.push_back(nodes[idx].pos);
                    }

                    const double area = std::abs(signed_area(poly));
                    if (area < settings.min_area || area > settings.max_area)
                    {
                        continue;
                    }

                    // Check that every edge belongs to a “closing” road.
                    bool closable = true;
                    for (std::size_t i = 0; i < face.size(); ++i)
                    {
                        const int a = face[i];
                        const int b = face[(i + 1) % face.size()];
                        auto it = edge_data.find(edge_key(a, b));
                        if (it == edge_data.end() || !it->second.closure)
                        {
                            closable = false;
                            break;
                        }
                    }

                    FaceRecord record;
                    record.poly = std::move(poly);
                    record.area = area;
                    record.closable = closable;
                    faces.push_back(std::move(record));
                }
            }
        }

        if (!faces.empty() && settings.guard_largest_face_removal)
        {
            std::size_t max_idx = 0;
            double max_area = faces[0].area;
            for (std::size_t i = 1; i < faces.size(); ++i)
            {
                if (faces[i].area > max_area)
                {
                    max_area = faces[i].area;
                    max_idx = i;
                }
            }

            // Only remove the largest face if it touches bounds or is significantly larger
            bool should_remove = false;
            if (polygon_touches_bounds(faces[max_idx].poly, params, settings.merge_radius * 2.0))
            {
                should_remove = true;
            }
            else if (faces.size() > 1)
            {
                // Find second largest area
                double second_max = 0.0;
                for (std::size_t i = 0; i < faces.size(); ++i)
                {
                    if (i != max_idx && faces[i].area > second_max)
                    {
                        second_max = faces[i].area;
                    }
                }
                // Remove if significantly larger than next
                if (second_max > 0.0 && max_area / second_max > settings.largest_face_threshold)
                {
                    should_remove = true;
                }
            }

            if (should_remove)
            {
                faces.erase(faces.begin() + static_cast<long long>(max_idx));
            }
        }
        else if (!faces.empty() && !settings.guard_largest_face_removal)
        {
            // Old behavior: always remove largest
            std::size_t max_idx = 0;
            double max_area = faces[0].area;
            for (std::size_t i = 1; i < faces.size(); ++i)
            {
                if (faces[i].area > max_area)
                {
                    max_area = faces[i].area;
                    max_idx = i;
                }
            }
            faces.erase(faces.begin() + static_cast<long long>(max_idx));
        }

        stats.faces_found = faces.size();
        for (auto &face : faces)
        {
            // Turn each face into a recorded polygon; only some become blocks.
            CityModel::Polygon face_poly;
            face_poly.points = make_closed_ring(std::move(face.poly));
            face_poly.district_id = field.sample_id(PolygonUtil::averagePoint(face_poly.points));
            if (out_faces)
            {
                out_faces->push_back(face_poly);
            }
            if (face.closable)
            {
                CityModel::BlockPolygon block;
                block.outer = face_poly.points;
                block.district_id = face_poly.district_id;
                out_polygons.push_back(std::move(block));
                stats.valid_blocks++;
            }
        }

        if (out_stats)
        {
            *out_stats = stats;
        }
    }

    // Main dispatcher: routes to legacy or GEOS implementation based on mode
    void generate(const CityParams &params,
                  const CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs,
                  const DistrictGenerator::DistrictField &field,
                  std::vector<CityModel::BlockPolygon> &out_polygons,
                  std::vector<CityModel::Polygon> *out_faces,
                  Stats *out_stats,
                  const Settings &settings)
    {
        // Dispatcher based on block generation mode
        BlockGenerator::Mode mode = BlockGenerator::Mode::Legacy;

        // Check if params specifies a mode (default is Legacy)
        if (params.block_gen_mode == BlockGenMode::GEOSOnly)
        {
            mode = BlockGenerator::Mode::GEOSOnly;
        }

        switch (mode)
        {
        case BlockGenerator::Mode::GEOSOnly:
            // Use GEOS-only implementation
            printf("[BlockGenerator] Using GEOS-only mode\n");
            BlockGeneratorGEOS::generate(params, city, user_inputs, field,
                                         out_polygons, out_faces, out_stats, settings);
            break;

        case BlockGenerator::Mode::Legacy:
        default:
            // Use legacy implementation
            generate_legacy(params, city, user_inputs, field,
                            out_polygons, out_faces, out_stats, settings);
            break;
        }
    }

} // namespace BlockGenerator
