#pragma once

/// @file RoutingLibrary.hpp
/// @brief Additive routing-algorithm adapters for the RogueCity Urban graph stack.
///
/// This module exposes eleven advanced routing algorithms as free functions and
/// supporting data structs scoped under RogueCity::Generators::Urban::Routing.
/// All algorithms operate on the existing Urban::Graph / Vertex / Edge types and
/// reuse GraphAlgorithms::shortestPath / internal Dijkstra helpers; no existing
/// API is modified.
///
/// ## Architecture position
/// ```
/// generators/Urban/Graph + GraphAlgorithms  (existing base layer)
///        ↓
/// generators/Urban/RoutingLibrary           (this file – additive layer)
///        ↓
/// generators/Pipeline/CityGenerator         (calls Routing:: APIs as needed)
/// ```
///
/// ## Invariants preserved across all algorithms
/// - Edge weight is always `edge.length` (meters), clamped to ≥ 0.001 m.
/// - No existing Graph or GraphAlgorithms signatures are modified.
/// - Algorithms that require pre-built auxiliary data (TNR, REAL, arc-flags) fall
///   back to GraphAlgorithms::shortestPath when that data is absent, so callers
///   always receive a valid (possibly empty) PathResult without crashing.
/// - All result structs are value types; they are safe to copy or move.
///
/// ## Implementation status
/// | # | Algorithm                          | Status           |
/// |---|------------------------------------|------------------|
/// | 1 | Yen's K-Shortest Paths             | Full             |
/// | 2 | Multi-Level Highway Hierarchies    | Full             |
/// | 3 | Delta-Stepping Distance Field      | Full (1-thread)  |
/// | 4 | Loopless Candidates (Pascoal)      | Placeholder      |
/// | 5 | Edge-Flag Dijkstra                 | Full             |
/// | 6 | FL-Constrained Shortest Path       | Full             |
/// | 7 | Crauser OUT-Criterion Labels       | Full             |
/// | 8 | Transit Node Routing               | Full (no rebuild)|
/// | 9 | External Memory BFS                | Full             |
/// |10 | Two-Level Arc-Flags Dijkstra       | Full             |
/// |11 | REAL (Reach + ALT) A*              | Full             |

// clang-format off
#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <functional>
#include <limits>
#include <vector>
// clang-format on

namespace RogueCity::Generators::Urban::Routing {

    // =========================================================================
    // 1. Yen's K-Shortest Loopless Paths
    // =========================================================================

    /// Result container for Yen's algorithm.
    /// Paths are sorted in ascending order of cost (cheapest first).
    struct KPathsResult {
        /// Up to k loopless paths from src to dst, sorted by ascending cost.
        /// May contain fewer than k entries when fewer loopless paths exist.
        std::vector<PathResult> paths;
    };

    /// Compute up to k loopless (simple) shortest paths from @p src to @p dst.
    ///
    /// Implements Yen's algorithm (1971): each iteration identifies a "spur node"
    /// on the last confirmed path, temporarily removes edges/vertices used by
    /// previously accepted paths, runs a restricted Dijkstra from the spur node,
    /// and concatenates the root prefix with the new spur segment.
    ///
    /// @param g   Road graph. Must be undirected (edges stored on both endpoints).
    /// @param src Source vertex ID.
    /// @param dst Destination vertex ID.
    /// @param k   Maximum number of paths to return (k ≥ 1).
    /// @return    KPathsResult with up to k entries sorted by cost.
    ///            Empty if @p src == @p dst, graph is disconnected, or k ≤ 0.
    ///
    /// @note Complexity: O(k · n · (m + n log n)) where n = |V|, m = |E|.
    ///
    /// ### Generator integration
    /// Use this to offer the player multiple route options between two city
    /// landmarks (e.g. port ↔ downtown).  Feed the top-3 paths into a
    /// road-type-assignment pass where path rank influences lane count and
    /// road class.  The path cost difference between rank-1 and rank-k can
    /// drive a "congestion premium" heuristic.
    [[nodiscard]] KPathsResult yenKShortestPaths(
        const Graph& g,
        VertexID     src,
        VertexID     dst,
        int          k);

    // =========================================================================
    // 2. Multi-Level Highway Hierarchies
    // =========================================================================

