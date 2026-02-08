#pragma once

#include <vector>
#include <set>

#include "CityModel.h"

struct Segment
{
    CityModel::Vec2 from;
    CityModel::Vec2 to;
};

struct Node
{
    CityModel::Vec2 value;
    std::vector<int> adj;
};

class Graph
{
public:
    Graph(const std::vector<std::vector<CityModel::Vec2>> &streamlines, double dstep, bool deleteDangling = false);
    std::vector<Node> nodes;
    std::vector<CityModel::Vec2> intersections;

private:
    int addOrGetNode(const CityModel::Vec2 &p);
    bool segmentIntersection(const CityModel::Vec2 &a, const CityModel::Vec2 &b,
                             const CityModel::Vec2 &c, const CityModel::Vec2 &d,
                             CityModel::Vec2 &out) const;
    double dotToSegment(const CityModel::Vec2 &p, const CityModel::Vec2 &start, const CityModel::Vec2 &dir) const;
    void removeDangling();

    double mergeRadius;
};
