#pragma once

#include <vector>

#include "CityModel.h"

// Cartesian grid accelerated storage for sampled points.
class GridStorage
{
public:
    GridStorage(const CityModel::Vec2 &worldDimensions,
                const CityModel::Vec2 &origin,
                double dsep);

    void addAll(const GridStorage &other);
    void addPolyline(const std::vector<CityModel::Vec2> &line);

    // Does not enforce separation.
    void addSample(const CityModel::Vec2 &v);

    // Tests whether v is at least sqrt(dSq) away from existing samples.
    bool isValidSample(const CityModel::Vec2 &v, double dSq) const;

    // Returns all points in cells within distance (approx square radius).
    std::vector<CityModel::Vec2> getNearbyPoints(const CityModel::Vec2 &v, double distance) const;

private:
    CityModel::Vec2 getSampleCoords(const CityModel::Vec2 &worldV) const;
    bool outOfBounds(const CityModel::Vec2 &gridV, const CityModel::Vec2 &bounds) const;
    bool farFromVectors(const CityModel::Vec2 &v, const std::vector<CityModel::Vec2> &vecs, double dSq) const;

    CityModel::Vec2 worldDimensions;
    CityModel::Vec2 origin;
    CityModel::Vec2 gridDimensions;
    double dsep;
    double dsepSq;
    std::vector<std::vector<std::vector<CityModel::Vec2>>> grid;
};