    /// One tier in a road hierarchy decomposition.
    /// Level 0 = lowest-order roads (Streets), higher levels = more important.
    struct HierarchyLevel {
        /// Zero-based level index; 0 = street-level, (num_levels-1) = highway.
        int level = 0;

        /// All vertex IDs that participate in at least one edge at this level.
        std::vector<VertexID> highway_vertices;

        /// All edge IDs classified at this level.
        std::vector<EdgeID> highway_edges;
    };

    /// Partition the graph into @p num_levels road-class tiers.
    ///
    /// Level assignment uses `edge.type` cast to an integer plus `edge.layer_id`
    /// as a secondary boost, clamped to [0, num_levels-1].  This mirrors the
    /// Core::RoadType ordering (Street=0 … Highway=4+).
    ///
    /// @param g          Road graph to decompose.
    /// @param num_levels Number of hierarchy tiers (default 3: street/arterial/highway).
    /// @return           Vector of HierarchyLevel, one per tier, index == level.
    ///
    /// @note Complexity: O(m) where m = |E|.
    ///
    /// ### Generator integration
    /// Call this after the road-noding / graph-simplification passes to produce
    /// a hierarchy that feeds:
    /// - The Multi-Level Dijkstra query (precompute shortcuts on upper levels).
    /// - The visualizer LOD system (render only top levels at low zoom).
    /// - District-density scoring (higher-level roads attract denser zoning).
    ///
    ///   Store contracted shortcuts in a separate `shortcut_edges` vector on
    ///   HierarchyLevel (tagged with the contracted-away vertex) rather than
    ///   inserting them into Graph.edges() directly.  This preserves the base
    ///   graph intact for rendering and plan-validation, and lets the CH query
    ///   use the shortcut layer only during routing unpacking.
    ///
    /// ### To extend: full contraction hierarchies
    /// Replace the type-based bucketing with a proper node-ordering by edge
    /// betweenness (sampledEdgeCentrality).  Add a contraction loop that inserts
    /// shortcut edges between each vertex's up-neighbours, storing them in an
    /// augmented graph that keeps the base graph intact.
    [[nodiscard]] std::vector<HierarchyLevel> extractHighwayHierarchy(
        const Graph& g,
        int          num_levels = 3);

    // =========================================================================
    // 3. Delta-Stepping Distance Field
    // =========================================================================

    /// All-pairs (single-source / all-targets) distance result.
    struct DistanceField {
        /// dist[v] = shortest distance (meters) from the source vertex to vertex v.
        /// dist[src] == 0.0.  Unreachable vertices hold std::numeric_limits<double>::infinity().
        /// Indexed by VertexID; size == graph.vertices().size().
        std::vector<double> dist;
    };

    /// Compute shortest distances from @p src to every reachable vertex using
    /// the Delta-Stepping algorithm (Meyer & Sanders 2003).
    ///
    /// Delta-Stepping organises vertices into buckets of width @p delta.
    /// "Light" edges (weight ≤ delta) are relaxed within the current bucket;
    /// "heavy" edges (weight > delta) propagate to a later bucket.  This
    /// single-threaded baseline preserves the bucket structure for a future
    /// parallel (OpenMP / std::async) upgrade.
    ///
    /// @param g     Road graph.
    /// @param src   Source vertex ID.
    /// @param delta Bucket width (meters, default 1.0).  Smaller values improve
    ///              parallelism but increase bucket overhead.  A good heuristic
    ///              is the average edge length of the graph.
    /// @return      DistanceField with dist[v] for all v.
    ///
    /// @note Complexity (single-threaded): O(n + m + (L/delta) · m) where
    ///       L = maximum shortest path length.  For uniform-weight graphs with
    ///       delta ≈ average weight this degrades gracefully to O(n + m).
    ///
    /// ### Generator integration
    /// Use the distance field to build accessibility maps:
    /// - Isochronal rings around transit stops or city entrances.
    /// - Per-vertex "distance-to-CBD" values fed into lot/district scoring.
    /// - Heatmap overlay for the visualizer (feed dist[] directly to the LOD
    ///   colour shader as a uniform buffer).
    ///
    /// ### To extend: parallel delta-stepping
    /// Partition bucket processing across std::async tasks.  Each task owns a
    /// thread-local tentative-distance array; merge via atomic min at bucket
    /// boundaries.  Add a `uint32_t num_threads` parameter (default 1) and guard
    /// the parallel path behind ROGUECITY_ROUTING_PARALLEL.
    [[nodiscard]] DistanceField deltaSteppingDistanceField(
        const Graph& g,
        VertexID     src,
        double       delta = 1.0);

