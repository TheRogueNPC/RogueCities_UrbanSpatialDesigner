// test_routing_policy.cpp
// Proves:
//   1. LengthOnly selects the geometrically shortest path on a known 5-node graph.
//   2. LengthPlusTurnPenalty with a large penalty flips the winner vs LengthOnly.
//   3. HierarchyBiased prefers a detour on a Highway over a shortcut on an Alleyway.
//   4. The optional timing callback fires with elapsed_ns >= 0.

#include "RogueCity/Generators/Policy/RoutingPolicy.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <cassert>
#include <cstdio>

using namespace RogueCity::Generators::Urban;
using namespace RogueCity::Generators::Policy;
using RogueCity::Core::RoadType;
using RogueCity::Core::Vec2;

// ---------------------------------------------------------------------------
// Helper: build a small graph and return it.
//
// Topology (all edges undirected):
//
//   0 ---10m--- 1 ---10m--- 2
//   |                       |
//   20m                     5m  (Alleyway, so short but penalised by hierarchy)
//   |                       |
//   3 --------30m---------- 4
//
//   Extra highway path: 0 ->highway-> 3 ->highway-> 4 ->highway-> 2 (total len = 60m)
//   Direct short path:  0 -> 1 -> 2 -> 4 (10+10+5 = 25m)  — mix of Street + Alleyway
//
// The graph is also used to test turn-penalty: the route 0->1->2 goes straight;
// a detour via 0->3->4->2 also reaches 2 but requires turns.
// ---------------------------------------------------------------------------

struct TestGraph {
    Graph g;
    VertexID v[5]{};

    TestGraph() {
        // 5 vertices placed on a rough grid
        Vertex vt{};
        vt.pos = Vec2(0.0, 0.0);  v[0] = g.addVertex(vt);
        vt.pos = Vec2(10.0, 0.0); v[1] = g.addVertex(vt);
        vt.pos = Vec2(20.0, 0.0); v[2] = g.addVertex(vt);
        vt.pos = Vec2(0.0, -20.0);v[3] = g.addVertex(vt);
        vt.pos = Vec2(20.0,-20.0);v[4] = g.addVertex(vt);

        // Top edges (Street)
        addEdge(v[0], v[1], RoadType::Street,  10.0f);
        addEdge(v[1], v[2], RoadType::Street,  10.0f);

        // Right side Alleyway — short but low-class
        addEdge(v[2], v[4], RoadType::Alleyway, 5.0f);

        // Left side and bottom — Highway, longer but high-class
        addEdge(v[0], v[3], RoadType::Highway,  20.0f);
        addEdge(v[3], v[4], RoadType::Highway,  30.0f);
        addEdge(v[4], v[2], RoadType::Highway,  10.0f);  // duplicate right-side as highway
    }

private:
    void addEdge(VertexID a, VertexID b, RoadType t, float len) {
        Edge e{};
        e.a = a; e.b = b;
        e.type = t;
        e.length = len;
        // shape: just endpoints
        e.shape = { g.vertices()[a].pos, g.vertices()[b].pos };
        EdgeID eid = g.addEdge(e);
        g.getVertexMutable(a)->edges.push_back(eid);
        g.getVertexMutable(b)->edges.push_back(eid);
    }
};

// ---------------------------------------------------------------------------
// TEST 1: LengthOnly chooses the geometrically shortest path 0 -> 2
// Expected: 0->1->2  (cost=20m, vs highway path 0->3->4->2 cost=60m)
// ---------------------------------------------------------------------------
static void test_length_only() {
    TestGraph tg;
    RoutingPolicyConfig cfg{};
    cfg.mode = WeightMode::LengthOnly;

    const auto result = RoutingPolicy::compute(tg.g, tg.v[0], tg.v[2], cfg);
    assert(result.reachable());
    // The top path 0->1->2 = 20m must be cheaper than highway detour 0->3->4->2 = 60m
    assert(result.cost < 30.0);
    // The path must go through vertex 1
    bool via_1 = false;
    for (const auto vid : result.vertices) {
        if (vid == tg.v[1]) { via_1 = true; break; }
    }
    assert(via_1 && "LengthOnly should route 0->1->2 not via highway");
    std::printf("[PASS] test_length_only\n");
}

