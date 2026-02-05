#pragma once

#include <cmath>
#include <array>

namespace RogueCity::Core {

    /**
     * @brief Lightweight 2D vector for city generation
     *
     * Pure data structure with no external dependencies (except std math).
     * Used for positions, directions, and 2D geometry operations.
     */
    struct Vec2 {
        double x{ 0.0 };
        double y{ 0.0 };

        // ===== CONSTRUCTORS =====
        constexpr Vec2() = default;
        constexpr Vec2(double x, double y) : x(x), y(y) {}

        // ===== MUTATING OPERATIONS =====
        Vec2& add(const Vec2& v) { x += v.x; y += v.y; return *this; }
        Vec2& sub(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
        Vec2& multiply(double s) { x *= s; y *= s; return *this; }
        Vec2& divide(double s) { x /= s; y /= s; return *this; }
        Vec2& normalize();
        Vec2& setLength(double len);
        Vec2& rotateAround(const Vec2& center, double angle_radians);
        Vec2& set(double x_, double y_) { x = x_; y = y_; return *this; }

        // ===== NON-MUTATING QUERIES =====
        [[nodiscard]] double length() const { return std::sqrt(x * x + y * y); }
        [[nodiscard]] double lengthSquared() const { return x * x + y * y; }
        [[nodiscard]] double dot(const Vec2& v) const { return x * v.x + y * v.y; }
        [[nodiscard]] double cross(const Vec2& v) const { return x * v.y - y * v.x; }
        [[nodiscard]] double angle() const { return std::atan2(y, x); }
        [[nodiscard]] double distanceTo(const Vec2& v) const;
        [[nodiscard]] double distanceToSquared(const Vec2& v) const;
        [[nodiscard]] bool equals(const Vec2& v, double epsilon = 1e-9) const;
        [[nodiscard]] Vec2 negated() const { return Vec2(-x, -y); }
        [[nodiscard]] Vec2 clone() const { return *this; }

        // ===== OPERATORS =====
        Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
        Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
        Vec2 operator*(double s) const { return Vec2(x * s, y * s); }
        Vec2 operator/(double s) const { return Vec2(x / s, y / s); }
        Vec2& operator+=(const Vec2& v) { return add(v); }
        Vec2& operator-=(const Vec2& v) { return sub(v); }
        Vec2& operator*=(double s) { return multiply(s); }
        Vec2& operator/=(double s) { return divide(s); }
        bool operator==(const Vec2& v) const { return equals(v); }
        bool operator!=(const Vec2& v) const { return !equals(v); }
    };

    // ===== FREE FUNCTIONS =====
    [[nodiscard]] inline double distance(const Vec2& a, const Vec2& b) {
        return a.distanceTo(b);
    }

    [[nodiscard]] inline Vec2 lerp(const Vec2& a, const Vec2& b, double t) {
        return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    [[nodiscard]] inline double angleBetween(const Vec2& v1, const Vec2& v2) {
        return std::atan2(v1.cross(v2), v1.dot(v2));
    }

    [[nodiscard]] inline bool isLeft(const Vec2& line_point, const Vec2& line_dir, const Vec2& point) {
        Vec2 to_point = point - line_point;
        return line_dir.cross(to_point) > 0.0;
    }

} // namespace RogueCity::Core
