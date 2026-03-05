/// @file RoutingLibrary.cpp
/// @brief Implementations for the additive RoutingLibrary routing adapters.
///
/// All 11 algorithms share the private helpers defined in the anonymous namespace
/// below: neighborOf(), edgeWeight(), runDijkstra(), and reconstructPath().
/// These helpers deliberately do NOT modify GraphAlgorithms.cpp; they are kept
/// private to this translation unit to avoid polluting the Urban namespace.
///
/// ## Build-phase data (pre-computation TODOs)
/// Several algorithms (TNR, arc-flags, REAL) require auxiliary data that must be
/// built once after graph finalisation and cached in CityGenerator::Output.
/// The helper patterns to follow are:
///
/// ```cpp
/// // After CityGenerator::generate() completes:
/// auto& out = generator_output;
///
/// // REAL A* data
/// out.real_data.reach         = buildReachValues(out.road_graph);
/// out.real_data.landmarks     = chooseLandmarks(out.road_graph, /*count=*/8);
/// out.real_data.landmark_dist = buildLandmarkDistances(out.road_graph,
///                                                      out.real_data.landmarks);
///
/// // Transit Node Set (choose transit nodes = highway vertices in top tier)
/// auto hier = Routing::extractHighwayHierarchy(out.road_graph, 3);
/// out.tns.nodes = hier.back().highway_vertices;  // top-level tier
/// buildTransitDistances(out.road_graph, out.tns); // fills dist_table + access vecs
///
/// // Arc-flags (one region per district; use Vertex::layer_id as region index)
/// out.arc_flags = buildArcFlags(out.road_graph, /*num_regions=*/out.districts.size());
/// ```
///
/// None of the three build helpers above exist yet; they belong in a new file
/// `generators/src/Generators/Urban/RoutingPrecompute.cpp` alongside a matching
/// `RoutingPrecompute.hpp`.

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

    // =========================================================================
    // Internal helpers (anonymous namespace)
    // =========================================================================

    // kInvalidVertex / kInvalidEdge: sentinel values used as "no predecessor"
    // markers in predecessor arrays.  Chosen as max() so they are always out of
    // range for any valid graph (graph sizes are bounded by uint32_t capacity).
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
    //
    // Reference: Jin Y. Yen, "Finding the K Shortest Loopless Paths in a Network",
    //            Management Science, 17(11):712–716, 1971.
    //
    // Algorithm outline:
    //   A-list: paths confirmed as the 1st … k-th shortest (grown one per outer iter).
    //   B-heap: min-heap of candidate paths not yet accepted.
    //
    //   Iteration i (finding path A[i]):
    //     For each "spur node" along path A[i-1]:
    //       1. Root = prefix of A[i-1] up to and including the spur node.
    //       2. Remove from the graph:
    //          (a) Edges leaving the spur node that were used by previously
    //              accepted paths with the same root prefix → prevents duplicate spurs.
    //          (b) All edges incident to interior root vertices → enforces looplessness.
    //       3. Run Dijkstra from spur node to dst in the reduced graph.
    //       4. Concatenate root + spur path → candidate.
    //       5. Insert unique candidates into B.
    //     Pop the cheapest candidate from B → A[i].
    //
    // WHY the edge-removal is per-iteration-spur and not global:
    //   Yen's correctness relies on uniquely constraining each spur so that the
    //   resulting candidate cannot duplicate a path already in A.  A global removal
    //   would over-constrain the search and miss valid k-th paths.
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
    //
    // Reference: Robert Geisberger et al., "Contraction Hierarchies: Faster and
    //            Simpler Hierarchical Routing in Road Networks", WEA 2008.
    //            (This implementation provides the first step: level extraction,
    //             NOT full contraction.  Full CH requires shortcut insertion.)
    //
    // Level assignment:
    //   level(e) = clamp(int(e.type) + max(0, e.layer_id),  0, num_levels-1)
    //
    //   Core::RoadType enum values increase with road importance:
    //     Street=0, M_Minor=1, M_Major=2, Arterial=3, Highway=4, …
    //   e.layer_id provides a grade-separation boost (0 = at-grade, 1 = overpass, …).
    //
    // To extend to full Contraction Hierarchies:
    //   1. Compute importance(v) = edge_difference(v) + deleted_neighbours(v)
    //      + shortcut_cover(v) + node_level(v).  A common heuristic combines
    //      these with weights (e.g. 190, 120, 10, 1 – Geisberger 2008 values).
    //   2. Order all vertices by increasing importance (lazy-update priority queue).
    //   3. For each vertex u in order: contract it — for each pair of remaining
    //      neighbours (s, t) where the only witness path goes through u, insert
    //      shortcut edge s→t with weight dist(s,u) + dist(u,t).
    //   4. Store the contracted graph alongside the original; queries use
    //      bidirectional CH-Dijkstra that only relaxes upward edges.
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
    //
    // Reference: Ulrich Meyer & Peter Sanders, "Δ-stepping: A parallelisable
    //            shortest path algorithm", Journal of Algorithms 49(1):114–152, 2003.
    //
    // Key data structure: an infinite sequence of buckets B[0], B[1], … where
    // bucket B[i] holds tentative-settled vertices with distance in [i·Δ, (i+1)·Δ).
    // "Light" edges (weight ≤ Δ) may push a vertex into the same or adjacent bucket.
    // "Heavy" edges (weight > Δ) push into a bucket far ahead — these are safe to
    // defer because a heavy edge cannot create a path cheaper than the current
    // bucket's lower bound.
    //
    // Correctness invariant:
    //   When bucket B[i] is fully drained, all vertices with true shortest distance
    //   in [0, (i+1)·Δ) have been permanently settled.  This mirrors Dijkstra's
    //   greedy correctness but works in bucket granularity rather than per-vertex.
    //
    // Choosing delta:
    //   - Too small (Δ → 0): degrades to Dijkstra, no parallelism benefit.
    //   - Too large (Δ → max_edge): many vertices in each bucket, heavy synchronisation.
    //   - Recommended: Δ ≈ average edge length (meters), or 1/√m for theoretical optimality.
    //
    // Parallelism upgrade path (ROGUECITY_ROUTING_PARALLEL):
    //   Replace the inner bucket-processing loop with a parallel_for over the
    //   current bucket S.  Use thread-local candidate arrays to collect new
    //   (vertex, distance) pairs, then merge them with atomic min operations
    //   before the next bucket.  Add Crauser OUT labels (computeCrauserLabels)
    //   to determine the barrier-free settling frontier.
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
    //
    // Reference: M. Pascoal, "A new implementation of Yen's ranking loopless
    //            paths algorithm", 4OR 1(2):121–134, 2003.
    //
    // Current status: PLACEHOLDER — always returns empty candidates.
    //
    // The Pascoal algorithm improves upon Yen by avoiding the re-scanning of
    // previously accepted paths during the deviation-arc enumeration.  It uses
    // a "deviation" pointer per candidate and processes arcs in topological order,
    // reducing the constant factor by roughly 30–50 % on dense graphs.
    //
    // Implementation guide (when ready to implement):
    //   1. Represent each candidate as a (path, deviation_arc) pair.
    //   2. Build a deviation-arc tree rooted at the shortest path P₁.
    //   3. For each accepted path P_i, extend the tree with new deviation arcs
    //      from P_i's deviation point onward.
    //   4. Maintain a min-heap of partial extension costs; pop the cheapest
    //      extension, reconstruct the full candidate, add to output.
    //   5. Guard against cycles with path.size() > graph.vertices().size() check.
    //
    // Caller contract (preserved even after full implementation):
    //   An empty result must degrade gracefully — callers should fall back to
    //   yenKShortestPaths() or another candidate strategy.
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
    //
    // Thin wrapper around runDijkstra() with the edge_allowed mask populated
    // from the caller's edge_flags vector.
    //
    // Invariant: edges with index >= edge_flags.size() are treated as allowed
    // (permissive default) so that callers do not need to pre-size edge_flags to
    // exactly graph.edges().size() when only blocking a subset of edges.
    //
    // Usage pattern (incremental road construction):
    //   std::vector<bool> flags(graph.edges().size(), false); // all blocked
    //   for (EdgeID eid : committed_edges) { flags[eid] = true; }
    //   auto route = edgeFlagDijkstra(graph, depot, destination, flags);
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
    //
    // Reference: Barrett et al., "Formal-Language-Constrained Path Problems",
    //            SIAM Journal on Computing 30(3):809–837, 2000.
    //
    // Product-graph construction:
    //   The search space is the Cartesian product G × A of the road graph and
    //   a DFA A.  A product-state (v, s) represents "at road vertex v with the
    //   automaton in state s".  Edges in the product graph are:
    //     ((u, s), (v, s')) with weight w(u,v)  iff transition(s, edge(u,v)) == s'.
    //   Dead transitions (return -1) omit the edge entirely.
    //
    // Memory: O(n · |A|) for dist / prev arrays.  For large automata (|A| > 100)
    //   consider sparse representation or trie-compressed state indexing.
    //
    // Typical automaton patterns for city road generation:
    //   - "No U-turn": 2 states tracking last-used edge direction.
    //   - "Highway-only segment": 3 states (off-highway, on-highway, dead).
    //   - "Mandatory waypoint": k+1 states for a sequence of k mandatory landmarks.
    //
    // Fallback: if automaton.transition is null or num_states ≤ 0, the single
    //   accepting state {0} means every path is valid → delegates to shortestPath().
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
    //
    // Reference: Andreas Crauser et al., "A Parallelization of Dijkstra's
    //            Shortest Path Algorithm", MFCS 1998, LNCS 1450:722–731.
    //
    // The OUT-criterion:
    //   A vertex u in the priority queue can be settled in parallel with other
    //   unsettled queue vertices if:
    //     dist[u] ≤ min_{v in queue, v ≠ u} (dist[v] + out_lower_bound[u])
    //   where out_lower_bound[u] = min weight of edges leaving u.
    //
    // This implementation only computes out_lower_bound[] (the label).
    // The actual settling logic belongs inside a parallel Dijkstra or
    // delta-stepping loop.  To wire it in:
    //
    //   CrauserLabels labels = computeCrauserLabels(graph);
    //   // Inside parallel Dijkstra inner loop:
    //   double threshold = dist[u] - labels.out_lower_bound[u];
    //   // Any queue vertex v with dist[v] > threshold cannot block u → settle u.
    //
    // Note: for undirected graphs, "outgoing" and "incoming" edge sets are
    // identical, so out_lower_bound[v] == in_lower_bound[v].  The IN-criterion
    // (settling u if dist[u] ≤ min_{v settled} dist[v] + out_lower_bound[u])
    // is also safe; consider implementing both for maximum parallelism.
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
    //
    // Reference: Holger Bast et al., "Fast Routing in Road Networks with Transit
    //            Nodes", Science 316(5824):566, 2007.
    //
    // Query algorithm (all data pre-built):
    //   cost(src, dst) = min_{i,j} ( forward_access[src][i]
    //                              + dist_table[i][j]
    //                              + backward_access[dst][j] )
    //
    // This is O(|nodes|²) per query, typically sub-millisecond for city-scale
    // graphs with |nodes| ≈ 100–500 transit vertices.
    //
    // Locality filter (not yet implemented):
    //   For short-range queries (src and dst within the same district or
    //   within a few hops), TNR can give wrong results because neither src nor
    //   dst has a meaningful access path to any transit node.  The standard
    //   fix is a locality test:  if dist(src, dst) < local_threshold, skip TNR
    //   and use a plain Dijkstra or BFS instead.  Add a `local_threshold_meters`
    //   parameter to TransitNodeSet and check it at the top of transitNodeQuery().
    //
    // Predecessor reconstruction:
    //   The current implementation returns a synthetic vertex list
    //   [src, tn_i, tn_j, dst].  Full reconstruction requires storing predecessor
    //   trees from each Dijkstra run during the build phase.  Add
    //   forward_prev[v][i]  (predecessor on the path from v to nodes[i]) and
    //   backward_prev[v][j] (predecessor on the path from nodes[j] to v) to
    //   TransitNodeSet, then splice the three sub-paths in transitNodeQuery().
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
    //
    // "External memory" refers to the sequential memory access pattern: vertices
    // are processed strictly in FIFO order using a deque, matching the access
    // pattern expected by external-memory or disk-backed graph algorithms where
    // random-access priority-queue operations would cause excessive page faults.
    //
    // In-memory usage in this codebase:
    //   - Connectivity check: run BFS from any vertex; count level[v] == -1.
    //   - Island detection: collect all unreachable vertices and link them via
    //     synthetic short edges during the road-repair pass.
    //   - Hop-distance proxy: use level[v] as a fast approximation to dist[v]
    //     when edges have uniform weight (e.g. early-stage block layout).
    //   - Spanning tree: the prev[] array encodes the BFS tree; use it to draw
    //     the "skeleton" of the network in the debug overlay.
    //
    // To handle disconnected graphs:
    //   Run externalMemoryBFS once per connected component.  Detect component
    //   boundaries by scanning for level[v] == -1 after each BFS call, then
    //   picking any unvisited vertex as the next source.
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
    // 10. Two-Level Arc-Flags Dijkstra
    // =========================================================================
    //
    // Reference: Ulrich Lauther, "An Extremely Fast, Exact Algorithm for Finding
    //            Shortest Paths in Static Networks with Geographical Background",
    //            GeoInfo 2004.
    //            Reinhard Möhring et al., "Partitioning Graphs to Speedup Dijkstra's
    //            Algorithm", ACM JEA 11, 2006.
    //
    // Arc-flag build algorithm (to be implemented in RoutingPrecompute.cpp):
    //   For each region r (e.g. district identified by Vertex::layer_id):
    //     For each boundary vertex b of region r:
    //       Run reverse Dijkstra from b (or forward Dijkstra to b on
    //       the reversed graph).
    //       For every edge (u,v) on the shortest-path tree rooted at b:
    //         flags[edge_id(u,v)][r] = true.
    //
    //   Boundary vertices: vertices v where ∃ edge (v,w) with w in a different region.
    //
    // Two-level extension (Möhring 2006):
    //   Partition the city first into "cells" (e.g. 4×4 grid of districts),
    //   then into "regions" (groups of cells).  Build flags at both levels.
    //   During a query, first prune with cell-level flags, then with region flags.
    //   This two-level hierarchy reduces flag storage from O(m·R) to O(m·√R).
    //
    // Current implementation: single-level (one flag per edge per region).
    // To upgrade to two-level: add a second ArcFlagsData for the coarser partition
    // and check both in the inner loop (check coarser first — cheaper to evaluate).
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
    //
    // Reference: Andrew V. Goldberg & Renato Werneck, "Computing Point-to-Point
    //            Shortest Paths from External Memory", ALENEX 2005.
    //
    // Two orthogonal speedup layers combined:
    //
    // --- Reach (Gutman 2004) ---
    //   Reach R(v) = max over all shortest paths P through v of
    //                min(dist(first(P), v), dist(v, last(P))).
    //   A vertex u can be skipped during a query (src→dst) if:
    //     g_cost[u] > reach[u]   AND   h(u) > reach[u]
    //   where g_cost[u] = distance from src so far,
    //         h(u)       = ALT lower bound on dist(u, dst).
    //
    //   WHY this is safe: if R(v) is small, v does not appear on any long path.
    //   A path from src to dst with g_cost[u] > R(u) would mean u is "too far from
    //   the start" relative to its reach, and h(u) > R(u) means "u is also far from
    //   the end" — so u cannot lie on the src→dst shortest path.
    //
    // --- ALT (Goldberg & Harrelson 2005) ---
    //   Lower bound:  h(v) = max_L |dist(L, dst) - dist(L, v)|
    //   This is admissible by the triangle inequality:
    //     dist(v, dst) ≥ dist(L, dst) - dist(L, v)  for any landmark L.
    //
    //   Good landmark placement (avoid heuristic, Goldberg 2005):
    //     1. Pick any vertex as landmark 0.
    //     2. Greedily add the vertex farthest from all existing landmarks
    //        (using SSSP distances, not Euclidean).
    //     3. Repeat until |landmarks| == target count (4–16 for city-scale).
    //
    // Reach value build (to be implemented in RoutingPrecompute.cpp):
    //   Full APSP is O(n · (n + m) log n) — expensive for large graphs.
    //   The pruned-reach algorithm (Goldberg 2005) computes approximate
    //   reach values via partial shortest-path trees with an upper-bound
    //   pruning criterion, reducing the effective work to O(n^1.5) in practice.
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