// ---------------------------------------------------------------------------
// TEST 2: TurnPenalty forces a different route
//
// We build a dedicated 4-node graph where the straight route is length 4 but
// the penalty route is length 6 but avoids 1 turn vs 0 turns:
//
//  A -2m- B -2m- C    (straight, no turns, total=4m, 0 turns)
//  A -3m- D -3m- C    (bent route, 2 turn-crossings, total=6m under pure length)
//
// With no penalty: A->B->C (cost=4) wins.
// With turn_penalty=100m per turn: A->B->C already straight so 0 penalty;
// the bent path A->D->C has 2 turns(*100) → costs 206. Still A->B->C wins.
//
// Better: a graph where the direct route DOES change bearing at every node:
//
//  A(0,0) -> B(1,0) -> C(1,1)  — the A-B segment goes east, B-C goes north (1 turn)
//                                  length = 2m total, 1 turn
//  A(0,0) -> D(0,1) -> C(1,1)  — A-D goes north, D-C goes east (1 turn)
//                                  length = 2m total, 1 turn
//
// Both routes have same length and same turn count — not useful.
//
// Use: A(0,0) -> B(2,0) -> C(2,1)  short but 90° turn at B
//      A(0,0) -> D(0,1) -> E(1,1) -> C(2,1)  longer in length, but
// Actually the whole point of simplestPath is to penalise TURN MAGNITUDE
// (how sharp the turn is).  Let me build a graph where the short path
// has a near-180° reversal (full penalty of ≈turn_penalty) and the longer
// path goes straight:
//
//  A(0,0) -> B(1,0) -> C(0,0) -- can't reuse A as C
//
// Simplest provable case:
//   Short but zig-zag: A(0,0)->B(1,0)->C(0.5, 0.1)  length≈1.5, angle close to 180°
//   Long but straight: A(0,0)->D(0.5,0)->C(0.5,0.1) length≈0.6, but routes through different nodes
//
// Actually, simplestPath adds `turn * turn_penalty` where turn=1-|cos(angle)|.
// To provably flip the route:
//
// Build:
//   S(0,0), M(1,0), T(2,0) — collinear, length=2, turn at M ≈ 0 (straight)
//   S(0,0), X(1,2), T(2,0) — not collinear, length = sqrt(5)*2 ≈ 4.47, sharp turns at X
//
// LengthOnly: S->X->T wins (both are ~4.47m) — no, S-M-T = 2m is shorter.
// With turn_penalty=1000: S->M->T (2 + 0*1000 = 2) vs S->X->T (4.47 + 1*1000 ≈ 1004) 
// → S-M-T wins in both cases.  Not what we want.
//
// Here we want turn_penalty to INCREASE cost such that the originally-winning
// (shorter) path becomes MORE EXPENSIVE than the longer path.
//
// Setup:
//   S(0,0) -> M(1,0) -> T(2,0): length=2, perfectly straight turn=0 at M → cost=2
//   S(0,0) -> X(0.5,-10) -> T(2,0): goes sharply south, length≈20.1, almost full reversal
//
// That doesn't help either because the zig is longer.
//
// The correct setup for "turn penalty changes winner":
//   Route 1 (short): S -> A -> T  length = 4  with a near-180° turn at A (cost = 4 + turn_penalty * ~1.0)
//   Route 2 (long):  S -> B -> T  length = 6  with no turn at B (collinear, turn=0)
//
// Layout: S=(0,0) A=(1,0) T=(0,0) — T must differ from S…
//
// Trick: make T = (2, 0):
//   Route 1 straight:  S(0,0)->A(1,0)->T(2,0), length=2, turn≈0 (straight)
//   Route 2 detour:    S(0,0)->B(1,5)->T(2,0), length = sqrt(26)*2 ≈ 10.2, sharp turn at B
//
// Same as before. The longer route is never the winner under turn_penalty because length dominates.
//
// CORRECT approach: put S, A, T collinear for route 2 (no penalty) but make that route
// slightly longer; route 1 is shorter but has a huge reversal penalty.
//
// S=(0,0),  T=(10,0)
// Route 1 (short but reversal): S->A(5,0)->T(10,0) length=10, angle at A = 0° (straight) → cost 10
// Route 1 (short with reversal): S->A(5,0)->T(4,0) -- T can't be behind A
//
// I'm overcomplicating this. Let me just use the simplestPath documented behavior:
// it adds `(1 - |cos(angle)|) * turn_penalty` at each intermediate vertex.
// A 90° turn contributes 1.0 * turn_penalty.
// A straight (0° deviation) contributes 0.
//
// 4-node layout for the test:
//   S=(0,0), A=(10,0), T=(10,10): S->A->T length=20, 90° turn at A → penalty=turn_penalty
//                                  total cost (with penalty) = 20 + turn_penalty
//   S=(0,0), B=(5, 5), T=(10,10): B is on the diagonal.
//                                  length = sqrt(50) + sqrt(50) ≈ 14.14, turn at B ≈ 0°
//                                  total cost = 14.14 + 0
//
// LengthOnly: S->B->T (14.14) wins.
// Turn penalty: S->A->T (20 + turn_penalty) vs S->B->T (14.14)
//   ⇒ route through A is always more expensive. Still not a flip.
//
// The flip only works if the straight-ish path is LONGER in length than the 
// penalised path (so length wins first) but the penalty reverses it.
//
// Final working setup:
//   S->A->T: A is approximately on the S-T line (almost straight), length=15m, turn≈0
//   S->B->T: B causes a 90° turn, length=12m (shorter)
//
// LengthOnly winner: S->B->T (12m)
// TurnPenalty winner: when turn_penalty > (12 - 15) is impossible since 12 < 15…
//
// WAIT: if turn_penalty adds cost to a path, it can only make paths MORE expensive.
// So under turn_penalty the straight path (S->A->T) stays at 15m,
// and the zig-zag path (S->B->T) gains +turn_penalty ≥ some value.
// If turn_penalty is large enough: S->A->T (15) < S->B->T (12 + turn_penalty).
// Under LengthOnly: S->B->T (12) < S->A->T (15).
// ⇒ The winner FLIPS when turn_penalty > 3.0 m.
//
// Layout:
//   S=(0,0), T=(15,0)
//   A=(7.5, 0.1) — almost on the S-T line (barely above), length ≈ 15m, turn ≈ 0
//   B=(2,4) — creates a right-angle-ish detour

