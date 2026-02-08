#include "PolygonFinder.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "PolygonUtil.h"

PolygonFinder::PolygonFinder(const std::vector<Node> &nodesIn,
                             const PolygonParams &paramsIn,
                             const TensorField::TensorField &fieldIn)
    : graphNodes(nodesIn),
      params(paramsIn),
      field(fieldIn) {}

void PolygonFinder::reset()
{
    polygons.clear();
}

namespace
{
    // Compute signed angle from vector (1,0) to vector (dx,dy) in [-PI, PI]
    double angle_of(double dx, double dy)
    {
        return std::atan2(dy, dx);
    }

    // For a node, sort adjacent nodes by angle from reference direction
    std::vector<int> sort_neighbors_by_angle(const Node &node, const std::vector<Node> &allNodes)
    {
        std::vector<std::pair<double, int>> angles;
        angles.reserve(node.adj.size());
        for (int neighbor : node.adj)
        {
            double dx = allNodes[neighbor].value.x - node.value.x;
            double dy = allNodes[neighbor].value.y - node.value.y;
            angles.push_back({angle_of(dx, dy), neighbor});
        }
        std::sort(angles.begin(), angles.end());
        std::vector<int> result;
        result.reserve(angles.size());
        for (const auto &p : angles)
        {
            result.push_back(p.second);
        }
        return result;
    }

    // Find the next edge in CCW order around a node
    int next_edge_ccw(int from, int current, const std::vector<Node> &nodes,
                      const std::vector<std::vector<int>> &sorted_adj)
    {
        const auto &neighbors = sorted_adj[current];
        if (neighbors.empty())
            return -1;
        if (neighbors.size() == 1)
            return neighbors[0]; // only one way to go

        // Find 'from' in the sorted list
        int from_idx = -1;
        for (size_t i = 0; i < neighbors.size(); ++i)
        {
            if (neighbors[i] == from)
            {
                from_idx = static_cast<int>(i);
                break;
            }
        }
        if (from_idx == -1)
            return neighbors[0]; // shouldn't happen

        // Next CCW is the previous index (wrapping)
        int next_idx = (from_idx - 1 + static_cast<int>(neighbors.size())) % static_cast<int>(neighbors.size());
        return neighbors[next_idx];
    }
} // namespace

void PolygonFinder::findPolygons()
{
    polygons.clear();

    const size_t n = graphNodes.size();
    if (n < 3)
        return;

    // Precompute sorted adjacency lists for each node (sorted by angle)
    std::vector<std::vector<int>> sorted_adj(n);
    for (size_t i = 0; i < n; ++i)
    {
        sorted_adj[i] = sort_neighbors_by_angle(graphNodes[i], graphNodes);
    }

    // Track which directed edges have been used
    std::unordered_set<uint64_t> used_edges;
    auto edge_key = [](int from, int to) -> uint64_t
    {
        return (static_cast<uint64_t>(from) << 32) | static_cast<uint64_t>(to);
    };

    // For each directed edge, trace a minimal face (polygon)
    for (size_t start_node = 0; start_node < n; ++start_node)
    {
        for (int first_neighbor : sorted_adj[start_node])
        {
            uint64_t start_edge = edge_key(static_cast<int>(start_node), first_neighbor);
            if (used_edges.count(start_edge))
                continue;

            // Trace a face starting with edge (start_node -> first_neighbor)
            std::vector<int> face;
            face.push_back(static_cast<int>(start_node));

            int prev = static_cast<int>(start_node);
            int curr = first_neighbor;
            int steps = 0;
            const int max_steps = static_cast<int>(n) * 2;

            while (curr != static_cast<int>(start_node) && steps < max_steps)
            {
                face.push_back(curr);
                used_edges.insert(edge_key(prev, curr));

                int next = next_edge_ccw(prev, curr, graphNodes, sorted_adj);
                if (next == -1)
                    break;

                prev = curr;
                curr = next;
                ++steps;
            }

            if (curr == static_cast<int>(start_node) && face.size() >= 3)
            {
                // Mark the final closing edge as used
                used_edges.insert(edge_key(prev, curr));

                // Build polygon points
                std::vector<CityModel::Vec2> poly;
                poly.reserve(face.size() + 1);
                for (int idx : face)
                {
                    poly.push_back(graphNodes[idx].value);
                }

                // Check if it's a valid polygon (positive area = CCW, reasonable size)
                double area = PolygonUtil::polygonArea(poly);
                if (area >= params.minArea && area < 1e8) // Filter tiny and huge polygons
                {
                    poly.push_back(poly.front()); // Close the ring
                    polygons.push_back(std::move(poly));
                }
            }
        }
    }
}
