#include "RoadGenerator.h"

#include <cmath>
#include <set>
#include <algorithm>
#include <limits>

#include "DebugLog.hpp"
#include "PolygonUtil.h"
#include "Graph.h"
#include "PolygonFinder.h"

#if RCG_USE_GEOS
#include <geos_c.h>
#endif

namespace RoadGenerator
{

    namespace
    {
#if RCG_USE_GEOS
        struct GeosWkbWriter
        {
            GEOSContextHandle_t ctx{nullptr};

            GeosWkbWriter()
            {
                ctx = GEOS_init_r();
            }

            ~GeosWkbWriter()
            {
                if (ctx)
                {
                    GEOS_finish_r(ctx);
                }
            }

            std::vector<unsigned char> make_linestring_wkb(const std::vector<CityModel::Vec2> &points) const
            {
                std::vector<unsigned char> out;
                if (!ctx || points.size() < 2)
                {
                    return out;
                }

                GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(points.size()), 2);
                if (!seq)
                {
                    return out;
                }

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                    GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), points[i].x);
                    GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), points[i].y);
                }

                GEOSGeometry *line = GEOSGeom_createLineString_r(ctx, seq);
                if (!line)
                {
                    GEOSCoordSeq_destroy_r(ctx, seq);
                    return out;
                }

                std::size_t size = 0;
                unsigned char *buf = GEOSGeomToWKB_buf_r(ctx, line, &size);
                if (buf && size > 0)
                {
                    out.assign(buf, buf + size);
                    GEOSFree_r(ctx, buf);
                }

                GEOSGeom_destroy_r(ctx, line);
                return out;
            }
        };