    // =========================================================================
    // 4. Loopless Candidate Generation (Pascoal) – placeholder
    // =========================================================================

    /// Container for enumerated loopless path candidates.
    struct LooplessCandidates {
        /// Candidate paths from src to dst. Empty until full Pascoal
        /// enumeration is implemented (see looplessCandidates() notes).
        std::vector<PathResult> candidates;
    };

    /// Enumerate loopless path candidates using the Pascoal (2003) algorithm.
    ///
    /// @par Current status: **safe placeholder**
    /// The function always returns an empty LooplessCandidates.  Callers must
    /// treat an empty result as "no additional candidates found" and degrade
    /// gracefully (e.g. fall back to Yen's yenKShortestPaths()).
    ///
    /// @param g              Road graph.
    /// @param src            Source vertex ID.
    /// @param dst            Destination vertex ID.
    /// @param max_candidates Maximum number of candidates to enumerate.
    /// @return               Empty LooplessCandidates until implemented.
    ///
    /// ### To implement (Pascoal 2003)
    /// The Pascoal algorithm maintains a candidate set using a deviation-arc
    /// enumeration that avoids repeated edges without requiring the full Yen
    /// path-copy overhead.  Implementation steps:
    /// 1. Compute the shortest loopless path P₁ via yenKShortestPaths(g,src,dst,1).
    /// 2. For each edge (u,v) on P₁, temporarily mark (u,v) as forbidden and
    ///    run a constrained Dijkstra from u to dst.
    /// 3. Combine the prefix P₁[src…u] with the new suffix to form a candidate.
    /// 4. Insert unique candidates into a min-heap ordered by total cost.
    /// 5. Pop the best candidate, add to result, generate its deviations, repeat.
    /// This produces candidates in non-decreasing order up to max_candidates.
    ///
    /// ### Generator integration
    /// Planned use: diversified route selection during the major-connector-graph
    /// phase (MajorConnectorGraph.cpp).  Multiple loopless candidates seed the
    /// road-type assignment pass with structurally distinct alternatives, reducing
    /// visual repetitiveness in generated city layouts.
    [[nodiscard]] LooplessCandidates looplessCandidates(
        const Graph& g,
        VertexID     src,
        VertexID     dst,
        int          max_candidates = 10);

    // =========================================================================
    // 5. Edge-Flag Dijkstra
    // =========================================================================

    /// Standard Dijkstra restricted to a caller-defined subset of edges.
    ///
    /// Each entry in @p edge_flags corresponds to an EdgeID (index into
    /// `graph.edges()`).  Edges with flag == false are skipped entirely.
    /// If @p edge_flags is shorter than the edge count, out-of-bounds edges
    /// are treated as allowed (permissive default).
    ///
    /// @param g          Road graph.
    /// @param src        Source vertex ID.
    /// @param dst        Destination vertex ID.
    /// @param edge_flags Per-edge boolean mask (true = traversable).
    /// @return           Shortest PathResult using only allowed edges, or an
    ///                   unreachable PathResult if no path exists.
    ///
    /// @note Complexity: O((n + m) log n) in the worst case over allowed edges.
    ///
    /// ### Generator integration
    /// Primary use case: **road-construction phase gates**.
    /// - Mark completed road edges as allowed; under-construction edges as blocked.
    /// - During incremental generation passes, restrict routing to the current
    ///   "built" subgraph, then extend the flag array as new edges are committed.
    /// - Also useful for enforcing "no-go zones" (e.g. water body interiors):
    ///   set flags[eid] = false for any edge whose midpoint lies in a water mask.
    [[nodiscard]] PathResult edgeFlagDijkstra(
        const Graph&             g,
        VertexID                 src,
        VertexID                 dst,
        const std::vector<bool>& edge_flags);

    // =========================================================================
    // 6. Formal-Language-Constrained Shortest Path
    // =========================================================================

