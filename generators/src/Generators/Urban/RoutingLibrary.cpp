#include "RogueCity/Generators/Urban/RoutingLibrary.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace RogueCity::Generators::Urban::Routing {

    namespace {

        // The invalid sentinel for VertexID / EdgeID values.
        constexpr VertexID kInvalidVertex = std::numeric_limits<VertexID>::max();
        constexpr EdgeID   kInvalidEdge   = std::numeric_limits<EdgeID>::max();
        constexpr double   kInf           = std::numeric_limits<double>::infinity();

        // Returns the other endpoint of an undirected edge relative to vertex v.
        [[nodiscard]] inline VertexID neighborOf(const Edge& e, VertexID v) noexcept {
            return (e.a == v) ? e.b : e.a;
        }

        // Edge weight: edge.length clamped to a small positive minimum.
        [[nodiscard]] inline double edgeWeight(const Edge& e) noexcept {
            return static_cast<double>(std::max(0.001f, e.length));
        }

        // ---------------------------------------------------------------------------
        // Internal: plain single-source Dijkstra that returns full distance + prev maps.
        // Used by multiple algorithms below without modifying GraphAlgorithms.
        // ---------------------------------------------------------------------------
        struct DijkstraResult {
            std::vector<double>   dist;
            std::vector<VertexID> prev;
            std::vector<EdgeID>   prev_edge;
        };

        [[nodiscard]] DijkstraResult runDijkstra(
            const Graph&             g,
            VertexID                 src,
            const std::vector<bool>& edge_allowed) // empty = all allowed
        {
            const size_t n = g.vertices().size();
            DijkstraResult out;
            out.dist.assign(n, kInf);
            out.prev.assign(n, kInvalidVertex);
            out.prev_edge.assign(n, kInvalidEdge);

            if (!g.isVertexValid(src)) {
                return out;
            }

            using QNode = std::pair<double, VertexID>;
            auto cmp = [](const QNode& a, const QNode& b) { return a.first > b.first; };
            std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> pq(cmp);

            out.dist[src] = 0.0;
            pq.push({0.0, src});

            while (!pq.empty()) {
                const auto [d, v] = pq.top();
                pq.pop();
                if (d > out.dist[v]) {
                    continue;
                }

                const auto* vert = g.getVertex(v);
                if (vert == nullptr) {
                    continue;
                }
                for (const EdgeID eid : vert->edges) {
                    // Respect optional edge-allow filter.
                    if (!edge_allowed.empty() && eid < edge_allowed.size() && !edge_allowed[eid]) {
                        continue;
                    }
                    const auto* e = g.getEdge(eid);
                    if (e == nullptr) {
                        continue;
                    }
                    const VertexID nb   = neighborOf(*e, v);
                    const double   next = out.dist[v] + edgeWeight(*e);
                    if (next < out.dist[nb]) {
                        out.dist[nb]      = next;
                        out.prev[nb]      = v;
                        out.prev_edge[nb] = eid;
                        pq.push({next, nb});
                    }
                }
            }
            return out;
        }

        // Overload without edge filter.
        [[nodiscard]] DijkstraResult runDijkstra(const Graph& g, VertexID src) {
            return runDijkstra(g, src, {});
        }

        // Reconstruct a vertex path from a predecessor map.
        // Returns empty if path is invalid (src not reachable to dst).
        [[nodiscard]] PathResult reconstructPath(
            const DijkstraResult& r,
            VertexID              src,
            VertexID              dst)
        {
            PathResult out;
            if (!std::isfinite(r.dist[dst])) {
                return out;
            }
            out.cost = r.dist[dst];

            std::vector<VertexID> path;
            for (VertexID cur = dst; cur != kInvalidVertex; cur = r.prev[cur]) {
                path.push_back(cur);
                if (cur == src) {
                    break;
                }
                if (path.size() > r.dist.size()) {
                    // Cycle guard – should not happen in a valid graph.
                    return PathResult{};
                }
            }
            std::reverse(path.begin(), path.end());
            if (path.empty() || path.front() != src) {
                return PathResult{};
            }
            out.vertices = std::move(path);
            return out;
        }

    } // namespace

    // =========================================================================
    // 1. Yen's K-Shortest Loopless Paths
    // =========================================================================
    KPathsResult yenKShortestPaths(
        const Graph& g,
        VertexID     src,
        VertexID     dst,
        int          k)
    {
        KPathsResult result;
        if (k <= 0 || !g.isVertexValid(src) || !g.isVertexValid(dst)) {
            return result;
        }

        // A-list: confirmed k-shortest paths.
        std::vector<PathResult> A;
        // B-list: candidate paths (min-heap by cost).
        auto bCmp = [](const PathResult& a, const PathResult& b) { return a.cost > b.cost; };
        std::priority_queue<PathResult, std::vector<PathResult>, decltype(bCmp)> B(bCmp);
        // Track which candidate paths are already in B to avoid duplicates.
        std::set<std::vector<VertexID>> seen;

        // Seed with the plain shortest path.
        PathResult first = GraphAlgorithms::shortestPath(g, src, dst);
        if (!first.reachable()) {
            return result;
        }
        A.push_back(first);

        for (int ki = 1; ki < k; ++ki) {
            const PathResult& prev = A[static_cast<size_t>(ki) - 1];

            for (size_t spur_idx = 0; spur_idx + 1 < prev.vertices.size(); ++spur_idx) {
                const VertexID spur_node = prev.vertices[spur_idx];

                // Root path: prev.vertices[0..spur_idx].
                const std::vector<VertexID> root(
                    prev.vertices.begin(),
                    prev.vertices.begin() + static_cast<ptrdiff_t>(spur_idx + 1));

                // Build edge-removal set: remove edges used by paths that share
                // the same root prefix to avoid duplicate spur paths.
                std::unordered_set<EdgeID> removed_edges;

                for (const PathResult& a : A) {
                    if (a.vertices.size() > spur_idx &&
                        std::equal(root.begin(), root.end(), a.vertices.begin())) {
                        // Remove the edge leaving spur_node in this path.
                        if (spur_idx + 1 < a.vertices.size()) {
                            const VertexID nb = a.vertices[spur_idx + 1];
                            const auto* sv = g.getVertex(spur_node);
                            if (sv != nullptr) {
                                for (EdgeID eid : sv->edges) {
                                    const auto* e = g.getEdge(eid);
                                    if (e == nullptr) {
                                        continue;
                                    }
                                    if (neighborOf(*e, spur_node) == nb) {
                                        removed_edges.insert(eid);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // Mark root interior nodes (excluding spur_node) as unreachable.
                std::unordered_set<VertexID> removed_vertices(root.begin(), root.end());
                removed_vertices.erase(spur_node);

                // Build edge-allowed mask (edges in removed_edges or incident to
                // removed interior vertices are disallowed).
                std::vector<bool> edge_allowed(g.edges().size(), true);
                for (EdgeID eid : removed_edges) {
                    if (eid < edge_allowed.size()) {
                        edge_allowed[eid] = false;
                    }
                }
                for (VertexID rv : removed_vertices) {
                    const auto* rv_ptr = g.getVertex(rv);
                    if (rv_ptr == nullptr) {
                        continue;
                    }
                    for (EdgeID eid : rv_ptr->edges) {
                        if (eid < edge_allowed.size()) {
                            edge_allowed[eid] = false;
                        }
                    }
                }

                // Run Dijkstra from spur_node to dst under restrictions.
                const DijkstraResult dr = runDijkstra(g, spur_node, edge_allowed);
                if (!std::isfinite(dr.dist[dst])) {
                    continue;
                }

                // Reconstruct spur path.
                PathResult spur = reconstructPath(dr, spur_node, dst);
                if (!spur.reachable()) {
                    continue;
                }

                // Total path = root prefix + spur (excluding the duplicated spur_node).
                PathResult candidate;
                candidate.vertices = root;
                candidate.vertices.insert(
                    candidate.vertices.end(),
                    spur.vertices.begin() + 1,
                    spur.vertices.end());

                // Recompute cost as sum of edge lengths.
                double total = 0.0;
                bool   valid = true;
                for (size_t vi = 0; vi + 1 < candidate.vertices.size(); ++vi) {
                    const VertexID va = candidate.vertices[vi];
                    const VertexID vb = candidate.vertices[vi + 1];
                    const auto* pv = g.getVertex(va);
                    if (pv == nullptr) { valid = false; break; }
                    bool found = false;
                    for (EdgeID eid : pv->edges) {
                        const auto* e = g.getEdge(eid);
                        if (e == nullptr) { continue; }
                        if (neighborOf(*e, va) == vb) {
                            total += edgeWeight(*e);
                            found = true;
                            break;
                        }
                    }
                    if (!found) { valid = false; break; }
                }
                if (!valid) {
                    continue;
                }
                candidate.cost = total;

                if (seen.insert(candidate.vertices).second) {
                    B.push(candidate);
                }
            }

            if (B.empty()) {
                break;
            }
            A.push_back(B.top());
            B.pop();
        }

        result.paths = std::move(A);
        return result;
    }

    // =========================================================================
    // 2. Multi-Level Highway Hierarchies
    // =========================================================================
    std::vector<HierarchyLevel> extractHighwayHierarchy(
        const Graph& g,
        int          num_levels)
    {
        // Map edge type to a 0-based level index.
        // Higher RoadType enum values correspond to higher-order roads.
        auto edgeLevel = [](const Edge& e, int max_level) -> int {
            // Core::RoadType values: Street=0, M_Minor=1, M_Major=2, Arterial=3, Highway=4...
            // layer_id provides an additional secondary classification.
            const int type_level = static_cast<int>(e.type);
            const int layer_boost = std::max(0, e.layer_id);
            const int raw = type_level + layer_boost;
            return std::min(raw, max_level - 1);
        };

        const int levels = std::max(1, num_levels);
        std::vector<HierarchyLevel> result(static_cast<size_t>(levels));
        for (int i = 0; i < levels; ++i) {
            result[static_cast<size_t>(i)].level = i;
        }

        // Assign edges to levels.
        for (EdgeID eid = 0; eid < static_cast<EdgeID>(g.edges().size()); ++eid) {
            const auto* e = g.getEdge(eid);
            if (e == nullptr) {
                continue;
            }
            const int lv = edgeLevel(*e, levels);
            result[static_cast<size_t>(lv)].highway_edges.push_back(eid);
        }

        // Collect vertices that participate in each level.
        for (auto& lvl : result) {
            std::unordered_set<VertexID> vset;
            for (EdgeID eid : lvl.highway_edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                vset.insert(e->a);
                vset.insert(e->b);
            }
            lvl.highway_vertices.assign(vset.begin(), vset.end());
            std::sort(lvl.highway_vertices.begin(), lvl.highway_vertices.end());
        }
        return result;
    }

    // =========================================================================
    // 3. Delta-Stepping Distance Field (single-threaded baseline)
    // =========================================================================
    DistanceField deltaSteppingDistanceField(
        const Graph& g,
        VertexID     src,
        double       delta)
    {
        DistanceField field;
        const size_t n = g.vertices().size();
        field.dist.assign(n, kInf);

        if (!g.isVertexValid(src) || n == 0) {
            return field;
        }

        // Clamp delta to a reasonable positive value.
        const double bucket_width = std::max(1e-6, delta);

        // Number of buckets: sized to cover the diameter of the graph.
        // In practice we use a deque of buckets that grows on demand.
        field.dist[src] = 0.0;

        // bucket[i] holds vertices with tentative distance in [i*delta, (i+1)*delta).
        std::deque<std::vector<VertexID>> buckets;
        auto getBucket = [&](double d) -> size_t {
            return static_cast<size_t>(d / bucket_width);
        };
        auto ensureBucket = [&](size_t idx) {
            while (buckets.size() <= idx) {
                buckets.emplace_back();
            }
        };

        ensureBucket(0);
        buckets[0].push_back(src);

        size_t current_bucket = 0;

        while (current_bucket < buckets.size()) {
            // Process all vertices in the current bucket (may re-insert during processing).
            while (!buckets[current_bucket].empty()) {
                std::vector<VertexID> S = std::move(buckets[current_bucket]);
                buckets[current_bucket].clear();

                for (const VertexID v : S) {
                    // Skip stale entry: vertex was already settled to a
                    // bucket earlier than current_bucket.
                    if (field.dist[v] < static_cast<double>(current_bucket) * bucket_width) {
                        continue;
                    }
                    const auto* vert = g.getVertex(v);
                    if (vert == nullptr) {
                        continue;
                    }
                    for (const EdgeID eid : vert->edges) {
                        const auto* e = g.getEdge(eid);
                        if (e == nullptr) {
                            continue;
                        }
                        const VertexID nb   = neighborOf(*e, v);
                        const double   next = field.dist[v] + edgeWeight(*e);
                        if (next < field.dist[nb]) {
                            field.dist[nb] = next;
                            const size_t bk = getBucket(next);
                            ensureBucket(bk);
                            buckets[bk].push_back(nb);
                        }
                    }
                }
            }
            ++current_bucket;
        }
        return field;
    }

    // =========================================================================
    // 4. Loopless Candidate Generation (Pascoal) – safe placeholder
    // =========================================================================
    LooplessCandidates looplessCandidates(
        const Graph& /*g*/,
        VertexID     /*src*/,
        VertexID     /*dst*/,
        int          /*max_candidates*/)
    {
        // Placeholder: full Pascoal enumeration is not yet implemented.
        // Returns empty candidates as documented; callers should handle this gracefully.
        return LooplessCandidates{};
    }

    // =========================================================================
    // 5. Edge-Flag Dijkstra
    // =========================================================================
    PathResult edgeFlagDijkstra(
        const Graph&             g,
        VertexID                 src,
        VertexID                 dst,
        const std::vector<bool>& edge_flags)
    {
        const DijkstraResult dr = runDijkstra(g, src, edge_flags);
        if (dst >= dr.dist.size()) {
            return PathResult{};
        }
        return reconstructPath(dr, src, dst);
    }

    // =========================================================================
    // 6. Formal-Language-Constrained Shortest Path
    // =========================================================================
    PathResult constrainedShortestPath(
        const Graph&              g,
        VertexID                  src,
        VertexID                  dst,
        const AutomatonConstraint& automaton)
    {
        // Guard: trivial automaton (single always-accepting state) falls through
        // to standard Dijkstra.
        if (automaton.num_states <= 0 || !automaton.transition) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        const size_t n  = g.vertices().size();
        const int    ns = automaton.num_states;

        // State space: (vertex_id, automaton_state).
        // dist[v][s] = shortest known distance to vertex v in automaton state s.
        std::vector<std::vector<double>> dist(n, std::vector<double>(static_cast<size_t>(ns), kInf));
        std::vector<std::vector<VertexID>> prev_v(n, std::vector<VertexID>(static_cast<size_t>(ns), kInvalidVertex));
        std::vector<std::vector<int>> prev_s(n, std::vector<int>(static_cast<size_t>(ns), -1));

        using QNode = std::tuple<double, VertexID, int>; // (cost, vertex, automaton_state)
        auto cmp = [](const QNode& a, const QNode& b) { return std::get<0>(a) > std::get<0>(b); };
        std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> pq(cmp);

        const int init_state = automaton.initial_state;
        if (init_state < 0 || init_state >= ns) {
            return PathResult{};
        }

        dist[src][static_cast<size_t>(init_state)] = 0.0;
        pq.push({0.0, src, init_state});

        while (!pq.empty()) {
            const auto [d, v, s] = pq.top();
            pq.pop();
            if (d > dist[v][static_cast<size_t>(s)]) {
                continue;
            }

            const auto* vert = g.getVertex(v);
            if (vert == nullptr) {
                continue;
            }
            for (const EdgeID eid : vert->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                const int ns_next = automaton.transition(s, eid);
                if (ns_next < 0 || ns_next >= ns) {
                    // Dead transition.
                    continue;
                }
                const VertexID nb   = neighborOf(*e, v);
                const double   next = d + edgeWeight(*e);
                if (next < dist[nb][static_cast<size_t>(ns_next)]) {
                    dist[nb][static_cast<size_t>(ns_next)] = next;
                    prev_v[nb][static_cast<size_t>(ns_next)] = v;
                    prev_s[nb][static_cast<size_t>(ns_next)] = s;
                    pq.push({next, nb, ns_next});
                }
            }
        }

        // Find the best accepting state at dst.
        double best_cost = kInf;
        int    best_state = -1;
        for (int acc : automaton.accepting_states) {
            if (acc < 0 || acc >= ns) {
                continue;
            }
            if (dist[dst][static_cast<size_t>(acc)] < best_cost) {
                best_cost  = dist[dst][static_cast<size_t>(acc)];
                best_state = acc;
            }
        }
        if (best_state < 0 || !std::isfinite(best_cost)) {
            return PathResult{};
        }

        // Reconstruct path by walking predecessor maps.
        std::vector<VertexID> path;
        VertexID cur = dst;
        int      cs  = best_state;
        while (cur != kInvalidVertex) {
            path.push_back(cur);
            if (cur == src) {
                break;
            }
            const VertexID pv = prev_v[cur][static_cast<size_t>(cs)];
            const int      ps = prev_s[cur][static_cast<size_t>(cs)];
            cur = pv;
            cs  = ps;
            if (path.size() > n + 1) {
                // Cycle guard.
                return PathResult{};
            }
        }
        std::reverse(path.begin(), path.end());
        if (path.empty() || path.front() != src) {
            return PathResult{};
        }
        PathResult out;
        out.cost     = best_cost;
        out.vertices = std::move(path);
        return out;
    }

    // =========================================================================
    // 7. Crauser OUT-Criterion Helpers
    // =========================================================================
    CrauserLabels computeCrauserLabels(const Graph& g) {
        CrauserLabels labels;
        labels.out_lower_bound.assign(g.vertices().size(), kInf);

        for (VertexID v = 0; v < static_cast<VertexID>(g.vertices().size()); ++v) {
            const auto* vert = g.getVertex(v);
            if (vert == nullptr) {
                continue;
            }
            double min_w = kInf;
            for (const EdgeID eid : vert->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                min_w = std::min(min_w, edgeWeight(*e));
            }
            labels.out_lower_bound[v] = min_w;
        }
        return labels;
    }

    // =========================================================================
    // 8. Transit Node Routing Query
    // =========================================================================
    PathResult transitNodeQuery(
        const Graph&          g,
        VertexID              src,
        VertexID              dst,
        const TransitNodeSet& tns)
    {
        // Fall back when transit node data is absent.
        if (tns.nodes.empty() ||
            tns.forward_access.empty() ||
            tns.backward_access.empty() ||
            tns.dist_table.empty()) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        const size_t nt = tns.nodes.size();
        const size_t n  = g.vertices().size();

        // Validate that access vectors cover src/dst.
        if (src >= n || dst >= n ||
            src >= tns.forward_access.size() ||
            dst >= tns.backward_access.size()) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        const auto& fa = tns.forward_access[src];
        const auto& ba = tns.backward_access[dst];

        if (fa.size() < nt || ba.size() < nt) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        // TNR query: best cost through any transit node pair (i, j).
        double best = kInf;
        size_t best_i = nt;
        size_t best_j = nt;
        for (size_t i = 0; i < nt; ++i) {
            if (!std::isfinite(fa[i])) {
                continue;
            }
            if (tns.dist_table[i].size() < nt) {
                continue;
            }
            for (size_t j = 0; j < nt; ++j) {
                if (!std::isfinite(ba[j])) {
                    continue;
                }
                const double c = fa[i] + tns.dist_table[i][j] + ba[j];
                if (c < best) {
                    best   = c;
                    best_i = i;
                    best_j = j;
                }
            }
        }
        if (!std::isfinite(best) || best_i == nt) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        // Return a synthetic PathResult containing just the cost and the
        // src/tn_i/tn_j/dst vertex chain.  Full waypoint reconstruction would
        // require per-access predecessor data which is not stored in TransitNodeSet.
        PathResult out;
        out.cost = best;
        out.vertices = {src, tns.nodes[best_i], tns.nodes[best_j], dst};
        // Deduplicate consecutive equal vertices (e.g. if src == tn_i).
        out.vertices.erase(
            std::unique(out.vertices.begin(), out.vertices.end()),
            out.vertices.end());
        return out;
    }

    // =========================================================================
    // 9. External Memory BFS Helper
    // =========================================================================
    BFSResult externalMemoryBFS(const Graph& g, VertexID src) {
        BFSResult result;
        const size_t n = g.vertices().size();
        result.level.assign(n, -1);
        result.prev.assign(n, kInvalidVertex);

        if (!g.isVertexValid(src)) {
            return result;
        }

        std::deque<VertexID> queue;
        result.level[src] = 0;
        queue.push_back(src);

        while (!queue.empty()) {
            const VertexID v = queue.front();
            queue.pop_front();

            const auto* vert = g.getVertex(v);
            if (vert == nullptr) {
                continue;
            }
            for (const EdgeID eid : vert->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                const VertexID nb = neighborOf(*e, v);
                if (result.level[nb] == -1) {
                    result.level[nb] = result.level[v] + 1;
                    result.prev[nb]  = v;
                    queue.push_back(nb);
                }
            }
        }
        return result;
    }

    // =========================================================================
    // 10. Two-Level Arc-Flags Helper
    // =========================================================================
    PathResult arcFlagsDijkstra(
        const Graph&        g,
        VertexID            src,
        VertexID            dst,
        const ArcFlagsData& arc_flags,
        int                 target_region)
    {
        const size_t n = g.vertices().size();

        // If arc-flags data is absent, run normal Dijkstra.
        if (arc_flags.flags.empty() || arc_flags.num_regions <= 0) {
            const DijkstraResult dr = runDijkstra(g, src);
            if (dst >= dr.dist.size()) {
                return PathResult{};
            }
            return reconstructPath(dr, src, dst);
        }

        std::vector<double>   dist(n, kInf);
        std::vector<VertexID> prev(n, kInvalidVertex);

        using QNode = std::pair<double, VertexID>;
        auto cmp = [](const QNode& a, const QNode& b) { return a.first > b.first; };
        std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> pq(cmp);

        dist[src] = 0.0;
        pq.push({0.0, src});

        while (!pq.empty()) {
            const auto [d, v] = pq.top();
            pq.pop();
            if (d > dist[v]) {
                continue;
            }
            if (v == dst) {
                break;
            }

            const auto* vert = g.getVertex(v);
            if (vert == nullptr) {
                continue;
            }
            for (const EdgeID eid : vert->edges) {
                // Prune edge if arc-flag for target_region is false.
                if (eid < arc_flags.flags.size() &&
                    target_region >= 0 &&
                    target_region < arc_flags.num_regions) {
                    const auto& ef = arc_flags.flags[eid];
                    const auto  tr = static_cast<size_t>(target_region);
                    if (tr < ef.size() && !ef[tr]) {
                        continue;
                    }
                }
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                const VertexID nb   = neighborOf(*e, v);
                const double   next = dist[v] + edgeWeight(*e);
                if (next < dist[nb]) {
                    dist[nb] = next;
                    prev[nb] = v;
                    pq.push({next, nb});
                }
            }
        }

        if (!std::isfinite(dist[dst])) {
            return PathResult{};
        }

        PathResult out;
        out.cost = dist[dst];
        std::vector<VertexID> path;
        for (VertexID cur = dst; cur != kInvalidVertex; cur = prev[cur]) {
            path.push_back(cur);
            if (cur == src) {
                break;
            }
            if (path.size() > n + 1) {
                return PathResult{};
            }
        }
        std::reverse(path.begin(), path.end());
        if (path.empty() || path.front() != src) {
            return PathResult{};
        }
        out.vertices = std::move(path);
        return out;
    }

    // =========================================================================
    // 11. REAL (Reach + ALT) A* Search
    // =========================================================================
    PathResult realAStar(
        const Graph&    g,
        VertexID        src,
        VertexID        dst,
        const REALData& real_data)
    {
        // If reach or landmark data is absent, fall back to plain shortest path.
        if (real_data.reach.empty() ||
            real_data.landmarks.empty() ||
            real_data.landmark_dist.empty()) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        const size_t n  = g.vertices().size();
        const size_t nl = real_data.landmarks.size();

        // Validate landmark_dist dimensions.
        if (real_data.landmark_dist.size() < nl) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }
        for (const auto& ld : real_data.landmark_dist) {
            if (ld.size() < n) {
                return GraphAlgorithms::shortestPath(g, src, dst);
            }
        }
        if (real_data.reach.size() < n) {
            return GraphAlgorithms::shortestPath(g, src, dst);
        }

        // ALT lower-bound heuristic: h(v) = max over landmarks L of
        //   |dist(L, dst) - dist(L, v)|.
        auto heuristic = [&](VertexID v) -> double {
            double h = 0.0;
            for (size_t li = 0; li < nl; ++li) {
                const double lv  = real_data.landmark_dist[li][v];
                const double ld  = real_data.landmark_dist[li][dst];
                if (std::isfinite(lv) && std::isfinite(ld)) {
                    h = std::max(h, std::abs(ld - lv));
                }
            }
            return h;
        };

        std::vector<double>   g_cost(n, kInf); // true cost from src
        std::vector<VertexID> prev(n, kInvalidVertex);

        using QNode = std::pair<double, VertexID>; // (f = g + h, vertex)
        auto cmp = [](const QNode& a, const QNode& b) { return a.first > b.first; };
        std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> pq(cmp);

        g_cost[src] = 0.0;
        pq.push({heuristic(src), src});

        while (!pq.empty()) {
            const auto [f, v] = pq.top();
            pq.pop();

            if (v == dst) {
                break;
            }

            // Stale entry guard.
            if (f > g_cost[v] + heuristic(v) + 1e-9) {
                continue;
            }

            const auto* vert = g.getVertex(v);
            if (vert == nullptr) {
                continue;
            }
            for (const EdgeID eid : vert->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                const VertexID nb  = neighborOf(*e, v);
                const double   w   = edgeWeight(*e);
                const double   ng  = g_cost[v] + w;

                // Reach-based pruning: skip vertex nb if reach[nb] < ng and
                // the remaining distance to dst is also > reach[nb].
                // This requires a lower bound on dist(nb, dst) >= heuristic(nb).
                const double reach_nb = real_data.reach[nb];
                if (reach_nb < ng && reach_nb < heuristic(nb)) {
                    continue;
                }

                if (ng < g_cost[nb]) {
                    g_cost[nb] = ng;
                    prev[nb]   = v;
                    pq.push({ng + heuristic(nb), nb});
                }
            }
        }

        if (!std::isfinite(g_cost[dst])) {
            return PathResult{};
        }

        PathResult out;
        out.cost = g_cost[dst];
        std::vector<VertexID> path;
        for (VertexID cur = dst; cur != kInvalidVertex; cur = prev[cur]) {
            path.push_back(cur);
            if (cur == src) {
                break;
            }
            if (path.size() > n + 1) {
                return PathResult{};
            }
        }
        std::reverse(path.begin(), path.end());
        if (path.empty() || path.front() != src) {
            return PathResult{};
        }
        out.vertices = std::move(path);
        return out;
    }

} // namespace RogueCity::Generators::Urban::Routing
