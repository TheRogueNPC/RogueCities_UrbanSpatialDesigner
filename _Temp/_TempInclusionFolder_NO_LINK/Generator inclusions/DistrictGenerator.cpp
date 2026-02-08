#include "DistrictGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>
#include <cstdio>

#include "DebugLog.hpp"
#include "FrontageProfiles.h"
#if RCG_USE_GEOS
#include <geos_c.h>
#endif

namespace
{
    struct LabelKey
    {
        int primary;
        int secondary;

        bool operator==(const LabelKey &other) const
        {
            return primary == other.primary && secondary == other.secondary;
        }
    };

    struct LabelKeyHash
    {
        std::size_t operator()(const LabelKey &k) const
        {
            return (static_cast<std::size_t>(k.primary) << 32) ^ static_cast<std::size_t>(k.secondary + 0x9e3779b9);
        }
    };

    double distance_to_segment(const CityModel::Vec2 &p, const CityModel::Vec2 &a, const CityModel::Vec2 &b)
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
        CityModel::Vec2 proj{a.x + t * vx, a.y + t * vy};
        return CityModel::distance(p, proj);
    }

    CityModel::RoadType nearest_road_type(const CityModel::City &city, const CityModel::Vec2 &pos)
    {
        CityModel::RoadType best_type = CityModel::RoadType::Street;
        double best_dist = 1e9;
        for (auto type : CityModel::generated_road_order)
        {
            const auto &segments = city.segment_roads_by_type[CityModel::road_type_index(type)];
            for (const auto &road : segments)
            {
                if (road.points.size() < 2)
                {
                    continue;
                }
                for (std::size_t i = 0; i + 1 < road.points.size(); ++i)
                {
                    const double d = distance_to_segment(pos, road.points[i], road.points[i + 1]);
                    if (d < best_dist)
                    {
                        best_dist = d;
                        best_type = type;
                    }
                }
            }
        }
        return best_type;
    }

    std::array<double, 5> axiom_bias_for_type(int type)
    {
        // Order: Mixed, Residential, Commercial, Civic, Industrial
        switch (type)
        {
        case 0: // Radial
            return {0.25, 0.30, 0.55, 0.35, 0.20};
        case 1: // Delta
            return {0.20, 0.45, 0.30, 0.25, 0.35};
        case 2: // Block
            return {0.20, 0.55, 0.25, 0.20, 0.30};
        case 3: // Grid Corrective
        default:
            return {0.25, 0.30, 0.45, 0.25, 0.35};
        }
    }

    // Influencer-based district type biasing (landmark seeding)
    // Order: Mixed, Residential, Commercial, Civic, Industrial
    std::array<double, 5> influencer_bias_for_type(InfluencerType influencer)
    {
        switch (influencer)
        {
        case InfluencerType::Market:
            // Markets strongly bias toward Commercial
            return {0.15, 0.10, 0.75, 0.20, 0.15};
        case InfluencerType::Keep:
            // Keeps (castles/forts) bias toward Civic
            return {0.15, 0.15, 0.20, 0.75, 0.10};
        case InfluencerType::Temple:
            // Temples bias toward Civic with some Mixed
            return {0.30, 0.15, 0.15, 0.65, 0.10};
        case InfluencerType::Harbor:
            // Harbors bias toward Industrial and Commercial
            return {0.15, 0.10, 0.45, 0.10, 0.55};
        case InfluencerType::Park:
            // Parks bias toward Residential and Luxury areas
            return {0.20, 0.65, 0.15, 0.20, 0.05};
        case InfluencerType::Gate:
            // City gates are mixed use with commerce
            return {0.45, 0.20, 0.45, 0.15, 0.15};
        case InfluencerType::Well:
            // Wells/fountains bias toward Residential
            return {0.25, 0.60, 0.20, 0.20, 0.10};
        case InfluencerType::None:
        default:
            // No influencer - neutral weights
            return {0.25, 0.25, 0.25, 0.25, 0.25};
        }
    }

    std::array<double, 5> frontage_bias_from_profile(const CityModel::RoadFrontageProfile &profile)
    {
        const double A = profile.access;
        const double E = profile.exposure;
        const double S = profile.serviceability;
        const double P = profile.privacy;
        return {
            0.25 * (A + E + S + P),                    // Mixed
            0.60 * P + 0.20 * A + 0.10 * S + 0.10 * E, // Residential
            0.60 * E + 0.20 * A + 0.10 * S + 0.10 * P, // Commercial
            0.50 * E + 0.20 * A + 0.10 * S + 0.20 * P, // Civic
            0.60 * S + 0.25 * A + 0.10 * E + 0.05 * P  // Industrial
        };
    }

    struct Edge
    {
        int x0;
        int y0;
        int x1;
        int y1;
        bool used{false};
    };

    std::vector<CityModel::Vec2> build_border_loop(const std::vector<Edge> &edges,
                                                   const CityModel::Vec2 &origin,
                                                   const CityModel::Vec2 &cell)
    {
        if (edges.empty())
        {
            return {};
        }

        std::vector<Edge> working = edges;
        std::vector<CityModel::Vec2> best_loop;
        double best_len = 0.0;

        for (auto &edge : working)
        {
            if (edge.used)
            {
                continue;
            }
            std::vector<CityModel::Vec2> loop;
            edge.used = true;
            int sx = edge.x0;
            int sy = edge.y0;
            int cx = edge.x1;
            int cy = edge.y1;
            loop.push_back({origin.x + sx * cell.x, origin.y + sy * cell.y});
            loop.push_back({origin.x + cx * cell.x, origin.y + cy * cell.y});

            bool closed = false;
            while (!closed)
            {
                bool found = false;
                for (auto &next : working)
                {
                    if (next.used)
                    {
                        continue;
                    }
                    if (next.x0 == cx && next.y0 == cy)
                    {
                        next.used = true;
                        cx = next.x1;
                        cy = next.y1;
                        loop.push_back({origin.x + cx * cell.x, origin.y + cy * cell.y});
                        found = true;
                        break;
                    }
                    if (next.x1 == cx && next.y1 == cy)
                    {
                        next.used = true;
                        cx = next.x0;
                        cy = next.y0;
                        loop.push_back({origin.x + cx * cell.x, origin.y + cy * cell.y});
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    break;
                }
                if (cx == sx && cy == sy)
                {
                    closed = true;
                }
            }

            double length = 0.0;
            for (std::size_t i = 1; i < loop.size(); ++i)
            {
                length += CityModel::distance(loop[i - 1], loop[i]);
            }
            if (length > best_len)
            {
                best_len = length;
                best_loop = std::move(loop);
            }
        }

        return best_loop;
    }

    bool vec2_near(const CityModel::Vec2 &a, const CityModel::Vec2 &b, double eps = 1e-6)
    {
        return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps;
    }

    CityModel::Vec2 find_boundary(const DistrictGenerator::DistrictField &field,
                                  const CityModel::Vec2 &a,
                                  const CityModel::Vec2 &b,
                                  double t0,
                                  double t1,
                                  uint32_t id0)
    {
        double lo = t0;
        double hi = t1;
        for (int i = 0; i < 8; ++i)
        {
            double mid = 0.5 * (lo + hi);
            CityModel::Vec2 p = CityModel::lerp(a, b, mid);
            uint32_t id = field.sample_id(p);
            if (id == id0)
            {
                lo = mid;
            }
            else
            {
                hi = mid;
            }
        }
        return CityModel::lerp(a, b, 0.5 * (lo + hi));
    }

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

    void clip_polyline_points(const std::vector<CityModel::Vec2> &points,
                              const DistrictGenerator::DistrictField &field,
                              std::vector<CityModel::Polyline> &out
#if RCG_USE_GEOS
                              ,
                              const GeosWkbWriter *geos_writer
#endif
    )
    {
        if (points.size() < 2)
        {
            return;
        }
        const double step_len = std::min(field.cell_size.x, field.cell_size.y);
        if (step_len <= 0.0)
        {
            CityModel::Polyline poly;
            poly.points = points;
#if RCG_USE_GEOS
            if (geos_writer)
            {
                poly.geos_wkb = geos_writer->make_linestring_wkb(poly.points);
            }
#endif
            out.push_back(std::move(poly));
            return;
        }

        CityModel::Polyline current;
        uint32_t current_id = field.sample_id(points.front());
        if (current_id != 0)
        {
            current.points.push_back(points.front());
        }

        auto flush_current = [&]()
        {
            if (current_id != 0 && current.points.size() > 1)
            {
#if RCG_USE_GEOS
                if (geos_writer)
                {
                    current.geos_wkb = geos_writer->make_linestring_wkb(current.points);
                }
#endif
                out.push_back(current);
            }
            current.points.clear();
        };

        for (std::size_t i = 0; i + 1 < points.size(); ++i)
        {
            const auto &p0 = points[i];
            const auto &p1 = points[i + 1];
            const double seg_len = CityModel::distance(p0, p1);
            if (seg_len <= 1e-6)
            {
                continue;
            }

            uint32_t start_id = field.sample_id(p0);
            if (start_id != current_id)
            {
                flush_current();
                current_id = start_id;
                if (current_id != 0)
                {
                    current.points.push_back(p0);
                }
            }

            int steps = std::max(1, static_cast<int>(std::ceil(seg_len / step_len)));
            double prev_t = 0.0;
            uint32_t prev_id = current_id;

            for (int s = 1; s <= steps; ++s)
            {
                double t = static_cast<double>(s) / steps;
                CityModel::Vec2 pt = CityModel::lerp(p0, p1, t);
                uint32_t id = field.sample_id(pt);
                if (id == prev_id)
                {
                    prev_t = t;
                    continue;
                }

                CityModel::Vec2 boundary = find_boundary(field, p0, p1, prev_t, t, prev_id);
                if (prev_id != 0)
                {
                    if (current.points.empty() || !vec2_near(current.points.back(), boundary))
                    {
                        current.points.push_back(boundary);
                    }
                    flush_current();
                }

                current_id = id;
                prev_id = id;
                if (current_id != 0)
                {
                    current.points.clear();
                    current.points.push_back(boundary);
                }
                prev_t = t;
            }

            if (current_id != 0)
            {
                if (current.points.empty() || !vec2_near(current.points.back(), p1))
                {
                    current.points.push_back(p1);
                }
            }
        }

        if (current_id != 0 && current.points.size() > 1)
        {
#if RCG_USE_GEOS
            if (geos_writer)
            {
                current.geos_wkb = geos_writer->make_linestring_wkb(current.points);
            }
#endif
            out.push_back(std::move(current));
        }
    }

    void clip_segment_roads(const std::vector<CityModel::Road> &roads,
                            const DistrictGenerator::DistrictField &field,
                            std::vector<CityModel::Road> &out,
                            uint32_t &next_id
#if RCG_USE_GEOS
                            ,
                            const GeosWkbWriter *geos_writer
#endif
    )
    {
        for (const auto &road : roads)
        {
            std::vector<CityModel::Polyline> clipped;
            clip_polyline_points(road.points, field, clipped
#if RCG_USE_GEOS
                                 ,
                                 geos_writer
#endif
            );
            for (const auto &poly : clipped)
            {
                CityModel::Road clipped_road;
                clipped_road.points = poly.points;
                clipped_road.type = road.type;
                clipped_road.id = next_id++;
                clipped_road.is_user_created = road.is_user_created;
#if RCG_USE_GEOS
                clipped_road.geos_wkb = poly.geos_wkb;
#endif
                out.push_back(std::move(clipped_road));
            }
        }
    }
} // namespace