    /// A deterministic finite automaton (DFA) that constrains which edge
    /// sequences are valid for a constrained-shortest-path query.
    ///
    /// WHY: Real city routing often carries "road type grammar" constraints.
    /// For example: "a valid route must not switch from a Highway to a Street
    /// and back to a Highway" (no yo-yo road class changes).  Encoding this as
    /// a DFA allows a single Dijkstra pass over the product graph to enforce it.
    struct AutomatonConstraint {
        /// Total number of automaton states. Must be ≥ 1.
        int num_states = 1;

        /// State the automaton starts in at the source vertex (0-based).
        int initial_state = 0;

        /// States in which the path is considered valid at the destination.
        /// A path is only accepted if the automaton finishes in one of these states.
        std::vector<int> accepting_states = {0};

        /// Edge-label transition function: `(current_state, edge_id) → next_state`.
        ///
        /// Return -1 (or any value outside [0, num_states)) to indicate a
        /// forbidden (dead) transition — the algorithm will not explore paths
        /// that use this edge from this automaton state.
        ///
        /// The function must be thread-safe (it is called once per edge relaxation
        /// from the priority queue).
        std::function<int(int, EdgeID)> transition;
    };

    /// Find the shortest path from @p src to @p dst that is accepted by @p automaton.
    ///
    /// Runs Dijkstra over the product graph G × A where G is the road graph and
    /// A is the DFA.  A product-state (v, s) is settled when vertex v is reached
    /// with automaton in state s.  Only product-states where v == dst and s is an
    /// accepting state are considered destinations.
    ///
    /// Falls back to GraphAlgorithms::shortestPath() if @p automaton.transition
    /// is null or @p automaton.num_states ≤ 0.
    ///
    /// @param g        Road graph.
    /// @param src      Source vertex ID.
    /// @param dst      Destination vertex ID.
    /// @param automaton DFA describing the valid edge-sequence language.
    /// @return         Shortest accepted PathResult, or empty if none exists.
    ///
    /// @note Complexity: O((n·|A| + m·|A|) log(n·|A|)) where |A| = num_states.
    ///
    /// ### Generator integration
    /// **Road-class grammar enforcement during route generation:**
    /// - Build a 3-state DFA: state 0 = on any road, state 1 = on Highway,
    ///   state 2 = dead (tried to re-enter highway after leaving it once).
    ///   Transition: (0, Highway_edge) → 1; (1, non-Highway) → 0; (1, Highway) → 1;
    ///   (0, non-Highway) → 0.  Accepting: {0, 1}.
    /// - This prevents connector routes that dip in and out of highway segments,
    ///   producing more realistic urban-arterial topology.
    [[nodiscard]] PathResult constrainedShortestPath(
        const Graph&               g,
        VertexID                   src,
        VertexID                   dst,
        const AutomatonConstraint& automaton);

    // =========================================================================
    // 7. Crauser OUT-Criterion Helpers
    // =========================================================================

    /// Per-vertex labels used by Crauser's OUT-criterion for parallel Dijkstra.
    ///
    /// The OUT-criterion states: a vertex u may be settled in parallel with other
    /// vertices in the priority queue if dist[u] ≤ dist[v] + out_lower_bound[u]
    /// for all v currently in the queue.  The lower bound is the minimum outgoing
    /// edge weight from u, which bounds how far any path through u can still improve.
    struct CrauserLabels {
        /// out_lower_bound[v] = minimum weight among all edges incident to vertex v.
        /// Equal to std::numeric_limits<double>::infinity() for isolated vertices.
        /// Indexed by VertexID; size == graph.vertices().size().
        std::vector<double> out_lower_bound;
    };

    /// Pre-compute Crauser OUT-criterion labels for every vertex in @p g.
    ///
    /// Iterates over all edges once to find each vertex's minimum incident edge
    /// weight.  The result is used as a per-vertex lower bound during parallel
    /// Dijkstra to identify vertices that can be relaxed concurrently without
    /// violating the optimal-substructure property.
    ///
    /// @param g Road graph.
    /// @return  CrauserLabels with out_lower_bound[v] for all v.
    ///
    /// @note Complexity: O(m) time, O(n) space.
    ///
    /// ### Generator integration
    /// **Future parallel Dijkstra / delta-stepping upgrade path:**
    /// When deltaSteppingDistanceField() is upgraded to a parallel version
    /// (ROGUECITY_ROUTING_PARALLEL), pair it with computeCrauserLabels():
    /// - Precompute labels once after graph construction (cheap, O(m)).
    /// - Use labels inside the parallel bucket loop to identify the "safe frontier"
    ///   of vertices that can be settled without a global barrier.
    /// - This can cut synchronisation overhead by ~30-50 % on sparse graphs
    ///   compared to pure barrier-per-bucket strategies.
    [[nodiscard]] CrauserLabels computeCrauserLabels(const Graph& g);

