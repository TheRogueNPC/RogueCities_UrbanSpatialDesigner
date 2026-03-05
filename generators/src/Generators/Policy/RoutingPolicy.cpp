#include "RogueCity/Generators/Policy/RoutingPolicy.hpp"

// RoutingPolicy selects among three cost models and wraps the call with an
// optional wall-clock timing hook.  All determinism is preserved because:
//   - LengthOnly and LengthPlusTurnPenalty delegate to the existing, tested
//     GraphAlgorithms methods whose output is already deterministic.
//   - HierarchyBiased uses a local Dijkstra with a fixed multiplier table
//     keyed on Core::RoadType (no floating-point sources of non-determinism
//     beyond the existing edge.length values in the graph).
// No Graph state is mutated; DeterminismHash is unaffected.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <queue>

namespace RogueCity::Generators::Policy {

    // -------------------------------------------------------------------------
    // Hierarchy multiplier table
    // -------------------------------------------------------------------------
    // Higher multiplier → lower effective weight → router prefers this road class.
    // Values are chosen so Highway is ~4× preferred over an alleyway.

    /*static*/ double RoutingPolicy::hierarchyMultiplier(Core::RoadType type) noexcept {
        switch (type) {
            case Core::RoadType::Highway:   return 4.0;
            case Core::RoadType::Arterial:  return 3.0;
            case Core::RoadType::Avenue:    return 2.5;
            case Core::RoadType::Boulevard: return 2.0;
            case Core::RoadType::Street:    return 1.0;
            case Core::RoadType::Lane:      return 0.9;
            case Core::RoadType::Alleyway:  return 0.7;
            case Core::RoadType::CulDeSac:  return 0.7;
            case Core::RoadType::Drive:     return 0.85;
            case Core::RoadType::Driveway:  return 0.75;
            case Core::RoadType::M_Major:   return 2.5;
            case Core::RoadType::M_Minor:   return 1.0;
            default:                        return 1.0;
        }
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------
    namespace {

        // Minimal standalone Dijkstra used only for HierarchyBiased mode, so that
        // RoutingPolicy.cpp has no dependency on the anonymous-namespace dijkstra()
        // in GraphAlgorithms.cpp (which is intentionally not exported).
        [[nodiscard]] Urban::PathResult hierarchyDijkstra(
            const Urban::Graph& g,
            Urban::VertexID src,
            Urban::VertexID dst,
            double bonus_factor)
        {
            Urban::PathResult out;
            if (!g.isVertexValid(src) || !g.isVertexValid(dst)) {
                return out;
            }

            const double inf = std::numeric_limits<double>::infinity();
            const Urban::VertexID invalid_id = std::numeric_limits<Urban::VertexID>::max();

            std::vector<double> dist(g.vertices().size(), inf);
            std::vector<Urban::VertexID> prev(g.vertices().size(), invalid_id);

            using QNode = std::pair<double, Urban::VertexID>;
            auto cmp = [](const QNode& a, const QNode& b) { return a.first > b.first; };
            std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> q(cmp);

            dist[src] = 0.0;
            q.push({0.0, src});

            while (!q.empty()) {
                const auto [cost, v] = q.top();
                q.pop();

                if (cost > dist[v]) { continue; }
                if (v == dst) { break; }

                const auto* vertex = g.getVertex(v);
                if (!vertex) { continue; }

                for (const Urban::EdgeID eid : vertex->edges) {
                    const auto* e = g.getEdge(eid);
                    if (!e) { continue; }

                    const Urban::VertexID n = (e->a == v) ? e->b : e->a;

                    // Weight = length / multiplier  (higher-class roads get lower weight).
                    // bonus_factor is a user scaling knob on top of the class discount.
                    const double mult = RoutingPolicy::hierarchyMultiplier(e->type) * bonus_factor;
                    const double raw_len = std::max(0.001, static_cast<double>(e->length));
                    const double w = raw_len / std::max(0.1, mult);  // avoid div-by-zero

                    const double next = dist[v] + w;
                    if (next < dist[n]) {
                        dist[n] = next;
                        prev[n] = v;
                        q.push({next, n});
                    }
                }
            }

            if (!std::isfinite(dist[dst])) {
                return out;
            }

            out.cost = dist[dst];
            std::vector<Urban::VertexID> path;
            for (Urban::VertexID cur = dst; cur != invalid_id; ) {
                path.push_back(cur);
                if (cur == src) { break; }
                cur = prev[cur];
            }
            std::reverse(path.begin(), path.end());
            if (path.empty() || path.front() != src) {
                return Urban::PathResult{};
            }
            out.vertices = std::move(path);
            return out;
        }

        // Returns a short label string for use in the timing record.
        [[nodiscard]] const char* modeLabel(WeightMode mode) noexcept {
            switch (mode) {
                case WeightMode::LengthOnly:            return "RoutingPolicy::LengthOnly";
                case WeightMode::LengthPlusTurnPenalty: return "RoutingPolicy::TurnPenalty";
                case WeightMode::HierarchyBiased:       return "RoutingPolicy::HierarchyBiased";
                default:                                return "RoutingPolicy::Unknown";
            }
        }

    } // namespace

    // -------------------------------------------------------------------------
    // RoutingPolicy::compute
    // -------------------------------------------------------------------------

    /*static*/ Urban::PathResult RoutingPolicy::compute(
        const Urban::Graph&          g,
        Urban::VertexID              src,
        Urban::VertexID              dst,
        const RoutingPolicyConfig&   cfg,
        const RoutingTimingCallback& timing_cb)
    {
        const auto t0 = std::chrono::steady_clock::now();

        Urban::PathResult result;
        switch (cfg.mode) {
            case WeightMode::LengthOnly:
                result = Urban::GraphAlgorithms::shortestPath(g, src, dst);
                break;

            case WeightMode::LengthPlusTurnPenalty:
                result = Urban::GraphAlgorithms::simplestPath(
                    g, src, dst,
                    std::max(0.0, cfg.turn_penalty_meters));
                break;

            case WeightMode::HierarchyBiased:
                // bonus_factor=1.0 uses the raw multiplier table; callers may expose
                // this as a parameter in a future RoutingPolicyConfig extension.
                result = hierarchyDijkstra(g, src, dst, 1.0);
                break;
        }

        if (timing_cb) {
            const auto t1 = std::chrono::steady_clock::now();
            const long long ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            timing_cb(modeLabel(cfg.mode), ns);
        }

        return result;
    }

} // namespace RogueCity::Generators::Policy