#endif

        StreamlineParams makeParams(const RoadTypeParams &params)
        {
            StreamlineParams p;
            p.dsep = params.dsep;
            p.dtest = params.dtest;
            p.dstep = params.dstep;
            p.dlookahead = params.dlookahead;
            p.dcirclejoin = params.dcirclejoin;
            p.joinangle = params.joinangle;
            p.pathIterations = params.pathIterations;
            p.seedTries = params.seedTries;
            p.simplifyTolerance = params.simplifyTolerance;
            p.collideEarly = params.collideEarly;
            return p;
        }

        void seedWaterIntoGrid(StreamlineGenerator &gen, const std::vector<CityModel::Polyline> &water)
        {
            for (const auto &p : water)
                gen.grid(true).addPolyline(p.points);
            for (const auto &p : water)
                gen.grid(false).addPolyline(p.points);
        }

        std::vector<std::vector<CityModel::Vec2>> generateTier(StreamlineGenerator &gen,
                                                               bool majorDirection,
                                                               int maxLines)
        {
            std::vector<std::vector<CityModel::Vec2>> lines;
            for (int i = 0; i < maxLines; ++i)
            {
                auto seed = gen.getSeed(majorDirection);
                auto line = gen.integrateStreamline(seed, majorDirection);
                if (line.size() < 2)
                    break;
                gen.grid(majorDirection).addPolyline(line);
                gen.streamlines(majorDirection).push_back(line);
                gen.allStreamlines.push_back(line);
                gen.allStreamlinesSimple.push_back(gen.simplifyStreamline(line));
                lines.push_back(gen.simplifyStreamline(line));
            }
            return lines;
        }

        double point_segment_distance_sq(const CityModel::Vec2 &p, const CityModel::Vec2 &a, const CityModel::Vec2 &b)
        {
            const double abx = b.x - a.x;
            const double aby = b.y - a.y;
            const double apx = p.x - a.x;
            const double apy = p.y - a.y;
            const double ab_len_sq = abx * abx + aby * aby;
            if (ab_len_sq <= 1e-9)
            {
                const double dx = p.x - a.x;
                const double dy = p.y - a.y;
                return dx * dx + dy * dy;
            }
            double t = (apx * abx + apy * aby) / ab_len_sq;
            t = std::clamp(t, 0.0, 1.0);
            const double cx = a.x + abx * t;
            const double cy = a.y + aby * t;
            const double dx = p.x - cx;
            const double dy = p.y - cy;
            return dx * dx + dy * dy;
        }

        double min_distance_sq_to_line(const CityModel::Vec2 &p, const std::vector<CityModel::Vec2> &line)
        {
            if (line.empty())
                return std::numeric_limits<double>::max();
            if (line.size() == 1)
            {
                const double dx = p.x - line.front().x;
                const double dy = p.y - line.front().y;
                return dx * dx + dy * dy;
            }
            double min_sq = std::numeric_limits<double>::max();
            for (size_t i = 0; i + 1 < line.size(); ++i)
            {
                min_sq = std::min(min_sq, point_segment_distance_sq(p, line[i], line[i + 1]));
            }
            return min_sq;
        }

        bool is_line_too_close(const std::vector<CityModel::Vec2> &candidate,
                               const std::vector<std::vector<CityModel::Vec2>> &existing,
                               double min_distance)
        {
            if (existing.empty() || candidate.size() < 2)
                return false;
            const double min_dist_sq = min_distance * min_distance;
            size_t step = std::max<size_t>(1, candidate.size() / 16);
            size_t sample_count = 0;
            size_t close_count = 0;
            for (size_t i = 0; i < candidate.size(); i += step)
            {
                const CityModel::Vec2 &pt = candidate[i];
                double best_sq = std::numeric_limits<double>::max();
                for (const auto &line : existing)
                {
                    best_sq = std::min(best_sq, min_distance_sq_to_line(pt, line));
                    if (best_sq <= min_dist_sq)
                    {
                        break;
                    }
                }
                ++sample_count;
                if (best_sq <= min_dist_sq)
                {
                    ++close_count;
                }
            }
            if (sample_count == 0)
                return false;
            const double close_ratio = static_cast<double>(close_count) / static_cast<double>(sample_count);
            return close_ratio >= 0.6;
        }

        std::vector<std::vector<CityModel::Vec2>> filter_lines_by_proximity(
            const std::vector<std::vector<CityModel::Vec2>> &candidates,
            const std::vector<std::vector<CityModel::Vec2>> &existing,
            double min_distance)
        {
            std::vector<std::vector<CityModel::Vec2>> filtered;
            std::vector<std::vector<CityModel::Vec2>> accepted = existing;
            for (const auto &line : candidates)
            {
                if (is_line_too_close(line, accepted, min_distance))
                {
                    continue;
                }
                filtered.push_back(line);
                accepted.push_back(line);
            }
            return filtered;
        }

        uint32_t append_polylines(const std::vector<std::vector<CityModel::Vec2>> &lines,
                                  uint32_t limit,
                                  std::vector<CityModel::Polyline> &out
#if RCG_USE_GEOS
                                  ,
                                  const GeosWkbWriter *geos_writer
#endif
        )
        {
            uint32_t added = 0;
            for (const auto &line : lines)
            {
                if (line.size() < 2)
                {
                    continue;
                }
                if (limit > 0 && added >= limit)
                {
                    break;
                }
                CityModel::Polyline poly;
                poly.points = line;
#if RCG_USE_GEOS
                if (geos_writer)
                {
                    poly.geos_wkb = geos_writer->make_linestring_wkb(line);
                }
#endif
                out.push_back(std::move(poly));
                added++;
            }
            return added;
        }

        uint32_t append_segment_roads(const std::vector<std::vector<CityModel::Vec2>> &lines,
                                      double dstep,
                                      CityModel::RoadType type,
                                      bool deleteDangling,
                                      uint32_t limit,
                                      uint32_t &next_id,
                                      std::vector<CityModel::Road> &out
#if RCG_USE_GEOS
                                      ,
                                      const GeosWkbWriter *geos_writer
#endif
        )
        {
            Graph graph(lines, dstep, deleteDangling);
            std::set<std::pair<int, int>> edges;
            uint32_t added = 0;
            for (size_t i = 0; i < graph.nodes.size(); ++i)
            {
                for (int adj : graph.nodes[i].adj)
                {
                    int a = static_cast<int>(std::min(i, static_cast<size_t>(adj)));
                    int b = static_cast<int>(std::max(i, static_cast<size_t>(adj)));
                    if (!edges.insert({a, b}).second)
                    {
                        continue;
                    }
                    if (limit > 0 && added >= limit)
                    {
                        return added;
                    }
                    CityModel::Road road;
                    road.points = {graph.nodes[a].value, graph.nodes[b].value};
#if RCG_USE_GEOS
                    if (geos_writer)
                    {
                        road.geos_wkb = geos_writer->make_linestring_wkb(road.points);
                    }
#endif
                    road.type = type;
                    road.id = next_id++;
                    road.is_user_created = CityModel::is_user_road_type(type);
                    out.push_back(std::move(road));
                    added++;
                }
            }
            return added;
        }

        struct SegmentEdge
        {
            CityModel::Vec2 a;
            CityModel::Vec2 b;
            CityModel::RoadType type;
        };

        struct NodeInfo
        {
            CityModel::Vec2 pos;
            std::vector<int> edges;
            uint32_t typeMask{0};
        };

        int add_or_get_node(std::vector<NodeInfo> &nodes, const CityModel::Vec2 &p, double merge_radius_sq)
        {
            for (std::size_t i = 0; i < nodes.size(); ++i)
            {
                if (p.distanceToSquared(nodes[i].pos) <= merge_radius_sq)
                {
                    return static_cast<int>(i);
                }
            }
            nodes.push_back(NodeInfo{p, {}, 0});
            return static_cast<int>(nodes.size() - 1);
        }

        void build_nodes(const std::vector<SegmentEdge> &edges,
                         std::vector<NodeInfo> &nodes,
                         std::vector<std::pair<int, int>> &edge_nodes,
                         double merge_radius)
        {
            nodes.clear();
            edge_nodes.clear();
            double merge_radius_sq = merge_radius * merge_radius;
            nodes.reserve(edges.size() * 2);
            edge_nodes.reserve(edges.size());

            for (const auto &edge : edges)
            {
                int a = add_or_get_node(nodes, edge.a, merge_radius_sq);
                int b = add_or_get_node(nodes, edge.b, merge_radius_sq);
                edge_nodes.push_back({a, b});
            }

            for (std::size_t i = 0; i < edges.size(); ++i)
            {
                const auto &edge = edges[i];
                const auto &ends = edge_nodes[i];
                nodes[ends.first].edges.push_back(static_cast<int>(i));
                nodes[ends.second].edges.push_back(static_cast<int>(i));
                nodes[ends.first].typeMask |= CityModel::road_type_bit(edge.type);
                nodes[ends.second].typeMask |= CityModel::road_type_bit(edge.type);
            }
        }

        void apply_graph_rules(CityModel::City &city, const CityParams &params
#if RCG_USE_GEOS
                               ,
                               const GeosWkbWriter *geos_writer
#endif
        )
        {
            std::vector<SegmentEdge> edges;
            for (auto type : CityModel::generated_road_order)
            {
                const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
                edges.reserve(edges.size() + segments.size());
                for (const auto &seg : segments)
                {
                    if (seg.points.size() >= 2)
                    {
                        edges.push_back({seg.points.front(), seg.points.back(), type});
                    }
                }
            }

            if (edges.empty())
            {
                return;
            }

            const double merge_radius = 1.0;
            std::vector<NodeInfo> nodes;
            std::vector<std::pair<int, int>> edge_nodes;

            auto rebuild_node_cache = [&]()
            {
                build_nodes(edges, nodes, edge_nodes, merge_radius);
            };

            rebuild_node_cache();

            std::vector<SegmentEdge> filtered;
            filtered.reserve(edges.size());
            for (std::size_t i = 0; i < edges.size(); ++i)
            {
                const auto &edge = edges[i];
                const auto &tp = params.road_type_params[CityModel::road_type_index(edge.type)];
                if (!tp.enabled)
                {
                    continue;
                }

                double len = edge.a.distanceTo(edge.b);
                if (tp.minEdgeLength > 0.0 && len < tp.minEdgeLength)
                {
                    continue;
                }
                if (tp.maxEdgeLength > 0.0 && len > tp.maxEdgeLength)
                {
                    continue;
                }

                const auto ends = edge_nodes[i];
                uint32_t allow = tp.allowIntersectionsMask | CityModel::road_type_bit(edge.type);
                if ((nodes[ends.first].typeMask & ~allow) != 0u)
                {
                    continue;
                }
                if ((nodes[ends.second].typeMask & ~allow) != 0u)
                {
                    continue;
                }

                filtered.push_back(edge);
            }

            edges.swap(filtered);
            if (edges.empty())
            {
                for (auto type : CityModel::generated_road_order)
                {
                    city.segment_roads_by_type[CityModel::road_type_index(type)].clear();
                }
                return;
            }

            bool changed = true;
            while (changed)
            {
                changed = false;
                rebuild_node_cache();

                std::vector<SegmentEdge> temp;
                temp.reserve(edges.size());
                for (std::size_t i = 0; i < edges.size(); ++i)
                {
                    const auto &edge = edges[i];
                    const auto &tp = params.road_type_params[CityModel::road_type_index(edge.type)];
                    const auto ends = edge_nodes[i];

                    const std::size_t deg_a = nodes[ends.first].edges.size();
                    const std::size_t deg_b = nodes[ends.second].edges.size();
                    const bool is_dead_end = (deg_a <= 1 || deg_b <= 1);

                    if (!tp.allowDeadEnds && is_dead_end)
                    {
                        changed = true;
                        continue;
                    }
                    if (tp.requireDeadEnd && !is_dead_end)
                    {
                        changed = true;
                        continue;
                    }

                    temp.push_back(edge);
                }

                edges.swap(temp);
                if (edges.empty())
                {
                    break;
                }
            }

            for (auto type : CityModel::generated_road_order)
            {
                city.segment_roads_by_type[CityModel::road_type_index(type)].clear();
            }

            uint32_t next_id = 1;
            for (const auto &edge : edges)
            {
                CityModel::Road road;
                road.points = {edge.a, edge.b};
#if RCG_USE_GEOS
                if (geos_writer)
                {
                    road.geos_wkb = geos_writer->make_linestring_wkb(road.points);
                }
#endif
                road.type = edge.type;
                road.id = next_id++;
                road.is_user_created = CityModel::is_user_road_type(edge.type);
                city.segment_roads_by_type[CityModel::road_type_index(edge.type)].push_back(std::move(road));
            }
        }

    } // namespace

    void generate_roads(const CityParams &params,
                        const TensorField::TensorField &field,
                        const std::vector<CityModel::Polyline> &water,
                        CityModel::City &outCity)
    {
        RCG::DebugLog::printf("[RoadGen] start width=%.1f height=%.1f maxMajor=%u maxTotal=%u mode=%s water=%zu\n",
                              params.width,
                              params.height,
                              params.maxMajorRoads,
                              params.maxTotalRoads,
                              (params.roadDefinitionMode == RoadDefinitionMode::BySegment) ? "segment" : "polyline",
                              water.size());
        CityModel::Vec2 origin{0, 0};
        CityModel::Vec2 dims{params.width, params.height};

        const uint32_t maxMajor = params.maxMajorRoads;
        const uint32_t maxTotal = std::max(params.maxTotalRoads, params.maxMajorRoads);
        uint32_t next_segment_id = 1;
        uint32_t major_count = 0;
        uint32_t total_count = 0;
        const bool use_segments = params.roadDefinitionMode == RoadDefinitionMode::BySegment;

        std::vector<GridStorage> majorGrids;
        std::vector<GridStorage> minorGrids;
        std::vector<std::vector<CityModel::Vec2>> accepted_lines;

#if RCG_USE_GEOS
        GeosWkbWriter geos_writer;
#endif

        int pass_index = 0;
        for (CityModel::RoadType type : CityModel::generated_road_order)
        {
            const auto &typeParams = params.road_type_params[CityModel::road_type_index(type)];
            if (!typeParams.enabled)
            {
                ++pass_index;
                continue;
            }

            StreamlineParams sp = makeParams(typeParams);
            RK4Integrator integrator(field, sp.dstep);
            StreamlineGenerator gen(integrator, origin, dims, sp, CityModel::RNG(params.seed + 10 + pass_index * 17));

            for (const auto &g : majorGrids)
                gen.grid(true).addAll(g);
            for (const auto &g : minorGrids)
                gen.grid(false).addAll(g);

            seedWaterIntoGrid(gen, water);

            int estimate = static_cast<int>(dims.x * dims.y / std::max(1.0, sp.dsep * sp.dsep));
            auto rawLines = generateTier(gen, typeParams.majorDirection, estimate);
            double proximity = std::max(5.0, sp.dsep * 0.35);
            auto lines = filter_lines_by_proximity(rawLines, accepted_lines, proximity);

            RCG::DebugLog::printf("[RoadGen] %s raw=%zu filtered=%zu proximity=%.2f enabled=%s\n",
                                  CityModel::road_type_label(type),
                                  rawLines.size(),
                                  lines.size(),
                                  proximity,
                                  typeParams.enabled ? "on" : "off");

            if (!lines.empty())
            {
                auto &polylines = outCity.roads_by_type[CityModel::road_type_index(type)];
                uint32_t remaining_total = (maxTotal > total_count) ? (maxTotal - total_count) : 0;
                uint32_t remaining_major = (maxMajor > major_count) ? (maxMajor - major_count) : 0;
                uint32_t limit = remaining_total;
                if (CityModel::is_major_group(type))
                {
                    limit = std::min(remaining_total, remaining_major);
                }

                if (limit == 0)
                {
                    ++pass_index;
                    majorGrids.push_back(gen.grid(true));
                    minorGrids.push_back(gen.grid(false));
                    continue;
                }

                if (limit > 0 && lines.size() > limit)
                {
                    lines.resize(limit);
                }

                uint32_t added = append_polylines(lines, limit, polylines
#if RCG_USE_GEOS
                                                  ,
                                                  geos_writer.ctx ? &geos_writer : nullptr
#endif
                );

                RCG::DebugLog::printf("[RoadGen] %s added=%u limit=%u remaining_total=%u remaining_major=%u\n",
                                      CityModel::road_type_label(type),
                                      added,
                                      limit,
                                      remaining_total,
                                      remaining_major);

                if (use_segments)
                {
                    if (CityModel::is_major_group(type))
                    {
                        major_count += added;
                    }
                    total_count += added;
                }
                else
                {
                    if (CityModel::is_major_group(type))
                    {
                        major_count += added;
                    }
                    total_count += added;
                }

                auto &segments = outCity.segment_roads_by_type[CityModel::road_type_index(type)];
                append_segment_roads(lines, sp.dstep, type, typeParams.pruneDangling, 0, next_segment_id, segments
#if RCG_USE_GEOS
                                     ,
                                     geos_writer.ctx ? &geos_writer : nullptr
#endif
                );

                accepted_lines.insert(accepted_lines.end(), lines.begin(), lines.end());
            }

            majorGrids.push_back(gen.grid(true));
            minorGrids.push_back(gen.grid(false));
            ++pass_index;
        }

        apply_graph_rules(outCity, params
#if RCG_USE_GEOS
                          ,
                          geos_writer.ctx ? &geos_writer : nullptr
#endif
        );
        RCG::DebugLog::printf("[RoadGen] done.\n");
    }

} // namespace RoadGenerator
