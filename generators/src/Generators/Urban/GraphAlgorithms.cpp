#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <random>

namespace RogueCity::Generators::Urban {

    namespace {

        [[nodiscard]] VertexID neighborOf(const Edge& e, VertexID v) {
            return (e.a == v) ? e.b : e.a;
        }

        [[nodiscard]] EdgeID findEdgeBetween(const Graph& g, VertexID a, VertexID b) {
            const auto* va = g.getVertex(a);
            if (va == nullptr) {
                return std::numeric_limits<EdgeID>::max();
            }
            for (const EdgeID eid : va->edges) {
                const auto* e = g.getEdge(eid);
                if (e == nullptr) {
                    continue;
                }
                if ((e->a == a && e->b == b) || (e->a == b && e->b == a)) {
                    return eid;
                }
            }
            return std::numeric_limits<EdgeID>::max();
        }

        [[nodiscard]] PathResult dijkstra(
            const Graph& g,
            VertexID src,
            VertexID dst,
            const std::function<double(VertexID, EdgeID, EdgeID, const Edge&)>& weight_fn) {
            PathResult out;
            if (!g.isVertexValid(src) || !g.isVertexValid(dst)) {
                return out;
            }

            const double inf = std::numeric_limits<double>::infinity();
            std::vector<double> dist(g.vertices().size(), inf);
            std::vector<VertexID> prev(g.vertices().size(), std::numeric_limits<VertexID>::max());
            std::vector<EdgeID> prev_edge(g.vertices().size(), std::numeric_limits<EdgeID>::max());

            using QNode = std::pair<double, VertexID>;
            auto cmp = [](const QNode& a, const QNode& b) { return a.first > b.first; };
            std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> q(cmp);

            dist[src] = 0.0;
            q.push({ 0.0, src });

            while (!q.empty()) {
                const auto [cost, v] = q.top();
                q.pop();
                if (cost > dist[v]) {
                    continue;
                }
                if (v == dst) {
                    break;
                }

                const auto* vertex = g.getVertex(v);
                if (vertex == nullptr) {
                    continue;
                }
                for (const EdgeID eid : vertex->edges) {
                    const auto* e = g.getEdge(eid);
                    if (e == nullptr) {
                        continue;
                    }
                    const VertexID n = neighborOf(*e, v);
                    const double w = weight_fn(v, prev_edge[v], eid, *e);
                    const double next = dist[v] + std::max(1e-3, w);
                    if (next < dist[n]) {
                        dist[n] = next;
                        prev[n] = v;
                        prev_edge[n] = eid;
                        q.push({ next, n });
                    }
                }
            }

            if (!std::isfinite(dist[dst])) {
                return out;
            }

            out.cost = dist[dst];
            std::vector<VertexID> path;
            for (VertexID cur = dst; cur != std::numeric_limits<VertexID>::max(); cur = prev[cur]) {
                path.push_back(cur);
                if (cur == src) {
                    break;
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

    PathResult GraphAlgorithms::shortestPath(
        const Graph& g,
        VertexID src,
        VertexID dst) {
        return dijkstra(
            g,
            src,
            dst,
            [](VertexID, EdgeID, EdgeID, const Edge& e) {
                return static_cast<double>(std::max(0.001f, e.length));
            });
    }

    PathResult GraphAlgorithms::simplestPath(
        const Graph& g,
        VertexID src,
        VertexID dst,
        double turn_penalty) {
        return dijkstra(
            g,
            src,
            dst,
            [&g, turn_penalty](VertexID at, EdgeID in_eid, EdgeID, const Edge& edge) {
                double weight = static_cast<double>(std::max(0.001f, edge.length));
                if (in_eid == std::numeric_limits<EdgeID>::max()) {
                    return weight;
                }

                const auto* in_e = g.getEdge(in_eid);
                const auto* v = g.getVertex(at);
                if (in_e == nullptr || v == nullptr) {
                    return weight;
                }

                const VertexID prev = (in_e->a == at) ? in_e->b : in_e->a;
                const VertexID next = (edge.a == at) ? edge.b : edge.a;
                const auto* p_prev = g.getVertex(prev);
                const auto* p_next = g.getVertex(next);
                if (p_prev == nullptr || p_next == nullptr) {
                    return weight;
                }

                Core::Vec2 vin = (v->pos - p_prev->pos);
                Core::Vec2 vout = (p_next->pos - v->pos);
                const double lin = vin.length();
                const double lout = vout.length();
                if (lin <= 1e-6 || lout <= 1e-6) {
                    return weight;
                }
                vin /= lin;
                vout /= lout;

                const double turn = 1.0 - std::abs(vin.dot(vout)); // 0 straight, 1 right-angle-ish.
                return weight + turn * turn_penalty;
            });
    }

    std::vector<float> GraphAlgorithms::sampledEdgeCentrality(
        const Graph& g,
        size_t sample_count,
        uint32_t seed) {
        std::vector<float> centrality(g.edges().size(), 0.0f);
        if (g.vertices().size() < 2 || g.edges().empty()) {
            return centrality;
        }

        const size_t default_samples = std::max<size_t>(8, g.vertices().size() / 2);
        const size_t samples = std::max<size_t>(sample_count, default_samples);
        std::mt19937 rng(seed);
        std::uniform_int_distribution<uint32_t> dist(0u, static_cast<uint32_t>(g.vertices().size() - 1));

        for (size_t s = 0; s < samples; ++s) {
            VertexID src = dist(rng);
            VertexID dst = dist(rng);
            if (src == dst) {
                continue;
            }
            const auto path = shortestPath(g, src, dst);
            if (!path.reachable() || path.vertices.size() < 2) {
                continue;
            }

            for (size_t i = 1; i < path.vertices.size(); ++i) {
                const EdgeID eid = findEdgeBetween(g, path.vertices[i - 1], path.vertices[i]);
                if (eid != std::numeric_limits<EdgeID>::max() && eid < centrality.size()) {
                    centrality[eid] += 1.0f;
                }
            }
        }

        const float max_v = *std::max_element(centrality.begin(), centrality.end());
        if (max_v > 1e-5f) {
            for (auto& v : centrality) {
                v /= max_v;
            }
        }
        return centrality;
    }

} // namespace RogueCity::Generators::Urban
