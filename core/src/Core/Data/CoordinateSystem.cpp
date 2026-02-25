/**
 * @class CoordinateSystem
 * @brief Handles conversion between world coordinates, UV coordinates, and pixel coordinates within a bounded region.
 *
 * The CoordinateSystem class provides methods to map between world-space coordinates, normalized UV coordinates,
 * and pixel coordinates based on a specified bounding box and texture resolution. It supports reconfiguration
 * of bounds and resolution, and ensures proper clamping and conversion for accurate spatial representation.
 *
 * @param world_bounds The spatial bounds of the world region.
 * @param texture_resolution The resolution (number of pixels) for mapping.
 *
 * @method reconfigure Reinitializes the coordinate system with new bounds and resolution.
 * @method worldToUV Converts world coordinates to normalized UV coordinates (range [0,1]).
 * @method worldToPixel Converts world coordinates to pixel coordinates, clamped to valid range.
 * @method uvToWorld Converts normalized UV coordinates to world coordinates.
 * @method pixelToWorld Converts pixel coordinates to world coordinates, using pixel center.
 * @method isInBounds Checks if a world coordinate is within the defined bounds.
 */
 
#include "RogueCity/Core/Data/CoordinateSystem.hpp"

#include <algorithm>
#include <cmath>

namespace RogueCity::Core::Data {

    namespace {
        [[nodiscard]] double Clamp01(double v) {
            return std::clamp(v, 0.0, 1.0);
        }
    } // namespace

    CoordinateSystem::CoordinateSystem(const Bounds& world_bounds, int texture_resolution) {
        reconfigure(world_bounds, texture_resolution);
    }

    void CoordinateSystem::reconfigure(const Bounds& world_bounds, int texture_resolution) {
        world_bounds_ = world_bounds;
        texture_resolution_ = std::max(1, texture_resolution);

        const double world_width = std::max(0.0, world_bounds_.width());
        const double world_height = std::max(0.0, world_bounds_.height());
        const double longest_dimension = std::max(world_width, world_height);
        meters_per_pixel_ = (longest_dimension > 0.0)
            ? (longest_dimension / static_cast<double>(texture_resolution_))
            : 0.0;
    }

    Vec2 CoordinateSystem::worldToUV(const Vec2& world) const {
        const double width = world_bounds_.width();
        const double height = world_bounds_.height();

        if (width <= 0.0 || height <= 0.0) {
            return Vec2{};
        }

        return Vec2(
            (world.x - world_bounds_.min.x) / width,
            (world.y - world_bounds_.min.y) / height);
    }

    Int2 CoordinateSystem::worldToPixel(const Vec2& world) const {
        const Vec2 uv = worldToUV(world);
        const double x = Clamp01(uv.x) * static_cast<double>(texture_resolution_);
        const double y = Clamp01(uv.y) * static_cast<double>(texture_resolution_);

        Int2 pixel{};
        pixel.x = std::clamp(static_cast<int>(std::floor(x)), 0, texture_resolution_ - 1);
        pixel.y = std::clamp(static_cast<int>(std::floor(y)), 0, texture_resolution_ - 1);
        return pixel;
    }

    Vec2 CoordinateSystem::uvToWorld(const Vec2& uv) const {
        return Vec2(
            world_bounds_.min.x + uv.x * world_bounds_.width(),
            world_bounds_.min.y + uv.y * world_bounds_.height());
    }

    Vec2 CoordinateSystem::pixelToWorld(const Int2& pixel) const {
        const int px = std::clamp(pixel.x, 0, texture_resolution_ - 1);
        const int py = std::clamp(pixel.y, 0, texture_resolution_ - 1);
        const Vec2 uv(
            (static_cast<double>(px) + 0.5) / static_cast<double>(texture_resolution_),
            (static_cast<double>(py) + 0.5) / static_cast<double>(texture_resolution_));
        return uvToWorld(uv);
    }

    bool CoordinateSystem::isInBounds(const Vec2& world) const {
        return world_bounds_.contains(world);
    }

} // namespace RogueCity::Core::Data

