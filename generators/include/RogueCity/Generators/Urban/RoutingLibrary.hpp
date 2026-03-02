#pragma once

// Additive routing library – exposes advanced graph-routing algorithm adapters
// built on top of the existing Graph/GraphAlgorithms stack.
// All types live under RogueCity::Generators::Urban::Routing so they do not
// pollute the parent Urban namespace.
//
// Invariants preserved:
//  - Edge weight is always edge.length (meters).
//  - No existing Graph or GraphAlgorithms APIs are modified.
//  - Algorithms with incomplete full implementations provide safe fallbacks to
//    GraphAlgorithms::shortestPath so callers always get a valid (possibly empty)
//    PathResult.

#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <functional>
#include <limits>
#include <vector>

namespace RogueCity::Generators::Urban::Routing {

    // =========================================================================
    // 1. Yen's K-Shortest Loopless Paths
    //    Returns up to k loopless shortest paths from src to dst (ordered by
    //    ascending cost).  Paths may share vertices but are distinct sequences.
    // =========================================================================
    struct KPathsResult {
        std::vector<PathResult> paths; // up to k entries, sorted by cost
    };

    [[nodiscard]] KPathsResult yenKShortestPaths(
        const Graph& g,
        VertexID     src,
        VertexID     dst,
        int          k);

    // =========================================================================
    // 2. Multi-Level Highway Hierarchies – road-level extraction
    //    Partitions vertices and edges into num_levels tiers based on edge type
    //    and layer_id metadata already present on the Graph.
    // =========================================================================
    struct HierarchyLevel {
        int                  level = 0;
        std::vector<VertexID> highway_vertices;
        std::vector<EdgeID>   highway_edges;
    };

    [[nodiscard]] std::vector<HierarchyLevel> extractHighwayHierarchy(
        const Graph& g,
        int          num_levels = 3);

    // =========================================================================
    // 3. Delta-Stepping Distance Field (single-threaded baseline)
    //    Computes shortest distances from src to all vertices using bucket-based
    //    relaxation.  dist[v] == infinity means unreachable.
    // =========================================================================
    struct DistanceField {
        std::vector<double> dist; // indexed by VertexID
    };

    [[nodiscard]] DistanceField deltaSteppingDistanceField(
        const Graph& g,
        VertexID     src,
        double       delta = 1.0);

    // =========================================================================
    // 4. Loopless Candidate Generation (Pascoal) – safe placeholder
    //    Returns empty candidates; a full implementation can replace this body.
    //    Callers must treat an empty result as "no additional candidates found".
    // =========================================================================
    struct LooplessCandidates {
        std::vector<PathResult> candidates; // empty until full implementation
    };

    [[nodiscard]] LooplessCandidates looplessCandidates(
        const Graph& g,
        VertexID     src,
        VertexID     dst,
        int          max_candidates = 10);

    // =========================================================================
    // 5. Edge-Flag Dijkstra
    //    Standard Dijkstra that only traverses edges whose flag is true.
    //    edge_flags must have size >= g.edges().size(); missing flags default to
    //    true (permissive fallback).
    // =========================================================================
    [[nodiscard]] PathResult edgeFlagDijkstra(
        const Graph&             g,
        VertexID                 src,
        VertexID                 dst,
        const std::vector<bool>& edge_flags);

    // =========================================================================
    // 6. Formal-Language-Constrained Shortest Path
    //    Runs Dijkstra in the product of the graph and a deterministic
    //    finite automaton.  The automaton transition is provided by the caller.
    //    Only paths that end in an accepting automaton state are considered.
    // =========================================================================
    struct AutomatonConstraint {
        // Number of states in the automaton.
        int num_states = 1;

        // Initial automaton state.
        int initial_state = 0;

        // Set of accepting states (path must terminate in one of these).
        std::vector<int> accepting_states = {0};