static void test_turn_penalty_changes_route() {
    Graph g;
    Vertex vt{};

    vt.pos = Vec2(0.0, 0.0);   VertexID S  = g.addVertex(vt);
    vt.pos = Vec2(7.5, 0.1);   VertexID A  = g.addVertex(vt);  // near-straight path node
    vt.pos = Vec2(2.0, 4.0);   VertexID B  = g.addVertex(vt);  // zig-zag path node
    vt.pos = Vec2(15.0, 0.0);  VertexID T  = g.addVertex(vt);

    // We manually set lengths so the math is exact:
    //   S-A = 7.501 m,  A-T = 7.501 m  → total ≈ 15.0 m, near-zero turn at A
    //   S-B = 4.472 m,  B-T = 13.077 m → total ≈ 17.55 m   (sqrt(4+16) + sqrt(169+16))
    // Hmm, that makes straight path shorter. Let me write it differently:
    //   S→B   = 5.0 m   manually,   B→T = 7.0 m  →  total = 12.0 m  (SHORTER)
    //   S→A   = 7.5 m,              A→T = 7.5 m  →  total = 15.0 m  (LONGER, near-straight)
    // LengthOnly: S-B-T=12 wins.
    // TurnPenalty(>3m): straight route S-A-T=15 < zigzag S-B-T = 12+penalty*turn_factor

    auto addEdge2 = [&](VertexID a, VertexID b, float len) {
        Edge e{};
        e.a = a; e.b = b;
        e.type = RoadType::Street;
        e.length = len;
        e.shape = { g.vertices()[a].pos, g.vertices()[b].pos };
        EdgeID eid = g.addEdge(e);
        g.getVertexMutable(a)->edges.push_back(eid);
        g.getVertexMutable(b)->edges.push_back(eid);
    };

    addEdge2(S, A, 7.5f);
    addEdge2(A, T, 7.5f);
    addEdge2(S, B, 5.0f);
    addEdge2(B, T, 7.0f);

    // Under LengthOnly: S-B-T (12) must win
    {
        RoutingPolicyConfig cfg{};
        cfg.mode = WeightMode::LengthOnly;
        const auto r = RoutingPolicy::compute(g, S, T, cfg);
        assert(r.reachable());
        bool via_B = false;
        for (auto vid : r.vertices) { if (vid == B) via_B = true; }
        assert(via_B && "LengthOnly should pick the shorter S->B->T path");
    }

    // Under TurnPenalty (large): S-A-T (15, no turn) must beat S-B-T (12 + big penalty)
    // B sits at (2,4), T at (15,0).  Vector S->B = (2,4), vector B->T = (13,-4).
    // Normalised dot = (2*13 + 4*(-4)) / (sqrt(20)*sqrt(185)) = (26-16)/sqrt(3700) ≈ 10/60.8 ≈ 0.165
    // turn = 1 - |0.165| ≈ 0.835  → penalty contribution = 0.835 * turn_penalty_meters
    // For S-B-T to lose: 12 + 0.835*penalty > 15  ⇒  penalty > 3.593 m
    // Use penalty = 10m → S-B-T cost ≈ 20.35, S-A-T cost ≈ 15.
    {
        RoutingPolicyConfig cfg{};
        cfg.mode = WeightMode::LengthPlusTurnPenalty;
        cfg.turn_penalty_meters = 10.0;
        const auto r = RoutingPolicy::compute(g, S, T, cfg);
        assert(r.reachable());
        bool via_A = false;
        for (auto vid : r.vertices) { if (vid == A) via_A = true; }
        assert(via_A && "TurnPenalty=10 should flip winner to straight S->A->T path");
    }

    std::printf("[PASS] test_turn_penalty_changes_route\n");
}

