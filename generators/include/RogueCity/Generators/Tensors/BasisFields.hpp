#pragma once
#include "RogueCity/Core/Types.hpp"
#include <cmath>
#include <numbers>
#include <memory>

namespace RogueCity::Generators {

    using namespace Core;

    /// Base class for tensor basis field types (Strategy pattern)
    class BasisField {
    public:
        Vec2 center;
        double radius;
        double decay;

        BasisField(const Vec2& center, double radius, double decay)
            : center(center), radius(radius), decay(decay) {}

        virtual ~BasisField() = default;

        /// Sample tensor at world position
        [[nodiscard]] virtual Tensor2D sample(const Vec2& p) const = 0;

        /// Compute influence weight at position (exponential decay)
        [[nodiscard]] double getWeight(const Vec2& p) const {
            double dist = p.distanceTo(center);
            if (dist > radius) return 0.0;
            double norm_dist = dist / radius;
            return std::exp(-decay * norm_dist);
        }
    };

    // ===== RADIAL BASIS FIELD (Paris-style) =====

    /// Generates concentric circular roads (e.g., Paris Arc de Triomphe)
    class RadialField : public BasisField {
    public:
        RadialField(const Vec2& center, double radius, double decay)
            : BasisField(center, radius, decay) {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            Vec2 dir = p - center;
            double angle = std::atan2(dir.y, dir.x);
            // Tangent to circle (perpendicular to radius)
            return Tensor2D::fromAngle(angle + std::numbers::pi * 0.5);
        }
    };

    // ===== GRID BASIS FIELD (Manhattan-style) =====

    /// Generates orthogonal grid roads (e.g., Manhattan, Chicago)
    class GridField : public BasisField {
    public:
        double theta;  // Primary grid orientation (radians)

        GridField(const Vec2& center, double radius, double theta, double decay)
            : BasisField(center, radius, decay), theta(theta) {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            // Alternate between theta and theta + π/2 based on position
            // Creates orthogonal grid effect
            int cell_x = static_cast<int>(p.x / 50.0);  // 50m grid cells
            int cell_y = static_cast<int>(p.y / 50.0);
            bool use_primary = (cell_x + cell_y) % 2 == 0;
            
            double angle = use_primary ? theta : (theta + std::numbers::pi * 0.5);
            return Tensor2D::fromAngle(angle);
        }
    };

    // ===== DELTA BASIS FIELD (Organic 3-way junctions) =====

    enum class DeltaTerminal {
        North, South, East, West,
        NorthEast, NorthWest, SouthEast, SouthWest
    };

    /// Generates organic 3-way intersections (e.g., hillside towns)
    class DeltaField : public BasisField {
    public:
        DeltaTerminal terminal;

        DeltaField(const Vec2& center, double radius, DeltaTerminal terminal, double decay)
            : BasisField(center, radius, decay), terminal(terminal) {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            // Point toward terminal direction from center
            Vec2 terminal_dir = getTerminalDirection();
            Vec2 to_p = p - center;
            
            // Blend between radial and terminal direction
            double blend = std::min(1.0, to_p.length() / radius);
            Vec2 dir = lerp(to_p, terminal_dir, blend);
            
            return Tensor2D::fromVector(dir);
        }

    private:
        [[nodiscard]] Vec2 getTerminalDirection() const {
            switch (terminal) {
                case DeltaTerminal::North: return Vec2(0, -1);
                case DeltaTerminal::South: return Vec2(0, 1);
                case DeltaTerminal::East: return Vec2(1, 0);
                case DeltaTerminal::West: return Vec2(-1, 0);
                case DeltaTerminal::NorthEast: return Vec2(0.707, -0.707);
                case DeltaTerminal::NorthWest: return Vec2(-0.707, -0.707);
                case DeltaTerminal::SouthEast: return Vec2(0.707, 0.707);
                case DeltaTerminal::SouthWest: return Vec2(-0.707, 0.707);
                default: return Vec2(0, 1);
            }
        }
    };

    // ===== GRID CORRECTIVE FIELD (Straightens organic roads) =====

    /// Straightens and gridifies nearby roads (e.g., hybrid planning like DC)
    class GridCorrectiveField : public BasisField {
    public:
        double theta;  // Target grid orientation

        GridCorrectiveField(const Vec2& center, double radius, double theta, double decay)
            : BasisField(center, radius, decay), theta(theta) {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            // Strong alignment to grid orientation
            return Tensor2D::fromAngle(theta);
        }
    };

} // namespace RogueCity::Generators