    // =========================================================================
    // 8. Transit Node Routing
    // =========================================================================

    /// Pre-built Transit Node Routing (TNR) auxiliary data.
    ///
    /// TNR (Bast et al. 2007) accelerates long-range queries by pre-computing
    /// distances between a small set of "important" vertices (transit nodes, e.g.
    /// highway on-ramps or major interchange vertices) and storing:
    /// - A dense distance table between all pairs of transit nodes.
    /// - For each vertex, the distances to/from each transit node ("access distances").
    ///
    /// @par Populating this struct (build phase — not yet automated)
    /// 1. Choose transit nodes: vertices with high betweenness centrality
    ///    (use sampledEdgeCentrality) or all vertices on the top Highway-hierarchy tier.
    /// 2. For each transit node t_i, run dijkstraAll(g, t_i) to fill
    ///    forward_access[v][i] = dist(v, t_i) and dist_table[i][j] = dist(t_i, t_j).
    /// 3. backward_access[v][j] = dist(t_j, v) (same Dijkstra, reversed direction,
    ///    or identical for undirected graphs).
    ///
    /// Leaving any field empty causes transitNodeQuery() to fall back to
    /// GraphAlgorithms::shortestPath(), so partial construction is safe.
    struct TransitNodeSet {
        /// Vertex IDs of the chosen transit nodes.
        std::vector<VertexID> nodes;

        /// dist_table[i][j] = shortest distance (meters) between nodes[i] and nodes[j].
        /// Size: nodes.size() × nodes.size().
        std::vector<std::vector<double>> dist_table;

        /// forward_access[v][i] = shortest distance (meters) from vertex v to nodes[i].
        /// Size: graph.vertices().size() × nodes.size().
        std::vector<std::vector<double>> forward_access;

        /// backward_access[v][j] = shortest distance (meters) from nodes[j] to vertex v.
        /// For undirected graphs this equals forward_access[v][j].
        /// Size: graph.vertices().size() × nodes.size().
        std::vector<std::vector<double>> backward_access;
    };

    /// Answer a point-to-point query using the Transit Node Routing table.
    ///
    /// For each pair (transit node i, transit node j), the total path cost is
    /// forward_access[src][i] + dist_table[i][j] + backward_access[dst][j].
    /// The minimum over all such pairs is returned.
    ///
    /// Falls back to GraphAlgorithms::shortestPath() when @p tns is empty or
    /// does not cover @p src / @p dst.
    ///
    /// @param g   Road graph (used only for the shortestPath fallback).
    /// @param src Source vertex ID.
    /// @param dst Destination vertex ID.
    /// @param tns Pre-built transit node data (may be empty → fallback).
    /// @return    PathResult with cost from TNR table.  The `vertices` field
    ///            contains [src, best_tn_i, best_tn_j, dst] (synthetic; full
    ///            waypoints require per-access predecessor data).
    ///
    /// @note Query complexity: O(|transit_nodes|²) per call.
    ///       Pre-build complexity: O(|transit_nodes| · (n + m) log n).
    ///
    /// ### Generator integration
    /// - Build TransitNodeSet once after the CityGenerator road pipeline finalises.
    /// - Cache in CityGenerator::Output alongside the Graph.
    /// - Use in the visualizer for "navigate to selected landmark" queries —
    ///   these are latency-sensitive and benefit most from TNR's O(t²) speed.
    [[nodiscard]] PathResult transitNodeQuery(
        const Graph&          g,
        VertexID              src,
        VertexID              dst,
        const TransitNodeSet& tns);

    // =========================================================================
    // 9. External Memory BFS
    // =========================================================================

    /// BFS result: discovery level and predecessor for every reachable vertex.
    struct BFSResult {
        /// BFS level (hop count from src).  -1 = vertex not reachable from src.
        /// Indexed by VertexID; size == graph.vertices().size().
        std::vector<int> level;

