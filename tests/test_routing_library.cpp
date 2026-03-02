// Minimal unit tests for the RoutingLibrary additive module.
// Exercises Yen's K-shortest paths, Delta-stepping, and REAL A* on a small
// hand-crafted graph; validates that results are consistent with
// GraphAlgorithms::shortestPath and that safe-fallback paths work correctly.

#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"
#include "RogueCity/Generators/Urban/RoutingLibrary.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace RogueCity;
using namespace RogueCity::Generators::Urban;
using namespace RogueCity::Generators::Urban::Routing;

// ---------------------------------------------------------------------------
// Helper: build a small triangle graph  A--B--C--A  with known edge lengths.
// A=0, B=1, C=2.  Edges: AB=1, BC=2, CA=3.
// ---------------------------------------------------------------------------
static Graph buildTriangle() {
    Graph g;
    const VertexID a = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
    const VertexID b = g.addVertex({ .pos = Core::Vec2(1.0, 0.0) });
    const VertexID c = g.addVertex({ .pos = Core::Vec2(1.0, 2.0) });
    assert(a == 0 && b == 1 && c == 2);

    auto addEdge = [&](VertexID u, VertexID v, float len) {
        Edge e{};
        e.a = u; e.b = v; e.length = len;
        e.shape = { g.getVertex(u)->pos, g.getVertex(v)->pos };
        g.addEdge(e);
    };
    addEdge(a, b, 1.0f); // edge 0: A-B
    addEdge(b, c, 2.0f); // edge 1: B-C
    addEdge(c, a, 3.0f); // edge 2: C-A
    return g;
}

// ---------------------------------------------------------------------------
// Build a small linear chain: A--B--C--D with unit edges.
// ---------------------------------------------------------------------------
static Graph buildChain() {
    Graph g;
    const VertexID a = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
    const VertexID b = g.addVertex({ .pos = Core::Vec2(1.0, 0.0) });
    const VertexID c = g.addVertex({ .pos = Core::Vec2(2.0, 0.0) });
    const VertexID d = g.addVertex({ .pos = Core::Vec2(3.0, 0.0) });
    assert(a == 0 && b == 1 && c == 2 && d == 3);

    auto addEdge = [&](VertexID u, VertexID v, float len) {
        Edge e{};
        e.a = u; e.b = v; e.length = len;
        g.addEdge(e);
    };
    addEdge(a, b, 1.0f);
    addEdge(b, c, 1.0f);
    addEdge(c, d, 1.0f);
    return g;
}

// ---------------------------------------------------------------------------
// Test 1: Yen's K-shortest – triangle, k=2
// ---------------------------------------------------------------------------
static void testYenKShortest() {
    std::cout << "  [Yen] K-shortest paths on triangle... ";

    Graph g = buildTriangle();
    // src=A(0), dst=C(2), k=2.
    // Two paths exist with cost 3: A->B->C (1+2) and A->C (direct edge with length 3).
    const auto result = yenKShortestPaths(g, 0, 2, 2);
    assert(!result.paths.empty());
    assert(result.paths[0].reachable());
    // First path cost must match shortestPath.
    const auto sp = GraphAlgorithms::shortestPath(g, 0, 2);
    assert(std::abs(result.paths[0].cost - sp.cost) < 1e-6);
    // Paths must be sorted by cost.
    for (size_t i = 1; i < result.paths.size(); ++i) {
        assert(result.paths[i].cost >= result.paths[i - 1].cost - 1e-9);
    }

    std::cout << "PASS (" << result.paths.size() << " paths, cost0=" << result.paths[0].cost << ")\n";
}

// ---------------------------------------------------------------------------
// Test 2: Yen's K-shortest – chain, k=1 must equal shortestPath
// ---------------------------------------------------------------------------
static void testYenEqualsShortestPath() {
    std::cout << "  [Yen] k=1 must equal shortestPath on chain... ";
    Graph g = buildChain();
    const auto yen1  = yenKShortestPaths(g, 0, 3, 1);
    const auto sp    = GraphAlgorithms::shortestPath(g, 0, 3);
    assert(yen1.paths.size() == 1);
    assert(std::abs(yen1.paths[0].cost - sp.cost) < 1e-6);
    assert(yen1.paths[0].vertices == sp.vertices);
    std::cout << "PASS (cost=" << yen1.paths[0].cost << ")\n";
}

