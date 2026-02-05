#include "RogueCity/Core/Math/Vec2.hpp"
#include <cmath>

namespace RogueCity::Core {

    Vec2& Vec2::normalize() {
        double len = length();
        if (len > 1e-9) {
            x /= len;
            y /= len;
        }
        return *this;
    }

    Vec2& Vec2::setLength(double len) {
        normalize();
        return multiply(len);
    }

    Vec2& Vec2::rotateAround(const Vec2& center, double angle_radians) {
        double dx = x - center.x;
        double dy = y - center.y;
        double cos_a = std::cos(angle_radians);
        double sin_a = std::sin(angle_radians);
        x = center.x + dx * cos_a - dy * sin_a;
        y = center.y + dx * sin_a + dy * cos_a;
        return *this;
    }

    double Vec2::distanceTo(const Vec2& v) const {
        double dx = x - v.x;
        double dy = y - v.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    double Vec2::distanceToSquared(const Vec2& v) const {
        double dx = x - v.x;
        double dy = y - v.y;
        return dx * dx + dy * dy;
    }

    bool Vec2::equals(const Vec2& v, double epsilon) const {
        return std::abs(x - v.x) < epsilon && std::abs(y - v.y) < epsilon;
    }

} // namespace RogueCity::Core