        // Transition function: (current_state, edge_id) -> next_state.
        // Return -1 to indicate a non-accepting (dead) transition.
        std::function<int(int, EdgeID)> transition;
    };

    [[nodiscard]] PathResult constrainedShortestPath(
        const Graph&              g,
        VertexID                  src,
        VertexID                  dst,
        const AutomatonConstraint& automaton);

    // =========================================================================
    // 7. Crauser OUT-Criterion Helpers
    //    Computes, for each vertex, the minimum outgoing edge weight used as
    //    a lower-bound threshold to identify vertices safe to settle
    //    independently in parallel Dijkstra variants.
    // =========================================================================
    struct CrauserLabels {
        // out_lower_bound[v] = min weight of edges leaving v (infinity if no edges).
        std::vector<double> out_lower_bound;
    };

    [[nodiscard]] CrauserLabels computeCrauserLabels(const Graph& g);

    // =========================================================================
    // 8. Transit Node Routing Query
    //    Given a pre-built transit node set with a distance table, answers a
    //    query in O(|transit_nodes|) time for long-range queries.
    //    Falls back to GraphAlgorithms::shortestPath when the transit node set
    //    is empty or covers neither src nor dst.
    // =========================================================================
    struct TransitNodeSet {
        std::vector<VertexID>              nodes;
        // dist_table[i][j] = shortest distance from nodes[i] to nodes[j].
        std::vector<std::vector<double>>   dist_table;
        // forward_access[v]  = dist from vertex v  to each transit node (indexed by node order).
        std::vector<std::vector<double>>   forward_access;
        // backward_access[v] = dist from each transit node to vertex v.
        std::vector<std::vector<double>>   backward_access;
    };

    [[nodiscard]] PathResult transitNodeQuery(
        const Graph&         g,
        VertexID             src,
        VertexID             dst,
        const TransitNodeSet& tns);

    // =========================================================================
    // 9. External Memory BFS Helper
    //    BFS that records discovery level for every reachable vertex.
    //    level[v] == -1 means unreachable.
    // =========================================================================
    struct BFSResult {
        std::vector<int>      level; // BFS level, -1 = unvisited
        std::vector<VertexID> prev;  // predecessor vertex, max() = none
    };

    [[nodiscard]] BFSResult externalMemoryBFS(
        const Graph& g,
        VertexID     src);

    // =========================================================================
    // 10. Two-Level Arc-Flags Helper
    //     Dijkstra that prunes edges whose arc-flag for the target region is
    //     false.  flags.flags[edge_id][region_id] == true means the edge is on
    //     a shortest path to some vertex in region_id.
    //     Falls back to a normal traversal when flags is empty.
    // =========================================================================
    struct ArcFlagsData {
        // flags[edge_id][region_id] – may be empty (disables pruning).
        std::vector<std::vector<bool>> flags;
        int                            num_regions = 0;
    };

    [[nodiscard]] PathResult arcFlagsDijkstra(
        const Graph&        g,
        VertexID            src,
        VertexID            dst,
        const ArcFlagsData& arc_flags,
        int                 target_region = 0);

    // =========================================================================
    // 11. REAL (Reach + ALT) A* Search
    //     A* using reach-based pruning and ALT (A* with Landmarks and Triangle
    //     inequality) lower-bound heuristic.
    //     Falls back to GraphAlgorithms::shortestPath when reach values or
    //     landmark distances are absent.
    // =========================================================================
    struct REALData {
        // reach[v] = reach value for vertex v; empty => use shortestPath fallback.
        std::vector<double>              reach;
        // landmarks[i] = vertex ID of landmark i.
        std::vector<VertexID>            landmarks;
        // landmark_dist[i][v] = shortest distance from landmark i to vertex v.
        std::vector<std::vector<double>> landmark_dist;
    };

    [[nodiscard]] PathResult realAStar(
        const Graph&   g,
        VertexID       src,
        VertexID       dst,
        const REALData& real_data);

} // namespace RogueCity::Generators::Urban::Routing
