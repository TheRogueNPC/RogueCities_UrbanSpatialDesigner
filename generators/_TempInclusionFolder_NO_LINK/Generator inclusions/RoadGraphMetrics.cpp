#include "RoadGraphMetrics.h"

#include <queue>

namespace RoadGraphMetrics
{
    std::vector<int> compute_degrees(const Graph &graph)
    {
        std::vector<int> degrees;
        degrees.reserve(graph.nodes.size());
        for (const auto &node : graph.nodes)
        {
            degrees.push_back(static_cast<int>(node.adj.size()));
        }
        return degrees;
    }

    CommunityResult compute_components(const Graph &graph)
    {
        CommunityResult result;
        const int n = static_cast<int>(graph.nodes.size());
        result.node_community.assign(n, -1);
        result.node_degree = compute_degrees(graph);

        int current = 0;
        std::queue<int> q;
        for (int i = 0; i < n; ++i)
        {
            if (result.node_community[i] != -1)
            {
                continue;
            }
            result.node_community[i] = current;
            q.push(i);
            while (!q.empty())
            {
                int v = q.front();
                q.pop();
                for (int nb : graph.nodes[v].adj)
                {
                    if (nb < 0 || nb >= n)
                    {
                        continue;
                    }
                    if (result.node_community[nb] == -1)
                    {
                        result.node_community[nb] = current;
                        q.push(nb);
                    }
                }
            }
            current++;
        }

        result.community_count = current;
        return result;
    }
} // namespace RoadGraphMetrics