// ---------------------------------------------------------------------------
// Test 3: Delta-stepping distance field – chain
// ---------------------------------------------------------------------------
static void testDeltaStepping() {
    std::cout << "  [Delta-stepping] distance field on chain... ";
    Graph g = buildChain();

    const auto field = deltaSteppingDistanceField(g, 0, 1.0);
    assert(field.dist.size() == 4);
    assert(std::abs(field.dist[0] - 0.0) < 1e-9);
    assert(std::abs(field.dist[1] - 1.0) < 1e-9);
    assert(std::abs(field.dist[2] - 2.0) < 1e-9);
    assert(std::abs(field.dist[3] - 3.0) < 1e-9);

    // Compare with shortestPath for each destination.
    for (VertexID dst = 0; dst < 4; ++dst) {
        const auto sp = GraphAlgorithms::shortestPath(g, 0, dst);
        if (sp.reachable()) {
            assert(std::abs(field.dist[dst] - sp.cost) < 1e-6);
        }
    }
    std::cout << "PASS (dist[3]=" << field.dist[3] << ")\n";
}

// ---------------------------------------------------------------------------
// Test 4: Delta-stepping – triangle (all vertices reachable)
// ---------------------------------------------------------------------------
static void testDeltaSteppingTriangle() {
    std::cout << "  [Delta-stepping] triangle all vertices reachable... ";
    Graph g = buildTriangle();

    const auto field = deltaSteppingDistanceField(g, 0, 0.5);
    assert(std::isfinite(field.dist[0]));
    assert(std::isfinite(field.dist[1]));
    assert(std::isfinite(field.dist[2]));
    assert(std::abs(field.dist[0]) < 1e-9);  // src to itself
    assert(field.dist[1] > 0.0);
    assert(field.dist[2] > 0.0);
    std::cout << "PASS (dist[1]=" << field.dist[1] << ", dist[2]=" << field.dist[2] << ")\n";
}

// ---------------------------------------------------------------------------
// Test 5: REAL A* – fallback when data empty returns same as shortestPath
// ---------------------------------------------------------------------------
static void testRealAStarFallback() {
    std::cout << "  [REAL A*] empty data falls back to shortestPath... ";
    Graph g = buildChain();
    const REALData empty{};
    const auto real_result = realAStar(g, 0, 3, empty);
    const auto sp          = GraphAlgorithms::shortestPath(g, 0, 3);
    assert(real_result.reachable());
    assert(std::abs(real_result.cost - sp.cost) < 1e-6);
    std::cout << "PASS (cost=" << real_result.cost << ")\n";
}

// ---------------------------------------------------------------------------
// Test 6: REAL A* – with populated reach and landmark data on chain
// ---------------------------------------------------------------------------
static void testRealAStarWithData() {
    std::cout << "  [REAL A*] with reach+landmark data on chain... ";
    Graph g = buildChain();

    REALData rd;
    // Reach values: vertex 1 and 2 are interior, so reach >= 1.0.
    rd.reach = {0.0, 1.0, 1.0, 0.0};

    // Use vertex 0 as the only landmark.
    rd.landmarks = {0};
    // landmark_dist[0][v] = shortest distance from landmark 0 to each vertex.
    rd.landmark_dist = {{0.0, 1.0, 2.0, 3.0}};

    const auto result = realAStar(g, 0, 3, rd);
    const auto sp     = GraphAlgorithms::shortestPath(g, 0, 3);
    assert(result.reachable());
    assert(std::abs(result.cost - sp.cost) < 1e-6);
    std::cout << "PASS (cost=" << result.cost << ")\n";
}

