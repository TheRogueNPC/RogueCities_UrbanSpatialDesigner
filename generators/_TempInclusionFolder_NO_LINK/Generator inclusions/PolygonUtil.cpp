#include "PolygonUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
    double cross(const CityModel::Vec2 &a, const CityModel::Vec2 &b, const CityModel::Vec2 &c)
    {
        // (b - a) x (c - a)
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    bool segmentIntersect(const CityModel::Vec2 &p1, const CityModel::Vec2 &p2,
                          const CityModel::Vec2 &q1, const CityModel::Vec2 &q2,
                          CityModel::Vec2 *out)
    {
        const double s1_x = p2.x - p1.x;
        const double s1_y = p2.y - p1.y;
        const double s2_x = q2.x - q1.x;
        const double s2_y = q2.y - q1.y;

        const double denom = (-s2_x * s1_y + s1_x * s2_y);
        if (std::abs(denom) < 1e-9)
            return false;
        const double s = (-s1_y * (p1.x - q1.x) + s1_x * (p1.y - q1.y)) / denom;
        const double t = (s2_x * (p1.y - q1.y) - s2_y * (p1.x - q1.x)) / denom;
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            if (out)
            {
                out->x = p1.x + (t * s1_x);
                out->y = p1.y + (t * s1_y);
            }
            return true;
        }
        return false;
    }

    std::vector<CityModel::Vec2> sutherlandHodgman(const std::vector<CityModel::Vec2> &subject,
                                                   const CityModel::Vec2 &l1,
                                                   const CityModel::Vec2 &l2,
                                                   bool keepLeft)
    {
        std::vector<CityModel::Vec2> output = subject;
        std::vector<CityModel::Vec2> input;
        for (std::size_t i = 0; i < output.size(); ++i)
        {
            input = output;
            output.clear();
            if (input.empty())
                break;
            CityModel::Vec2 S = input.back();
            for (const auto &E : input)
            {
                const double cpE = cross(l1, l2, E);
                const double cpS = cross(l1, l2, S);
                const bool Ein = keepLeft ? cpE <= 0 : cpE >= 0;
                const bool Sin = keepLeft ? cpS <= 0 : cpS >= 0;
                if (Ein)
                {
                    if (!Sin)
                    {
                        CityModel::Vec2 inter;
                        segmentIntersect(S, E, l1, l2, &inter);
                        output.push_back(inter);
                    }
                    output.push_back(E);
                }
                else if (Sin)
                {
                    CityModel::Vec2 inter;
                    segmentIntersect(S, E, l1, l2, &inter);
                    output.push_back(inter);
                }
                S = E;
            }
            break;
        }
        return output;
    }
} // namespace

namespace PolygonUtil
{

    double polygonArea(const std::vector<CityModel::Vec2> &poly)
    {
        double total = 0.0;
        if (poly.size() < 3)
            return 0.0;
        for (std::size_t i = 0; i < poly.size(); ++i)
        {
            const std::size_t j = (i + 1) % poly.size();
            total += poly[i].x * poly[j].y;
            total -= poly[j].x * poly[i].y;
        }
        return std::abs(total) * 0.5;
    }

    CityModel::Vec2 averagePoint(const std::vector<CityModel::Vec2> &poly)
    {
        CityModel::Vec2 sum{0, 0};
        if (poly.empty())
            return sum;
        for (const auto &v : poly)
        {
            sum.x += v.x;
            sum.y += v.y;
        }
        sum.x /= static_cast<double>(poly.size());
        sum.y /= static_cast<double>(poly.size());
        return sum;
    }

    bool insidePolygon(const CityModel::Vec2 &point, const std::vector<CityModel::Vec2> &polygon)
    {
        if (polygon.empty())
            return false;
        bool inside = false;
        for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
        {
            const double xi = polygon[i].x, yi = polygon[i].y;
            const double xj = polygon[j].x, yj = polygon[j].y;
            const bool intersect = ((yi > point.y) != (yj > point.y)) &&
                                   (point.x < (xj - xi) * (point.y - yi) / (yj - yi + 1e-12) + xi);
            if (intersect)
                inside = !inside;
        }
        return inside;
    }