        /// prev[v] = predecessor VertexID on the BFS tree path from src to v.
        /// std::numeric_limits<VertexID>::max() when v is the source or unreachable.
        std::vector<VertexID> prev;
    };

    /// Run a breadth-first search from @p src and record hop counts and predecessors.
    ///
    /// "External memory" refers to the algorithm's O(n + m) I/O structure that
    /// processes vertices in FIFO order without random-access priority updates,
    /// making it friendly to cache and to future disk-backed or streaming
    /// graph representations.
    ///
    /// @param g   Road graph.
    /// @param src Source vertex ID.
    /// @return    BFSResult with level[v] and prev[v] for all v.
    ///
    /// @note Complexity: O(n + m) time and space.
    ///
    /// ### Generator integration
    /// **Connectivity diagnostics and island detection:**
    /// - Call after road-noding to verify the graph is fully connected.
    ///   Any vertex with level[v] == -1 is in a disconnected island; flag
    ///   it as a generation warning and optionally link it via a new edge.
    /// - Use BFS hop-count as a fast proxy for network centrality when
    ///   edge-weight distances are not yet needed (e.g. early-stage layout).
    /// - BFS predecessor tree = minimum-hop spanning tree; useful for
    ///   visualising the "skeleton" of the road network in the overlay panel.
    [[nodiscard]] BFSResult externalMemoryBFS(
        const Graph& g,
        VertexID     src);

    // =========================================================================
    // 10. Two-Level Arc-Flags Dijkstra
    // =========================================================================

    /// Pre-built arc-flags for region-partitioned shortest-path pruning.
    ///
    /// Arc-flags (Lauther 2004, Möhring et al. 2005) pre-label each directed
    /// edge with a boolean flag per region: flags[eid][r] == true means edge eid
    /// lies on at least one shortest path to some vertex in region r.  During a
    /// query to region r, edges with flags[eid][r] == false can be skipped safely.
    ///
    /// @par Populating this struct (build phase — not yet automated)
    /// 1. Partition vertices into `num_regions` regions (e.g. grid cells,
    ///    Voronoi cells, or district IDs already present on Vertex::layer_id).
    /// 2. For each region r and each vertex v on the boundary of r, run a
    ///    reverse Dijkstra from v and set flags[eid][r] = true for every edge
    ///    eid on any shortest-path tree rooted at v.
    /// 3. An edge not on any tree to region r gets flags[eid][r] = false.
    ///
    /// Leaving `flags` empty disables pruning — the algorithm degrades to
    /// standard Dijkstra.
    struct ArcFlagsData {
        /// flags[edge_id][region_id] — true means edge_id is useful for paths
        /// leading into region_id.  May be empty (disables all pruning).
        std::vector<std::vector<bool>> flags;

        /// Total number of regions used when building the flags.  Must match
        /// the second dimension of flags[][].
        int num_regions = 0;
    };

    /// Run arc-flags-pruned Dijkstra from @p src to @p dst targeting @p target_region.
    ///
    /// Edges whose arc-flag for @p target_region is false are skipped during
    /// queue relaxation.  This can prune a large fraction of edges in well-
    /// partitioned graphs, giving near-linear query times for inter-region queries.
    ///
    /// Falls back to standard Dijkstra when @p arc_flags.flags is empty or
    /// @p target_region is out of range.
    ///
    /// @param g             Road graph.
    /// @param src           Source vertex ID.
    /// @param dst           Destination vertex ID.
    /// @param arc_flags     Pre-built arc-flag data (may be empty → normal Dijkstra).
    /// @param target_region Region index that @p dst belongs to (0-based).
    /// @return              Shortest PathResult, arc-flag-pruned when data is present.
    ///
    /// @note Query complexity: O((n' + m') log n') where n', m' are the
    ///       unpruned vertex/edge counts — often much smaller than n, m.
    ///
    /// ### Generator integration
    /// - Partition the city into districts (use Vertex::layer_id or district_id
    ///   from the district generator).  One region = one district.
    /// - Build arc-flags once after graph finalisation and store in
    ///   CityGenerator::Output (alongside the Graph and TransitNodeSet).
    /// - Use for the visualizer "route highlight" feature: when the user
    ///   selects a destination district, this API provides fast cross-district
    ///   routing with visible pruning statistics for debugging.
    [[nodiscard]] PathResult arcFlagsDijkstra(
        const Graph&        g,
        VertexID            src,
        VertexID            dst,
        const ArcFlagsData& arc_flags,
        int                 target_region = 0);

