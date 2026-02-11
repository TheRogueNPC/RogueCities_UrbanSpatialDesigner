#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"

namespace RogueCity::Core::Data {

    struct Int2 {
        int x{ 0 };
        int y{ 0 };
    };

    class CoordinateSystem {
    public:
        CoordinateSystem() = default;
        CoordinateSystem(const Bounds& world_bounds, int texture_resolution);

        void reconfigure(const Bounds& world_bounds, int texture_resolution);

        [[nodiscard]] Vec2 worldToUV(const Vec2& world) const;
        [[nodiscard]] Int2 worldToPixel(const Vec2& world) const;

        [[nodiscard]] Vec2 uvToWorld(const Vec2& uv) const;
        [[nodiscard]] Vec2 pixelToWorld(const Int2& pixel) const;

        [[nodiscard]] double metersPerPixel() const { return meters_per_pixel_; }
        [[nodiscard]] int resolution() const { return texture_resolution_; }
        [[nodiscard]] const Bounds& bounds() const { return world_bounds_; }

        [[nodiscard]] bool isInBounds(const Vec2& world) const;

    private:
        Bounds world_bounds_{};
        int texture_resolution_{ 0 };
        double meters_per_pixel_{ 0.0 };
    };

} // namespace RogueCity::Core::Data