namespace DistrictGenerator
{
    uint32_t DistrictField::sample_id(const CityModel::Vec2 &pos) const
    {
        if (!valid())
        {
            return 0;
        }
        const double x = (pos.x - origin.x) / cell_size.x;
        const double y = (pos.y - origin.y) / cell_size.y;
        int ix = static_cast<int>(std::floor(x));
        int iy = static_cast<int>(std::floor(y));
        if (ix < 0 || iy < 0 || ix >= width || iy >= height)
        {
            return 0;
        }
        return district_ids[static_cast<std::size_t>(iy) * width + ix];
    }

    void generate(const CityParams &params,
                  const std::vector<AxiomInput> &axioms,
                  CityModel::City &city,
                  DistrictField &out_field,
                  const Settings &settings,
                  const TensorField::TensorField *tensor_field)
    {
        (void)params;
        RCG::DebugLog::printf("[DistrictGen] start axioms=%zu grid_res=%d rd=%s\n",
                              axioms.size(),
                              settings.grid_resolution,
                              settings.use_reaction_diffusion ? "on" : "off");
        city.districts.clear();
        out_field = {};

        const CityModel::Bounds &bounds = city.bounds;
        const CityModel::Vec2 origin = bounds.min;
        const CityModel::Vec2 extent{bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y};

        if (axioms.empty())
        {
            CityModel::District district;
            district.id = 1;
            district.primary_axiom_id = -1;
            district.secondary_axiom_id = -1;
            district.type = CityModel::DistrictType::Mixed;
            district.border = {
                {bounds.min.x, bounds.min.y},
                {bounds.max.x, bounds.min.y},
                {bounds.max.x, bounds.max.y},
                {bounds.min.x, bounds.max.y},
                {bounds.min.x, bounds.min.y}};
            city.districts.push_back(district);

            out_field.width = 1;
            out_field.height = 1;
            out_field.origin = origin;
            out_field.cell_size = extent;
            out_field.district_ids = {district.id};
            return;
        }

        int grid_res = settings.grid_resolution;

        // Adaptive resolution based on extent size if enabled
        if (settings.enable_adaptive_resolution)
        {
            const double max_extent = std::max(extent.x, extent.y);
            // Aim for ~5-10 units per cell
            const int adaptive_res = static_cast<int>(max_extent / 7.5);
            grid_res = std::max(settings.min_grid_resolution,
                                std::min(settings.max_grid_resolution, adaptive_res));
        }
        else
        {
            // Original snapping behavior
            if (grid_res <= 96)
                grid_res = 64;
            else if (grid_res <= 192)
                grid_res = 128;
            else
                grid_res = 256;
        }
        out_field.width = grid_res;
        out_field.height = grid_res;
        out_field.origin = origin;
        out_field.cell_size = {extent.x / grid_res, extent.y / grid_res};
        out_field.district_ids.assign(static_cast<std::size_t>(grid_res * grid_res), 0);

        std::unordered_map<LabelKey, uint32_t, LabelKeyHash> label_map;
        std::vector<CityModel::Vec2> center_sums;
        std::vector<uint32_t> counts;

        const double weight_scale = std::max(0.1, settings.weight_scale);
        double avg_weight = 0.0;
        for (const auto &axiom : axioms)
        {
            avg_weight += (axiom.radius * axiom.radius);
        }
        if (!axioms.empty())
        {
            avg_weight = avg_weight / static_cast<double>(axioms.size());
        }

        auto axiom_weight = [&](int type)
        {
            switch (type)
            {
            case 0:
                return 1.0;
            case 1:
                return 0.95;
            case 2:
                return 0.90;
            case 3:
            default:
                return 0.75;
            }
        };

        double secondary_cutoff;
        if (settings.use_local_secondary_cutoff)
        {
            // Use fixed value instead of global average
            secondary_cutoff = settings.fixed_secondary_cutoff;
        }
        else
        {
            // Original behavior: based on global average
            secondary_cutoff = std::max(1.0, avg_weight * settings.secondary_threshold);
        }

        uint32_t next_district_id = 1;

        std::vector<int> primary_ids(grid_res * grid_res, -1);
        std::vector<int> secondary_ids(grid_res * grid_res, -1);

        for (int y = 0; y < grid_res; ++y)
        {
            for (int x = 0; x < grid_res; ++x)
            {
                CityModel::Vec2 cell_center{
                    origin.x + (x + 0.5) * out_field.cell_size.x,
                    origin.y + (y + 0.5) * out_field.cell_size.y};

                double best_score = 1e18;
                double second_score = 1e18;
                int best_id = -1;
                int second_id = -1;

                for (const auto &axiom : axioms)
                {
                    const double dx = cell_center.x - axiom.pos.x;
                    const double dy = cell_center.y - axiom.pos.y;
                    const double dist2 = dx * dx + dy * dy;
                    const double weight = axiom_weight(axiom.type) * (axiom.radius * axiom.radius) * weight_scale;
                    const double score = dist2 - weight;
                    if (score < best_score)
                    {
                        second_score = best_score;
                        second_id = best_id;
                        best_score = score;
                        best_id = axiom.id;
                    }
                    else if (score < second_score)
                    {
                        second_score = score;
                        second_id = axiom.id;
                    }
                }

                int secondary_id = ((second_score - best_score) <= secondary_cutoff) ? second_id : -1;
                const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                primary_ids[idx] = best_id;
                secondary_ids[idx] = secondary_id;
            }
        }

        const double rd_mix = std::clamp(settings.rd_mix, 0.0, 1.0);
        if (settings.use_reaction_diffusion && rd_mix > 0.01)
        {
            std::vector<double> u(grid_res * grid_res, 1.0);
            std::vector<double> v(grid_res * grid_res, 0.0);
            const double Du = 0.16;
            const double Dv = 0.08;
            const double F = 0.035;
            const double K = 0.065;

            for (const auto &axiom : axioms)
            {
                for (int y = 0; y < grid_res; ++y)
                {
                    for (int x = 0; x < grid_res; ++x)
                    {
                        const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                        CityModel::Vec2 cell_center{
                            origin.x + (x + 0.5) * out_field.cell_size.x,
                            origin.y + (y + 0.5) * out_field.cell_size.y};
                        const double dist = CityModel::distance(cell_center, axiom.pos);
                        if (dist < axiom.radius * 0.5)
                        {
                            v[idx] = 1.0;
                            u[idx] = 0.0;
                        }
                    }
                }
            }

            auto idx_of = [&](int x, int y)
            {
                x = std::max(0, std::min(grid_res - 1, x));
                y = std::max(0, std::min(grid_res - 1, y));
                return static_cast<std::size_t>(y) * grid_res + x;
            };

            const int iterations = std::max(4, static_cast<int>(6 + rd_mix * 28));
            for (int it = 0; it < iterations; ++it)
            {
                std::vector<double> u2 = u;
                std::vector<double> v2 = v;
                for (int y = 0; y < grid_res; ++y)
                {
                    for (int x = 0; x < grid_res; ++x)
                    {
                        const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                        const double lap_u =
                            u[idx_of(x - 1, y)] + u[idx_of(x + 1, y)] +
                            u[idx_of(x, y - 1)] + u[idx_of(x, y + 1)] -
                            4.0 * u[idx];
                        const double lap_v =
                            v[idx_of(x - 1, y)] + v[idx_of(x + 1, y)] +
                            v[idx_of(x, y - 1)] + v[idx_of(x, y + 1)] -
                            4.0 * v[idx];
                        const double uvv = u[idx] * v[idx] * v[idx];
                        u2[idx] = u[idx] + (Du * lap_u - uvv + F * (1.0 - u[idx]));
                        v2[idx] = v[idx] + (Dv * lap_v + uvv - (F + K) * v[idx]);
                    }
                }
                u.swap(u2);
                v.swap(v2);
            }

            const double v_threshold = 0.35 - rd_mix * 0.2;
            for (int y = 0; y < grid_res; ++y)
            {
                for (int x = 0; x < grid_res; ++x)
                {
                    const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                    if (v[idx] < v_threshold && secondary_ids[idx] >= 0)
                    {
                        std::swap(primary_ids[idx], secondary_ids[idx]);
                        secondary_ids[idx] = -1;
                    }
                }
            }
        }

        for (int y = 0; y < grid_res; ++y)
        {
            for (int x = 0; x < grid_res; ++x)
            {
                const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                CityModel::Vec2 cell_center{
                    origin.x + (x + 0.5) * out_field.cell_size.x,
                    origin.y + (y + 0.5) * out_field.cell_size.y};
                const int best_id = primary_ids[idx];
                const int secondary_id = secondary_ids[idx];
                LabelKey key{best_id, secondary_id};
                auto it = label_map.find(key);
                if (it == label_map.end())
                {
                    CityModel::District district;
                    district.id = next_district_id++;
                    district.primary_axiom_id = best_id;
                    district.secondary_axiom_id = secondary_id;
                    district.type = CityModel::DistrictType::Mixed;
                    city.districts.push_back(district);
                    label_map[key] = district.id;
                    center_sums.push_back({0.0, 0.0});
                    counts.push_back(0);
                    it = label_map.find(key);
                }

                const uint32_t district_id = it->second;
                out_field.district_ids[idx] = district_id;

                std::size_t d_index = district_id - 1;
                center_sums[d_index].x += cell_center.x;
                center_sums[d_index].y += cell_center.y;
                counts[d_index] += 1;
            }
        }

        // Split disconnected regions into separate districts
        if (settings.split_disconnected_regions)
        {
            // Connected component analysis on district_ids
            std::vector<uint32_t> new_district_ids(out_field.district_ids.size(), 0);
            std::vector<bool> visited(out_field.district_ids.size(), false);
            uint32_t next_new_id = 1;

            // Map from old district ID to list of new district IDs (for disconnected regions)
            std::unordered_map<uint32_t, std::vector<uint32_t>> district_splits;

            for (std::size_t start_idx = 0; start_idx < out_field.district_ids.size(); ++start_idx)
            {
                if (visited[start_idx] || out_field.district_ids[start_idx] == 0)
                {
                    continue;
                }

                const uint32_t original_id = out_field.district_ids[start_idx];

                // BFS to find connected component
                std::vector<std::size_t> queue;
                queue.push_back(start_idx);
                visited[start_idx] = true;

                while (!queue.empty())
                {
                    std::size_t idx = queue.back();
                    queue.pop_back();

                    new_district_ids[idx] = next_new_id;

                    const int x = static_cast<int>(idx % grid_res);
                    const int y = static_cast<int>(idx / grid_res);

                    // Check 4-connected neighbors
                    const int dx[] = {-1, 1, 0, 0};
                    const int dy[] = {0, 0, -1, 1};

                    for (int d = 0; d < 4; ++d)
                    {
                        const int nx = x + dx[d];
                        const int ny = y + dy[d];

                        if (nx >= 0 && nx < grid_res && ny >= 0 && ny < grid_res)
                        {
                            const std::size_t neighbor_idx = static_cast<std::size_t>(ny) * grid_res + nx;

                            if (!visited[neighbor_idx] &&
                                out_field.district_ids[neighbor_idx] == original_id)
                            {
                                visited[neighbor_idx] = true;
                                queue.push_back(neighbor_idx);
                            }
                        }
                    }
                }

                district_splits[original_id].push_back(next_new_id);
                ++next_new_id;
            }

            // Update district IDs
            out_field.district_ids = new_district_ids;

            // Update city.districts to reflect splits
            std::vector<CityModel::District> new_districts;
            new_districts.reserve(next_new_id - 1);

            for (const auto &pair : district_splits)
            {
                const uint32_t old_id = pair.first;
                const std::vector<uint32_t> &new_ids = pair.second;

                // Find original district
                std::size_t orig_idx = 0;
                for (std::size_t i = 0; i < city.districts.size(); ++i)
                {
                    if (city.districts[i].id == old_id)
                    {
                        orig_idx = i;
                        break;
                    }
                }

                // Create new districts for each connected component
                for (uint32_t new_id : new_ids)
                {
                    CityModel::District district = city.districts[orig_idx];
                    district.id = new_id;
                    new_districts.push_back(district);
                }
            }

            city.districts = new_districts;

            // Rebuild center_sums and counts for new districts
            center_sums.clear();
            counts.clear();
            center_sums.resize(city.districts.size(), {0.0, 0.0});
            counts.resize(city.districts.size(), 0);

            for (int y = 0; y < grid_res; ++y)
            {
                for (int x = 0; x < grid_res; ++x)
                {
                    const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                    const uint32_t district_id = out_field.district_ids[idx];

                    if (district_id > 0 && district_id <= city.districts.size())
                    {
                        CityModel::Vec2 cell_center{
                            origin.x + (x + 0.5) * out_field.cell_size.x,
                            origin.y + (y + 0.5) * out_field.cell_size.y};

                        std::size_t d_index = district_id - 1;
                        center_sums[d_index].x += cell_center.x;
                        center_sums[d_index].y += cell_center.y;
                        counts[d_index] += 1;
                    }
                }
            }
        }

        // Compute border for each district
        std::vector<std::vector<Edge>> district_edges(city.districts.size());
        for (int y = 0; y < grid_res; ++y)
        {
            for (int x = 0; x < grid_res; ++x)
            {
                const std::size_t idx = static_cast<std::size_t>(y) * grid_res + x;
                const uint32_t id = out_field.district_ids[idx];
                if (id == 0)
                {
                    continue;
                }
                auto add_edge = [&](int x0, int y0, int x1, int y1)
                {
                    district_edges[id - 1].push_back({x0, y0, x1, y1, false});
                };

                const uint32_t left = (x > 0) ? out_field.district_ids[idx - 1] : 0;
                const uint32_t right = (x + 1 < grid_res) ? out_field.district_ids[idx + 1] : 0;
                const uint32_t down = (y > 0) ? out_field.district_ids[idx - grid_res] : 0;
                const uint32_t up = (y + 1 < grid_res) ? out_field.district_ids[idx + grid_res] : 0;

                if (left != id)
                {
                    add_edge(x, y, x, y + 1);
                }
                if (right != id)
                {
                    add_edge(x + 1, y + 1, x + 1, y);
                }
                if (down != id)
                {
                    add_edge(x + 1, y, x, y);
                }
                if (up != id)
                {
                    add_edge(x, y + 1, x + 1, y + 1);
                }
            }
        }

        for (std::size_t i = 0; i < city.districts.size(); ++i)
        {
            city.districts[i].border = build_border_loop(district_edges[i], origin, out_field.cell_size);
            if (city.districts[i].border.empty())
            {
                city.districts[i].border = {
                    {bounds.min.x, bounds.min.y},
                    {bounds.max.x, bounds.min.y},
                    {bounds.max.x, bounds.max.y},
                    {bounds.min.x, bounds.max.y},
                    {bounds.min.x, bounds.min.y}};
            }

            CityModel::Vec2 center{bounds.min.x + extent.x * 0.5, bounds.min.y + extent.y * 0.5};
            if (counts[i] > 0)
            {
                center.x = center_sums[i].x / static_cast<double>(counts[i]);
                center.y = center_sums[i].y / static_cast<double>(counts[i]);
            }

            auto primary_bias = axiom_bias_for_type(0);
            auto secondary_bias = axiom_bias_for_type(0);
            auto primary_influencer_bias = influencer_bias_for_type(InfluencerType::None);
            auto secondary_influencer_bias = influencer_bias_for_type(InfluencerType::None);
            for (const auto &axiom : axioms)
            {
                if (axiom.id == city.districts[i].primary_axiom_id)
                {
                    primary_bias = axiom_bias_for_type(axiom.type);
                    primary_influencer_bias = influencer_bias_for_type(axiom.influencer);
                }
                if (axiom.id == city.districts[i].secondary_axiom_id)
                {
                    secondary_bias = axiom_bias_for_type(axiom.type);
                    secondary_influencer_bias = influencer_bias_for_type(axiom.influencer);
                }
            }

            auto road_type = nearest_road_type(city, center);
            auto profile = CityModel::FrontageProfiles::get(road_type);
            auto frontage_bias = frontage_bias_from_profile(profile);

            double w_axiom = settings.desire_weight_axiom;
            double w_frontage = settings.desire_weight_frontage;
            const double sum = w_axiom + w_frontage;
            if (!settings.disable_weight_normalization && sum > 1e-6)
            {
                w_axiom /= sum;
                w_frontage /= sum;
            }
            else if (sum <= 1e-6)
            {
                w_axiom = 0.6;
                w_frontage = 0.4;
            }

            double geometry_factor = 1.0;
            if (settings.enable_desire_geometry_factor && settings.desire_density_radius > 0.0)
            {
                int local_density = 0;
                const double radius_sq = settings.desire_density_radius * settings.desire_density_radius;
                for (const auto &axiom : axioms)
                {
                    const double dx = axiom.pos.x - center.x;
                    const double dy = axiom.pos.y - center.y;
                    if (dx * dx + dy * dy <= radius_sq)
                    {
                        local_density++;
                    }
                }
                const double density_ratio = std::min(1.0, static_cast<double>(local_density) / 3.0);
                geometry_factor = 0.8 + 0.4 * density_ratio;
            }

            std::array<double, 5> scores{};
            for (int s = 0; s < 5; ++s)
            {
                // Combine axiom shape bias with influencer bias (landmark seeding)
                double axiom_score = primary_bias[s] + 0.5 * secondary_bias[s];
                double influencer_score = primary_influencer_bias[s] + 0.3 * secondary_influencer_bias[s];
                // Influencers have strong effect when present (weight 0.4 of axiom contribution)
                double combined_axiom = axiom_score * 0.6 + influencer_score * 0.4;
                scores[s] = w_axiom * combined_axiom * geometry_factor + w_frontage * frontage_bias[s];
            }

            int best_index = 0;
            double best_score = scores[0];
            for (int s = 1; s < 5; ++s)
            {
                if (scores[s] > best_score + settings.desire_score_epsilon)
                {
                    best_score = scores[s];
                    best_index = s;
                }
            }

            if (settings.debug_log_desire_scores)
            {
                RCG::DebugLog::printf("District %u: Mixed=%.3f Res=%.3f Com=%.3f Civ=%.3f Ind=%.3f -> winner=%d\n",
                                      city.districts[i].id,
                                      scores[0], scores[1], scores[2], scores[3], scores[4],
                                      best_index);
            }

            city.districts[i].type = static_cast<CityModel::DistrictType>(best_index);

            if (tensor_field)
            {
                CityModel::Vec2 dir = tensor_field->evaluate(center, true);
                if (dir.lengthSquared() > 1e-6)
                {
                    dir.normalize();
                }
                city.districts[i].orientation = dir;
            }
        }
        RCG::DebugLog::printf("[DistrictGen] done districts=%zu field=%dx%d\n",
                              city.districts.size(), out_field.width, out_field.height);
    }