    // =========================================================================
    // 11. REAL (Reach + ALT) A* Search
    // =========================================================================

    /// Pre-built data for REAL A* (Goldberg & Werneck 2005).
    ///
    /// REAL combines two orthogonal speedup techniques:
    /// - **Reach** (Gutman 2004): vertex v has reach R(v) = the maximum over all
    ///   shortest paths P through v of min(dist(src(P), v), dist(v, dst(P))).
    ///   Vertices with low reach can be pruned when the partial path cost already
    ///   exceeds their reach value.
    /// - **ALT** (Goldberg & Harrelson 2005): A* with Landmarks and Triangle
    ///   inequality.  For each landmark L, the triangle inequality gives
    ///   h(v) ≥ |dist(L, dst) − dist(L, v)|, providing an admissible lower bound
    ///   without Euclidean coordinates.
    ///
    /// @par Populating this struct (build phase — not yet automated)
    /// **Reach values:**
    /// 1. Run all-pairs shortest paths (or sampled SSSP for large graphs).
    /// 2. For each vertex v and each shortest path P through v, update
    ///    reach[v] = max(reach[v], min(dist(src, v), dist(v, dst))).
    /// 3. For large graphs use the pruned reach algorithm (Goldberg 2005)
    ///    which avoids full APSP via partial-tree bounds.
    ///
    /// **Landmarks:**
    /// 1. Choose landmarks using the "avoid" heuristic: greedily pick the vertex
    ///    farthest from all currently selected landmarks, using SSSP distances.
    ///    4–16 landmarks is typically sufficient for city-scale graphs.
    /// 2. For each landmark L_i run dijkstraAll(g, L_i) and store
    ///    landmark_dist[i][v] = result.dist[v].
    ///
    /// Leaving `reach` or `landmark_dist` empty causes realAStar() to fall back
    /// to GraphAlgorithms::shortestPath().
    struct REALData {
        /// reach[v] = reach value (meters) for vertex v.
        /// reach[v] == 0 for terminal / dead-end vertices.
        /// Empty vector → shortestPath fallback.
        std::vector<double> reach;

        /// landmarks[i] = VertexID of the i-th landmark.
        std::vector<VertexID> landmarks;

        /// landmark_dist[i][v] = shortest distance (meters) from landmark i to vertex v.
        /// Size: landmarks.size() × graph.vertices().size().
        /// Empty / under-sized → shortestPath fallback.
        std::vector<std::vector<double>> landmark_dist;
    };

    /// Run REAL A* from @p src to @p dst using reach pruning and ALT heuristic.
    ///
    /// The ALT lower-bound h(v) = max over landmarks L of |dist(L,dst) − dist(L,v)|
    /// guides the search toward the destination.  Reach pruning skips vertices
    /// whose reach value is too small to lie on any shortest path of the required
    /// length.  Together these two techniques typically reduce settled vertices by
    /// 60–90 % compared to plain Dijkstra on large road networks.
    ///
    /// Falls back to GraphAlgorithms::shortestPath() when @p real_data is empty.
    ///
    /// @param g         Road graph.
    /// @param src       Source vertex ID.
    /// @param dst       Destination vertex ID.
    /// @param real_data Pre-built reach values and landmark distances (may be empty).
    /// @return          Shortest PathResult (reach-pruned), or shortestPath result
    ///                  if real_data is unpopulated.
    ///
    /// @note Query complexity: O((n' + m') log n') where n', m' ≪ n, m when
    ///       reach pruning is effective.
    ///
    /// ### Generator integration
    /// - Pre-build REALData once at the end of each CityGenerator::generate() call
    ///   and cache in CityGenerator::Output.
    /// - Use in simulation/agent pathfinding: NPCs, transit vehicles, and emergency
    ///   service routing all benefit from sub-millisecond query times on city-scale
    ///   graphs (thousands of vertices).
    /// - The landmark set can double as "named places": store landmark vertex IDs
    ///   alongside district names so the visualizer can display route hints like
    ///   "via Downtown Interchange".
    [[nodiscard]] PathResult realAStar(
        const Graph&    g,
        VertexID        src,
        VertexID        dst,
        const REALData& real_data);

} // namespace RogueCity::Generators::Urban::Routing