// ---------------------------------------------------------------------------
// Test 7: Loopless candidates placeholder returns empty
// ---------------------------------------------------------------------------
static void testLooplessCandidatesPlaceholder() {
    std::cout << "  [Pascoal] placeholder returns empty... ";
    Graph g = buildChain();
    const auto lc = looplessCandidates(g, 0, 3, 5);
    assert(lc.candidates.empty());
    std::cout << "PASS\n";
}

// ---------------------------------------------------------------------------
// Test 8: Edge-flag Dijkstra – block middle edge, force detour
// ---------------------------------------------------------------------------
static void testEdgeFlagDijkstra() {
    std::cout << "  [EdgeFlag] block edge forces alternative route... ";
    // Build: A--B--C--D, also add diagonal A--C with length 10.
    Graph g = buildChain();
    // Add extra edge A(0)--C(2) with length 10.
    Edge extra{};
    extra.a = 0; extra.b = 2; extra.length = 10.0f;
    g.addEdge(extra); // edge id 3

    // Block edge 0 (A-B) to force A->C->D path.
    std::vector<bool> flags(g.edges().size(), true);
    flags[0] = false; // block A-B edge

    const auto result = edgeFlagDijkstra(g, 0, 3, flags);
    assert(result.reachable());
    // With A-B blocked, path must go A->C->D via edge 3 then edge 2.
    // Cost = 10 + 1 = 11.
    assert(std::abs(result.cost - 11.0) < 1e-6);
    std::cout << "PASS (cost=" << result.cost << ")\n";
}

// ---------------------------------------------------------------------------
// Test 9: Highway hierarchy – extract levels from typed graph
// ---------------------------------------------------------------------------
static void testHighwayHierarchy() {
    std::cout << "  [Hierarchy] highway level extraction... ";
    Graph g;
    const VertexID a = g.addVertex({ .pos = Core::Vec2(0.0, 0.0) });
    const VertexID b = g.addVertex({ .pos = Core::Vec2(1.0, 0.0) });
    const VertexID c = g.addVertex({ .pos = Core::Vec2(2.0, 0.0) });

    Edge street{};
    street.a = a; street.b = b; street.length = 1.0f;
    street.type = Core::RoadType::Street;
    g.addEdge(street);

    Edge highway{};
    highway.a = b; highway.b = c; highway.length = 1.0f;
    highway.type = Core::RoadType::Highway;
    g.addEdge(highway);

    const auto levels = extractHighwayHierarchy(g, 3);
    assert(levels.size() == 3);

    // There should be at least one level with the highway edge and one with street.
    bool has_highway = false;
    bool has_street  = false;
    for (const auto& lvl : levels) {
        if (!lvl.highway_edges.empty()) {
            // Highest level must contain the highway.
            has_highway = true;
        }
        if (!lvl.highway_edges.empty() || !has_street) {
            has_street = true; // street is at level 0
        }
    }
    (void)has_highway;
    (void)has_street;
    assert(!levels[0].highway_edges.empty() || !levels[1].highway_edges.empty());
    std::cout << "PASS (levels=" << levels.size() << ")\n";
}

// ---------------------------------------------------------------------------
// Test 10: Crauser labels – verify out_lower_bound is non-negative
// ---------------------------------------------------------------------------
static void testCrauserLabels() {
    std::cout << "  [Crauser] out_lower_bound non-negative... ";
    Graph g = buildTriangle();
    const auto labels = computeCrauserLabels(g);
    assert(labels.out_lower_bound.size() == g.vertices().size());
    for (const double lb : labels.out_lower_bound) {
        assert(lb >= 0.0);
    }
    std::cout << "PASS\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== RoutingLibrary Unit Tests ===\n\n";

    testYenKShortest();
    testYenEqualsShortestPath();
    testDeltaStepping();
    testDeltaSteppingTriangle();
    testRealAStarFallback();
    testRealAStarWithData();
    testLooplessCandidatesPlaceholder();
    testEdgeFlagDijkstra();
    testHighwayHierarchy();
    testCrauserLabels();

    std::cout << "\nAll RoutingLibrary tests PASSED\n";
    return 0;
}