// ---------------------------------------------------------------------------
// TEST 3: HierarchyBiased prefers the highway detour over the alleyway shortcut
// (uses the same TestGraph as test_length_only)
// ---------------------------------------------------------------------------
static void test_hierarchy_prefers_highway() {
    TestGraph tg;
    RoutingPolicyConfig cfg{};
    cfg.mode = WeightMode::HierarchyBiased;

    // LengthOnly winner: 0->1->2 (20m, street)
    // HierarchyBiased:   0->3->4->2 via highway (60m but mult=4 → effective cost=60/4=15)
    //                    0->1->2 via street (20m, mult=1 → effective cost=20)
    // So highway detour becomes cheaper.
    const auto result = RoutingPolicy::compute(tg.g, tg.v[0], tg.v[2], cfg);
    assert(result.reachable());
    bool via_highway = false;
    for (const auto vid : result.vertices) {
        if (vid == tg.v[3] || vid == tg.v[4]) { via_highway = true; break; }
    }
    assert(via_highway && "HierarchyBiased must prefer the highway detour");
    std::printf("[PASS] test_hierarchy_prefers_highway\n");
}

// ---------------------------------------------------------------------------
// TEST 4: Timing callback fires with elapsed_ns >= 0
// ---------------------------------------------------------------------------
static void test_timing_callback_fires() {
    TestGraph tg;
    RoutingPolicyConfig cfg{};
    cfg.mode = WeightMode::LengthOnly;

    int call_count = 0;
    long long recorded_ns = -1;

    RoutingTimingCallback cb = [&](const char* /*label*/, long long ns) {
        ++call_count;
        recorded_ns = ns;
    };

    const auto result = RoutingPolicy::compute(tg.g, tg.v[0], tg.v[2], cfg, cb);
    assert(result.reachable());
    assert(call_count == 1    && "Timing callback should fire exactly once per compute()");
    assert(recorded_ns >= 0   && "Elapsed ns must be non-negative");
    std::printf("[PASS] test_timing_callback_fires (elapsed_ns=%lld)\n", recorded_ns);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_length_only();
    test_turn_penalty_changes_route();
    test_hierarchy_prefers_highway();
    test_timing_callback_fires();
    std::printf("All routing policy tests passed.\n");
    return 0;
}
