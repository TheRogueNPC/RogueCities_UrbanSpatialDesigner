#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Urban::PolygonUtil {

    [[nodiscard]] double area(const std::vector<Core::Vec2>& ring);
    [[nodiscard]] Core::Vec2 centroid(const std::vector<Core::Vec2>& ring);
    [[nodiscard]] bool insidePolygon(const Core::Vec2& p, const std::vector<Core::Vec2>& ring);
    [[nodiscard]] std::vector<Core::Vec2> closed(const std::vector<Core::Vec2>& ring);
    [[nodiscard]] Core::Bounds bounds(const std::vector<Core::Vec2>& ring);

} // namespace RogueCity::Generators::Urban::PolygonUtil

