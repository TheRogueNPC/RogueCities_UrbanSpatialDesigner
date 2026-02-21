#include "RogueCity/Generators/Roads/RoadNoder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace RogueCity::Generators::Roads {

    // Initializes noder configuration and segment spatial index.
    RoadNoder::RoadNoder(NoderConfig cfg)
        : cfg_(std::move(cfg))
        , seg_index_(1, 1, 1.0f) {
        if (cfg_.type_params.empty()) {
            cfg_.type_params.resize(Core::road_type_count);
        }
    }

    // Builds a topological graph from polyline candidates by:
    // - indexing raw segments
    // - finding pairwise intersections
    // - splitting segments at cut points
    // - welding segment endpoints into graph vertices
    void RoadNoder::buildGraph(
        const std::vector<PolylineRoadCandidate>& candidates,
        Urban::Graph& out_graph) {
        out_graph.clear();
        if (candidates.empty()) {
            return;
        }

        // Compute spatial extent for segment grid sizing.
        Core::Vec2 max_p(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
        for (const auto& candidate : candidates) {
            for (const auto& p : candidate.pts) {
                max_p.x = std::max(max_p.x, p.x);
                max_p.y = std::max(max_p.y, p.y);
            }
        }

        const float grid_cell = std::max(10.0f, cfg_.global_weld_radius * 4.0f);
        const int grid_w = std::max(1, static_cast<int>(std::ceil(std::max(0.0, max_p.x) / grid_cell)) + 2);
        const int grid_h = std::max(1, static_cast<int>(std::ceil(std::max(0.0, max_p.y) / grid_cell)) + 2);
        seg_index_ = SegmentGridStorage(grid_w, grid_h, grid_cell);

        // Flatten candidate polylines into raw segment records.
        std::vector<RawSegment> segments;
        segments.reserve(candidates.size() * 8);
        uint32_t seg_id = 0;
        for (uint32_t cid = 0; cid < candidates.size(); ++cid) {
            const auto& c = candidates[cid];
            if (c.pts.size() < 2) {
                continue;
            }
            const int layer = std::clamp(c.layer_hint, 0, std::max(0, cfg_.max_layers - 1));
            for (uint32_t i = 1; i < c.pts.size(); ++i) {
                const Core::Vec2& a = c.pts[i - 1];
                const Core::Vec2& b = c.pts[i];
                if (a.equals(b, 1e-6)) {
                    continue;
                }

                RawSegment raw{};
                raw.id = seg_id++;
                raw.candidate_id = cid;
                raw.local_index = i - 1;
                raw.a = a;
                raw.b = b;
                raw.layer_id = layer;
                raw.type = c.type_hint;
                segments.push_back(raw);
                seg_index_.insert({ raw.id, raw.a, raw.b, raw.layer_id });
            }
        }

        if (segments.empty()) {
            return;
        }

        // Each segment starts with endpoints as mandatory split cuts.
        std::vector<std::vector<double>> cuts(segments.size());
        for (auto& c : cuts) {
            c.push_back(0.0);
            c.push_back(1.0);
        }

        float max_snap = cfg_.global_weld_radius;
        for (const auto& p : cfg_.type_params) {
            max_snap = std::max(max_snap, p.snap_radius);
        }

        constexpr float kTol = 1e-4f;
        // Query nearby segments and collect geometric intersections as split parameters.
        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& seg = segments[i];
            const Core::Vec2 mid = (seg.a + seg.b) * 0.5;
            const float seg_len = static_cast<float>(seg.a.distanceTo(seg.b));
            const float query_radius = seg_len * 0.6f + max_snap + 1.0f;

            std::vector<SegmentRef> nearby;
            seg_index_.queryRadius(mid, query_radius, seg.layer_id, nearby);

            for (const auto& ref : nearby) {
                if (ref.edge_id <= seg.id || ref.edge_id >= segments.size()) {
                    continue;
                }
                const auto& other = segments[ref.edge_id];
                if (seg.candidate_id == other.candidate_id) {
                    const int delta = std::abs(static_cast<int>(seg.local_index) - static_cast<int>(other.local_index));
                    if (delta <= 1) {
                        continue;
                    }
                }

                Core::Vec2 isect{};
                double ta = 0.0;
                double tb = 0.0;
                if (!segmentIntersect(seg.a, seg.b, other.a, other.b, isect, ta, tb, kTol)) {
                    continue;
                }

                const bool at_a_end = ta <= 1e-3 || ta >= (1.0 - 1e-3);
                const bool at_b_end = tb <= 1e-3 || tb >= (1.0 - 1e-3);
                if (seg.candidate_id == other.candidate_id && at_a_end && at_b_end) {
                    continue;
                }

                cuts[i].push_back(std::clamp(ta, 0.0, 1.0));
                cuts[ref.edge_id].push_back(std::clamp(tb, 0.0, 1.0));
            }
        }

        // Materialize split edges and insert into graph with per-type defaults.
        for (size_t i = 0; i < segments.size(); ++i) {
            auto& split = cuts[i];
            std::sort(split.begin(), split.end());
            split.erase(std::unique(split.begin(), split.end(),
                                    [](double a, double b) { return std::abs(a - b) < 1e-4; }),
                        split.end());

            const auto& seg = segments[i];
            const auto& type_cfg = paramsForType(seg.type);
            const float min_edge_len = std::max(cfg_.global_min_edge_length, type_cfg.min_edge_length);
            const float weld_radius = std::max(cfg_.global_weld_radius, type_cfg.weld_radius);

            for (size_t k = 1; k < split.size(); ++k) {
                const double t0 = split[k - 1];
                const double t1 = split[k];
                if ((t1 - t0) < 1e-5) {
                    continue;
                }

                const Core::Vec2 p0 = Core::lerp(seg.a, seg.b, t0);
                const Core::Vec2 p1 = Core::lerp(seg.a, seg.b, t1);
                const double len = p0.distanceTo(p1);
                if (len < static_cast<double>(min_edge_len)) {
                    continue;
                }

                const Urban::VertexID va = getOrCreateVertex(out_graph, p0, seg.layer_id, weld_radius);
                const Urban::VertexID vb = getOrCreateVertex(out_graph, p1, seg.layer_id, weld_radius);
                if (va == vb) {
                    continue;
                }

                Urban::Edge edge{};
                edge.a = va;
                edge.b = vb;
                edge.type = seg.type;
                edge.layer_id = seg.layer_id;
                edge.shape = { p0, p1 };
                edge.length = static_cast<float>(len);
                edge.flow.v_base = type_cfg.v_base;
                edge.flow.cap_base = type_cfg.cap_base;
                edge.flow.access_control = type_cfg.access_control;
                edge.flow.v_eff = type_cfg.v_base;
                out_graph.addEdge(edge);
            }
        }

        // Initialize vertex kind tags from final degree.
        for (Urban::VertexID vid = 0; vid < out_graph.vertices().size(); ++vid) {
            auto* v = out_graph.getVertexMutable(vid);
            if (v == nullptr) {
                continue;
            }
            const size_t degree = v->edges.size();
            if (degree <= 1) {
                v->kind = Urban::VertexKind::DeadEnd;
            } else if (degree >= 3) {
                v->kind = Urban::VertexKind::Intersection;
            } else {
                v->kind = Urban::VertexKind::Normal;
            }
        }
    }

    // Returns per-type noding parameters with fallback to first entry.
    const RoadTypeParams& RoadNoder::paramsForType(Core::RoadType type) const {
        const size_t idx = static_cast<size_t>(type);
        if (idx < cfg_.type_params.size()) {
            return cfg_.type_params[idx];
        }
        return cfg_.type_params.front();
    }

    // Finds existing nearby same-layer vertex or creates a new one.
    Urban::VertexID RoadNoder::getOrCreateVertex(
        Urban::Graph& g,
        const Core::Vec2& p,
        int layer_id,
        float weld_radius) const {
        const double r2 = static_cast<double>(weld_radius) * static_cast<double>(weld_radius);
        for (Urban::VertexID vid = 0; vid < g.vertices().size(); ++vid) {
            const auto* v = g.getVertex(vid);
            if (v == nullptr || v->layer_id != layer_id) {
                continue;
            }
            if (v->pos.distanceToSquared(p) <= r2) {
                return vid;
            }
        }

        Urban::Vertex v{};
        v.pos = p;
        v.layer_id = layer_id;
        return g.addVertex(v);
    }

    // Segment-segment intersection test returning intersection point and normalized params.
    bool RoadNoder::segmentIntersect(
        const Core::Vec2& a0,
        const Core::Vec2& a1,
        const Core::Vec2& b0,
        const Core::Vec2& b1,
        Core::Vec2& out_p,
        double& out_ta,
        double& out_tb,
        float tol) const {
        const Core::Vec2 r = a1 - a0;
        const Core::Vec2 s = b1 - b0;
        const double denom = r.cross(s);
        const Core::Vec2 qmp = b0 - a0;

        if (std::abs(denom) <= tol) {
            if (a0.distanceTo(b0) <= tol) {
                out_p = a0;
                out_ta = 0.0;
                out_tb = 0.0;
                return true;
            }
            if (a0.distanceTo(b1) <= tol) {
                out_p = a0;
                out_ta = 0.0;
                out_tb = 1.0;
                return true;
            }
            if (a1.distanceTo(b0) <= tol) {
                out_p = a1;
                out_ta = 1.0;
                out_tb = 0.0;
                return true;
            }
            if (a1.distanceTo(b1) <= tol) {
                out_p = a1;
                out_ta = 1.0;
                out_tb = 1.0;
                return true;
            }
            return false;
        }

        const double t = qmp.cross(s) / denom;
        const double u = qmp.cross(r) / denom;
        if (t < -tol || t > (1.0 + tol) || u < -tol || u > (1.0 + tol)) {
            return false;
        }

        out_ta = std::clamp(t, 0.0, 1.0);
        out_tb = std::clamp(u, 0.0, 1.0);
        out_p = a0 + (r * out_ta);
        return true;
    }

    // Polyline arc length helper.
    float RoadNoder::polylineLength(const std::vector<Core::Vec2>& pts) const {
        if (pts.size() < 2) {
            return 0.0f;
        }
        double total = 0.0;
        for (size_t i = 1; i < pts.size(); ++i) {
            total += pts[i - 1].distanceTo(pts[i]);
        }
        return static_cast<float>(total);
    }

} // namespace RogueCity::Generators::Roads
