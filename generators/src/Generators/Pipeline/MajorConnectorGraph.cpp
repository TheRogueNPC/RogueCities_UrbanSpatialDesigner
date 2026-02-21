#include "RogueCity/Generators/Pipeline/MajorConnectorGraph.hpp"

#include <boost/polygon/point_data.hpp>
#include <boost/polygon/voronoi.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <unordered_set>
#include <vector>

namespace RogueCity::Generators {
namespace {

using BoostPoint = boost::polygon::point_data<int>;
using VoronoiDiagram = boost::polygon::voronoi_diagram<double>;

struct EdgeCandidate {
    int a{ 0 };
    int b{ 0 };
    double length{ 0.0 };
};

[[nodiscard]] uint64_t EdgeKey(int a, int b) {
    const uint32_t lo = static_cast<uint32_t>(std::min(a, b));
    const uint32_t hi = static_cast<uint32_t>(std::max(a, b));
    return (static_cast<uint64_t>(lo) << 32ull) | static_cast<uint64_t>(hi);
}

struct DisjointSet {
    explicit DisjointSet(size_t n) : parent(n), rank(n, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    size_t Find(size_t x) {
        if (parent[x] == x) {
            return x;
        }
        parent[x] = Find(parent[x]);
        return parent[x];
    }

    bool Union(size_t a, size_t b) {
        a = Find(a);
        b = Find(b);
        if (a == b) {
            return false;
        }
        if (rank[a] < rank[b]) {
            std::swap(a, b);
        }
        parent[b] = a;
        if (rank[a] == rank[b]) {
            ++rank[a];
        }
        return true;
    }

    std::vector<size_t> parent;
    std::vector<uint8_t> rank;
};

[[nodiscard]] double Distance(const CityGenerator::AxiomInput& a, const CityGenerator::AxiomInput& b) {
    const double dx = a.position.x - b.position.x;
    const double dy = a.position.y - b.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

[[nodiscard]] std::vector<EdgeCandidate> BuildCandidatesFromVoronoi(
    const std::vector<CityGenerator::AxiomInput>& axioms) {
    std::vector<EdgeCandidate> candidates{};
    if (axioms.size() < 2) {
        return candidates;
    }

    std::vector<BoostPoint> points;
    points.reserve(axioms.size());
    constexpr double kScale = 100.0;
    for (const auto& axiom : axioms) {
        points.emplace_back(
            static_cast<int>(std::llround(axiom.position.x * kScale)),
            static_cast<int>(std::llround(axiom.position.y * kScale)));
    }

    VoronoiDiagram vd;
    boost::polygon::construct_voronoi(points.begin(), points.end(), &vd);

    std::unordered_set<uint64_t> seen;
    seen.reserve(vd.edges().size());
    for (const auto& edge : vd.edges()) {
        if (!edge.is_primary()) {
            continue;
        }
        const auto* c0 = edge.cell();
        const auto* c1 = edge.twin() != nullptr ? edge.twin()->cell() : nullptr;
        if (c0 == nullptr || c1 == nullptr) {
            continue;
        }
        const int a = static_cast<int>(c0->source_index());
        const int b = static_cast<int>(c1->source_index());
        if (a == b || a < 0 || b < 0 ||
            a >= static_cast<int>(axioms.size()) ||
            b >= static_cast<int>(axioms.size())) {
            continue;
        }

        const uint64_t key = EdgeKey(a, b);
        if (!seen.insert(key).second) {
            continue;
        }
        candidates.push_back({std::min(a, b), std::max(a, b), Distance(axioms[a], axioms[b])});
    }

    if (candidates.empty()) {
        // Deterministic fallback for degenerate layouts.
        for (int i = 0; i < static_cast<int>(axioms.size()); ++i) {
            int nearest = -1;
            double nearest_distance = std::numeric_limits<double>::max();
            for (int j = 0; j < static_cast<int>(axioms.size()); ++j) {
                if (i == j) {
                    continue;
                }
                const double d = Distance(axioms[i], axioms[j]);
                if (d < nearest_distance) {
                    nearest_distance = d;
                    nearest = j;
                }
            }
            if (nearest >= 0) {
                const int a = std::min(i, nearest);
                const int b = std::max(i, nearest);
                const uint64_t key = EdgeKey(a, b);
                if (seen.insert(key).second) {
                    candidates.push_back({a, b, nearest_distance});
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const EdgeCandidate& lhs, const EdgeCandidate& rhs) {
        if (lhs.length != rhs.length) {
            return lhs.length < rhs.length;
        }
        if (lhs.a != rhs.a) {
            return lhs.a < rhs.a;
        }
        return lhs.b < rhs.b;
    });
    return candidates;
}

} // namespace

MajorConnectorGraphOutput MajorConnectorGraph::Build(
    const std::vector<CityGenerator::AxiomInput>& axioms,
    const MajorConnectorGraphConfig& config) const {
    MajorConnectorGraphOutput output{};
    if (axioms.size() < 2) {
        return output;
    }

    std::vector<EdgeCandidate> candidates = BuildCandidatesFromVoronoi(axioms);
    if (candidates.empty()) {
        return output;
    }

    // Build nearest-neighbor baseline to derive deterministic length cutoff.
    std::vector<double> nearest(static_cast<size_t>(axioms.size()), std::numeric_limits<double>::max());
    for (const auto& edge : candidates) {
        nearest[static_cast<size_t>(edge.a)] = std::min(nearest[static_cast<size_t>(edge.a)], edge.length);
        nearest[static_cast<size_t>(edge.b)] = std::min(nearest[static_cast<size_t>(edge.b)], edge.length);
    }
    std::vector<double> nearest_sorted = nearest;
    std::sort(nearest_sorted.begin(), nearest_sorted.end());
    const double median_nearest = nearest_sorted[nearest_sorted.size() / 2];
    const double max_length = std::max(1.0, median_nearest * std::max(1.0, config.max_edge_length_factor));

    std::vector<int> degree(static_cast<size_t>(axioms.size()), 0);
    std::unordered_set<uint64_t> selected{};
    selected.reserve(candidates.size());

    // Pass 1: MST for guaranteed connectivity.
    DisjointSet dsu(axioms.size());
    for (const auto& edge : candidates) {
        if (dsu.Union(static_cast<size_t>(edge.a), static_cast<size_t>(edge.b))) {
            output.edges.push_back({edge.a, edge.b});
            selected.insert(EdgeKey(edge.a, edge.b));
            degree[static_cast<size_t>(edge.a)] += 1;
            degree[static_cast<size_t>(edge.b)] += 1;
        }
    }

    // Pass 2: deterministic pruning/add-back with length and degree limits.
    for (const auto& edge : candidates) {
        const uint64_t key = EdgeKey(edge.a, edge.b);
        if (selected.find(key) != selected.end()) {
            continue;
        }
        if (edge.length > max_length) {
            continue;
        }
        if (degree[static_cast<size_t>(edge.a)] >= config.max_degree_per_node + config.extra_edges_per_node) {
            continue;
        }
        if (degree[static_cast<size_t>(edge.b)] >= config.max_degree_per_node + config.extra_edges_per_node) {
            continue;
        }
        output.edges.push_back({edge.a, edge.b});
        selected.insert(key);
        degree[static_cast<size_t>(edge.a)] += 1;
        degree[static_cast<size_t>(edge.b)] += 1;
    }

    std::sort(output.edges.begin(), output.edges.end());
    return output;
}

} // namespace RogueCity::Generators

