#pragma once

#include <vector>

#include "CityModel.h"

namespace PolygonUtil
{

    double polygonArea(const std::vector<CityModel::Vec2> &poly);
    CityModel::Vec2 averagePoint(const std::vector<CityModel::Vec2> &poly);
    bool insidePolygon(const CityModel::Vec2 &point, const std::vector<CityModel::Vec2> &polygon);
    bool pointInRectangle(const CityModel::Vec2 &point, const CityModel::Vec2 &origin, const CityModel::Vec2 &dimensions);

    // Clip rectangle by line endpoints, returning smaller side polygon.
    std::vector<CityModel::Vec2> sliceRectangle(const CityModel::Vec2 &origin,
                                                const CityModel::Vec2 &worldDimensions,
                                                const CityModel::Vec2 &p1,
                                                const CityModel::Vec2 &p2);

    // Returns polygon representing half-plane of rectangle on smaller side of line polyline.
    std::vector<CityModel::Vec2> lineRectanglePolygonIntersection(const CityModel::Vec2 &origin,
                                                                  const CityModel::Vec2 &worldDimensions,
                                                                  const std::vector<CityModel::Vec2> &line);

    // Simple grow/shrink of polygon or polyline.
    std::vector<CityModel::Vec2> resizeGeometry(const std::vector<CityModel::Vec2> &geometry,
                                                double spacing,
                                                bool isPolygon = true);

    // Recursive subdivision of polygon; may return empty if below minArea.
    std::vector<std::vector<CityModel::Vec2>> subdividePolygon(const std::vector<CityModel::Vec2> &poly,
                                                               double minArea);

} // namespace PolygonUtil
