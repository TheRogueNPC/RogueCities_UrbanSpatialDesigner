#pragma once

#include <vector>

#include "CityModel.h"
#include "Graph.h"
#include "TensorField.h"

struct PolygonParams
{
    int maxLength{20};
    double minArea{80.0};
    double shrinkSpacing{4.0};
    double chanceNoDivide{1.0};
};

class PolygonFinder
{
public:
    PolygonFinder(const std::vector<Node> &nodes,
                  const PolygonParams &params,
                  const TensorField::TensorField &field);

    const std::vector<std::vector<CityModel::Vec2>> &getPolygons() const { return polygons; }
    void findPolygons();
    void reset();

private:
    std::vector<Node> graphNodes;
    PolygonParams params;
    const TensorField::TensorField &field;
    std::vector<std::vector<CityModel::Vec2>> polygons;
};
