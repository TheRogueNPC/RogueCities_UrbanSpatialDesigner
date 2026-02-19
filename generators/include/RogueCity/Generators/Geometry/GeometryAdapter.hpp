#pragma once

#include "RogueCity/Core/Types.hpp"

#include <cstdint>
#include <vector>
/// Thin adapter around geometry backends used by generation systems.
namespace RogueCity::Generators::Geometry {

    enum class Backend : uint8_t {  
        LegacyGeos = 0,
        BoostGeometry = 1,
    };

    class GeometryAdapter {
    public:
        using Ring = std::vector<Core::Vec2>;

        [[nodiscard]] static Backend backend() noexcept;
        [[nodiscard]] static const char* backendName() noexcept;

        [[nodiscard]] static double distance(const Core::Vec2& a, const Core::Vec2& b) noexcept;
        [[nodiscard]] static bool intersects(const Core::Bounds& a, const Core::Bounds& b) noexcept;
        [[nodiscard]] static bool intersects(const Ring& a, const Ring& b);
        [[nodiscard]] static Ring convexHull(const Ring& points);
        [[nodiscard]] static Ring simplify(const Ring& ring, double tolerance);
    };

} // namespace RogueCity::Generators::Geometry