    bool pointInRectangle(const CityModel::Vec2 &point, const CityModel::Vec2 &origin, const CityModel::Vec2 &dimensions)
    {
        return point.x >= origin.x && point.y >= origin.y &&
               point.x <= origin.x + dimensions.x &&
               point.y <= origin.y + dimensions.y;
    }

    std::vector<CityModel::Vec2> sliceRectangle(const CityModel::Vec2 &origin,
                                                const CityModel::Vec2 &worldDimensions,
                                                const CityModel::Vec2 &p1,
                                                const CityModel::Vec2 &p2)
    {
        std::vector<CityModel::Vec2> rect{
            {origin.x, origin.y},
            {origin.x + worldDimensions.x, origin.y},
            {origin.x + worldDimensions.x, origin.y + worldDimensions.y},
            {origin.x, origin.y + worldDimensions.y}};

        auto left = sutherlandHodgman(rect, p1, p2, true);
        auto right = sutherlandHodgman(rect, p1, p2, false);
        const double aLeft = polygonArea(left);
        const double aRight = polygonArea(right);
        if (aLeft == 0 && aRight == 0)
            return rect;
        return (aLeft < aRight) ? left : right;
    }

    std::vector<CityModel::Vec2> lineRectanglePolygonIntersection(const CityModel::Vec2 &origin,
                                                                  const CityModel::Vec2 &worldDimensions,
                                                                  const std::vector<CityModel::Vec2> &line)
    {
        if (line.size() < 2)
            return {};
        return sliceRectangle(origin, worldDimensions, line.front(), line.back());
    }

    std::vector<CityModel::Vec2> resizeGeometry(const std::vector<CityModel::Vec2> &geometry,
                                                double spacing,
                                                bool isPolygon)
    {
        if (geometry.empty())
            return {};
        CityModel::Vec2 center = averagePoint(geometry);
        std::vector<CityModel::Vec2> out;
        out.reserve(geometry.size());
        for (const auto &p : geometry)
        {
            CityModel::Vec2 dir = p.clone().sub(center);
            double len = dir.length();
            if (len == 0.0)
            {
                out.push_back(p);
                continue;
            }
            dir.divide(len);
            CityModel::Vec2 shifted = p.clone().add(CityModel::Vec2{dir.x * spacing, dir.y * spacing});
            out.push_back(shifted);
        }
        if (!isPolygon)
            return out;
        // ensure loop closure
        return out;
    }

    std::vector<std::vector<CityModel::Vec2>> subdividePolygon(const std::vector<CityModel::Vec2> &poly,
                                                               double minArea)
    {
        double area = polygonArea(poly);
        if (area < 0.5 * minArea)
            return {};
        if (area < 2 * minArea || poly.size() < 4)
            return {poly};

        // Find longest edge
        double longest = -1.0;
        std::size_t idx = 0;
        for (std::size_t i = 0; i < poly.size(); ++i)
        {
            std::size_t j = (i + 1) % poly.size();
            double d = std::sqrt(poly[i].distanceToSquared(poly[j]));
            if (d > longest)
            {
                longest = d;
                idx = i;
            }
        }
        CityModel::Vec2 a = poly[idx];
        CityModel::Vec2 b = poly[(idx + 1) % poly.size()];
        CityModel::Vec2 mid = CityModel::Vec2{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5};
        CityModel::Vec2 normal{a.y - b.y, -(a.x - b.x)};
        CityModel::Vec2 far = mid.clone().add(normal);

        auto left = sutherlandHodgman(poly, mid, far, true);
        auto right = sutherlandHodgman(poly, mid, far, false);
        std::vector<std::vector<CityModel::Vec2>> out;
        if (!left.empty())
        {
            auto subdivLeft = subdividePolygon(left, minArea);
            out.insert(out.end(), subdivLeft.begin(), subdivLeft.end());
        }
        if (!right.empty())
        {
            auto subdivRight = subdividePolygon(right, minArea);
            out.insert(out.end(), subdivRight.begin(), subdivRight.end());
        }
        return out;
    }

} // namespace PolygonUtil
