#include "GridStorage.h"

#include <cmath>

using CityModel::Vec2;

GridStorage::GridStorage(const Vec2 &worldDimensionsIn,
                         const Vec2 &originIn,
                         double dsepIn)
    : worldDimensions(worldDimensionsIn),
      origin(originIn),
      dsep(dsepIn),
      dsepSq(dsepIn * dsepIn)
{
    gridDimensions = Vec2{
        std::max(1.0, std::floor(worldDimensions.x / dsep)),
        std::max(1.0, std::floor(worldDimensions.y / dsep))};
    grid.resize(static_cast<std::size_t>(gridDimensions.x));
    for (std::size_t x = 0; x < static_cast<std::size_t>(gridDimensions.x); ++x)
    {
        grid[x].resize(static_cast<std::size_t>(gridDimensions.y));
    }
}

void GridStorage::addAll(const GridStorage &other)
{
    for (std::size_t x = 0; x < other.grid.size(); ++x)
    {
        for (std::size_t y = 0; y < other.grid[x].size(); ++y)
        {
            for (const auto &sample : other.grid[x][y])
            {
                addSample(sample);
            }
        }
    }
}

void GridStorage::addPolyline(const std::vector<Vec2> &line)
{
    for (const auto &v : line)
        addSample(v);
}

void GridStorage::addSample(const Vec2 &v)
{
    const Vec2 coords = getSampleCoords(v);
    const std::size_t xi = static_cast<std::size_t>(coords.x);
    const std::size_t yi = static_cast<std::size_t>(coords.y);
    if (xi < grid.size() && yi < grid[xi].size())
    {
        grid[xi][yi].push_back(v);
    }
}

bool GridStorage::isValidSample(const Vec2 &v, double dSq) const
{
    const Vec2 coords = getSampleCoords(v);
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            Vec2 cell{coords.x + dx, coords.y + dy};
            if (!outOfBounds(cell, gridDimensions))
            {
                const auto &bucket = grid[static_cast<std::size_t>(cell.x)][static_cast<std::size_t>(cell.y)];
                if (!farFromVectors(v, bucket, dSq))
                    return false;
            }
        }
    }
    return true;
}

std::vector<Vec2> GridStorage::getNearbyPoints(const Vec2 &v, double distance) const
{
    std::vector<Vec2> out;
    const double radius = std::ceil((distance / dsep) - 0.5);
    const Vec2 coords = getSampleCoords(v);
    for (int dx = static_cast<int>(-radius); dx <= static_cast<int>(radius); ++dx)
    {
        for (int dy = static_cast<int>(-radius); dy <= static_cast<int>(radius); ++dy)
        {
            Vec2 cell{coords.x + dx, coords.y + dy};
            if (!outOfBounds(cell, gridDimensions))
            {
                const auto &bucket = grid[static_cast<std::size_t>(cell.x)][static_cast<std::size_t>(cell.y)];
                out.insert(out.end(), bucket.begin(), bucket.end());
            }
        }
    }
    return out;
}

Vec2 GridStorage::getSampleCoords(const Vec2 &worldV) const
{
    Vec2 v{worldV.x - origin.x, worldV.y - origin.y};
    if (outOfBounds(v, worldDimensions))
    {
        return Vec2{0.0, 0.0};
    }
    return Vec2{std::floor(v.x / dsep), std::floor(v.y / dsep)};
}

bool GridStorage::outOfBounds(const Vec2 &gridV, const Vec2 &bounds) const
{
    return gridV.x < 0.0 || gridV.y < 0.0 || gridV.x >= bounds.x || gridV.y >= bounds.y;
}

bool GridStorage::farFromVectors(const Vec2 &v, const std::vector<Vec2> &vecs, double dSq) const
{
    for (const auto &sample : vecs)
    {
        if (&sample != &v)
        {
            const double distSq = (sample.x - v.x) * (sample.x - v.x) + (sample.y - v.y) * (sample.y - v.y);
            if (distSq < dSq)
                return false;
        }
    }
    return true;
}
