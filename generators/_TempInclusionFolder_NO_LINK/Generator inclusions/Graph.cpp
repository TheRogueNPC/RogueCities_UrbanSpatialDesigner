#include "Graph.h"

#include <algorithm>
#include <cmath>

namespace
{
    double cross(const CityModel::Vec2 &a, const CityModel::Vec2 &b, const CityModel::Vec2 &c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }
}

Graph::Graph(const std::vector<std::vector<CityModel::Vec2>> &streamlines, double dstep, bool deleteDangling)
{
    mergeRadius = std::max(0.001, dstep * 0.25);
    std::vector<Segment> segments;
    for (const auto &s : streamlines)
    {
        for (std::size_t i = 0; i + 1 < s.size(); ++i)
        {
            segments.push_back({s[i], s[i + 1]});
        }
    }

    // Find intersections
    for (std::size_t i = 0; i < segments.size(); ++i)
    {
        for (std::size_t j = i + 1; j < segments.size(); ++j)
        {
            CityModel::Vec2 inter;
            if (segmentIntersection(segments[i].from, segments[i].to, segments[j].from, segments[j].to, inter))
            {
                intersections.push_back(inter);
            }
        }
    }

    // Build per-streamline nodes ordered along polyline (including intersections)
    for (const auto &s : streamlines)
    {
        std::vector<CityModel::Vec2> points = s;
        // Insert intersection points that lie on segment
        for (const auto &inter : intersections)
        {
            for (std::size_t i = 0; i + 1 < s.size(); ++i)
            {
                const double c1 = cross(s[i], s[i + 1], inter);
                if (std::abs(c1) < 1e-6)
                {
                    // check projection within segment
                    double dot1 = (inter.x - s[i].x) * (s[i + 1].x - s[i].x) + (inter.y - s[i].y) * (s[i + 1].y - s[i].y);
                    double dot2 = (inter.x - s[i + 1].x) * (s[i].x - s[i + 1].x) + (inter.y - s[i + 1].y) * (s[i].y - s[i + 1].y);
                    if (dot1 >= 0 && dot2 >= 0)
                    {
                        points.push_back(inter);
                    }
                }
            }
        }
        // Sort along length
        std::sort(points.begin(), points.end(), [&](const CityModel::Vec2 &a, const CityModel::Vec2 &b)
                  {
            double da = (a.x - s.front().x)*(a.x - s.front().x) + (a.y - s.front().y)*(a.y - s.front().y);
            double db = (b.x - s.front().x)*(b.x - s.front().x) + (b.y - s.front().y)*(b.y - s.front().y);
            return da < db; });

        int prev = -1;
        for (const auto &p : points)
        {
            int idx = addOrGetNode(p);
            if (prev != -1 && std::find(nodes[prev].adj.begin(), nodes[prev].adj.end(), idx) == nodes[prev].adj.end())
            {
                nodes[prev].adj.push_back(idx);
                nodes[idx].adj.push_back(prev);
            }
            prev = idx;
        }
    }

    if (deleteDangling)
        removeDangling();
}

int Graph::addOrGetNode(const CityModel::Vec2 &p)
{
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        if (p.distanceToSquared(nodes[i].value) <= mergeRadius * mergeRadius)
        {
            return static_cast<int>(i);
        }
    }
    nodes.push_back(Node{p, {}});
    return static_cast<int>(nodes.size() - 1);
}

bool Graph::segmentIntersection(const CityModel::Vec2 &a, const CityModel::Vec2 &b,
                                const CityModel::Vec2 &c, const CityModel::Vec2 &d,
                                CityModel::Vec2 &out) const
{
    const double s1_x = b.x - a.x;
    const double s1_y = b.y - a.y;
    const double s2_x = d.x - c.x;
    const double s2_y = d.y - c.y;

    const double denom = (-s2_x * s1_y + s1_x * s2_y);
    if (std::abs(denom) < 1e-9)
        return false;
    const double s = (-s1_y * (a.x - c.x) + s1_x * (a.y - c.y)) / denom;
    const double t = (s2_x * (a.y - c.y) - s2_y * (a.x - c.x)) / denom;
    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        out.x = a.x + (t * s1_x);
        out.y = a.y + (t * s1_y);
        return true;
    }
    return false;
}

void Graph::removeDangling()
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (std::size_t i = 0; i < nodes.size();)
        {
            if (nodes[i].adj.size() <= 1)
            {
                // remove references to node i
                for (auto idx : nodes[i].adj)
                {
                    auto &adjList = nodes[idx].adj;
                    adjList.erase(std::remove(adjList.begin(), adjList.end(), static_cast<int>(i)), adjList.end());
                }
                nodes.erase(nodes.begin() + i);
                // rebuild indices to stay consistent
                for (auto &n : nodes)
                {
                    for (auto &v : n.adj)
                    {
                        if (v > static_cast<int>(i))
                            --v;
                    }
                }
                changed = true;
            }
            else
            {
                ++i;
            }
        }
    }
}

double Graph::dotToSegment(const CityModel::Vec2 &p, const CityModel::Vec2 &start, const CityModel::Vec2 &dir) const
{
    CityModel::Vec2 diff = p.clone().sub(start);
    return diff.dot(dir);
}