    void clip_roads_to_districts(CityModel::City &city, const DistrictField &field)
    {
        if (!field.valid())
        {
            return;
        }

#if RCG_USE_GEOS
        GeosWkbWriter geos_writer;
        GeosWkbWriter *geos_ptr = geos_writer.ctx ? &geos_writer : nullptr;
#endif

        for (std::size_t i = 0; i < city.roads_by_type.size(); ++i)
        {
            std::vector<CityModel::Polyline> clipped;
            clipped.reserve(city.roads_by_type[i].size());
            for (const auto &road : city.roads_by_type[i])
            {
                clip_polyline_points(road.points, field, clipped
#if RCG_USE_GEOS
                                     ,
                                     geos_ptr
#endif
                );
            }
            city.roads_by_type[i] = std::move(clipped);
        }

        uint32_t next_id = 1;
        for (std::size_t i = 0; i < city.segment_roads_by_type.size(); ++i)
        {
            std::vector<CityModel::Road> clipped;
            clipped.reserve(city.segment_roads_by_type[i].size());
            clip_segment_roads(city.segment_roads_by_type[i], field, clipped, next_id
#if RCG_USE_GEOS
                               ,
                               geos_ptr
#endif
            );
            city.segment_roads_by_type[i] = std::move(clipped);
        }
    }
} // namespace DistrictGenerator
