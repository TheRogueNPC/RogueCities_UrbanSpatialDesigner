#pragma once
#include "RogueCity/Core/Types.hpp"
#include <algorithm>
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

        /// Compute influence weight at position (exponential decay with falloff)
        [[nodiscard]] double getWeight(const Vec2& p) const {
            double dist = p.distanceTo(center);
            // RC-0.09-Test P0 fix: Add 20% falloff margin to contain generation
            const double falloff_margin = radius * 0.2;
            const double max_dist = radius + falloff_margin;
            
            if (dist > max_dist) return 0.0;
            
            // Smoothstep falloff in margin zone
            if (dist > radius) {
                const double falloff_t = (dist - radius) / falloff_margin;
                const double falloff = 1.0 - falloff_t; // Linear falloff
                // Apply smoothstep for smooth curve: smoothstep(0, 1, falloff)
                const double t = std::clamp(falloff, 0.0, 1.0);
                const double smooth_falloff = t * t * (3.0 - 2.0 * t);
                return std::exp(-decay * (dist / max_dist)) * smooth_falloff;
            }
            
            // Inside radius: standard exponential decay
            double norm_dist = dist / radius;
            return std::exp(-decay * norm_dist);
        }
    };

    namespace detail {
        [[nodiscard]] inline Vec2 perp(const Vec2& v) { return Vec2(-v.y, v.x); }

        [[nodiscard]] inline double frac(double x) {
            return x - std::floor(x);
        }

        [[nodiscard]] inline double nearest_line_distance(double x) {
            // Distance to nearest integer in normalized cell coordinates [0..0.5].
            const double f = frac(x);
            return std::min(f, 1.0 - f);
        }

        [[nodiscard]] inline double smoothstep(double edge0, double edge1, double x) {
            const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
            return t * t * (3.0 - 2.0 * t);
        }
    } // namespace detail

    // ===== ORGANIC BASIS FIELD (Curvy natural flow) =====

    class OrganicField : public BasisField {
    public:
        double theta;
        float curviness; // [0..1]

        OrganicField(const Vec2& center, double radius, double theta, float curviness, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , curviness(std::clamp(curviness, 0.0f, 1.0f))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 d = p - center;
            const double base = std::atan2(d.y, d.x) + std::numbers::pi * 0.5; // gentle swirl

            // Deterministic pseudo-noise (cheap, no dependencies)
            const double freq = 0.002 + 0.018 * static_cast<double>(curviness);  // 1/m
            const double amp = 0.10 + 1.10 * static_cast<double>(curviness);    // radians
            const double n =
                std::sin((p.x + center.y) * freq) +
                std::cos((p.y - center.x) * freq * 1.3) +
                0.5 * std::sin((p.x + p.y) * freq * 0.7);

            const double angle = base + amp * (n / 2.5) + theta * 0.15;
            return Tensor2D::fromAngle(angle);
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

    // ===== RADIAL HUB-AND-SPOKE (Star/Crescent) =====

    class RadialHubSpokeField : public BasisField {
    public:
        int spokes;

        RadialHubSpokeField(const Vec2& center, double radius, int spokes, double decay)
            : BasisField(center, radius, decay)
            , spokes(std::max(3, spokes))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 d = p - center;
            const double dist = std::max(1e-6, d.length());
            const double ang = std::atan2(d.y, d.x);

            // Create crisp spoke directions by snapping angle into spoke bins.
            const double bin = (2.0 * std::numbers::pi) / static_cast<double>(spokes);
            const double spoke_ang = std::round(ang / bin) * bin;

            // Near the hub: spokes (radial). Toward the outside: crescents (tangent).
            const double t = detail::smoothstep(0.15, 0.85, dist / radius);
            const double radial = spoke_ang;
            const double tangent = ang + std::numbers::pi * 0.5;
            const double angle = radial * (1.0 - t) + tangent * t;

            return Tensor2D::fromAngle(angle);
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
            // Alternate between theta and theta + ?/2 based on position
            // Creates orthogonal grid effect
            int cell_x = static_cast<int>(p.x / 50.0);  // 50m grid cells
            int cell_y = static_cast<int>(p.y / 50.0);
            bool use_primary = (cell_x + cell_y) % 2 == 0;
            
            double angle = use_primary ? theta : (theta + std::numbers::pi * 0.5);
            return Tensor2D::fromAngle(angle);
        }
    };

    // ===== HEXAGONAL BASIS FIELD (Six-sided blocks) =====

    class HexagonalField : public BasisField {
    public:
        double theta;
        double spacing;

        HexagonalField(const Vec2& center, double radius, double theta, double spacing, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , spacing(std::max(10.0, spacing))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            // Three families of parallel lines -> hexagonal blocks (dual lattice).
            const Vec2 rel = p - center;
            const double angles[3] = { theta, theta + std::numbers::pi / 3.0, theta + 2.0 * std::numbers::pi / 3.0 };

            int best_i = 0;
            double best_dist = 1e9;
            for (int i = 0; i < 3; ++i) {
                const double a = angles[i];
                const Vec2 dir(std::cos(a), std::sin(a));
                const Vec2 n = detail::perp(dir);
                const double coord = rel.dot(n) / spacing;
                const double dist = detail::nearest_line_distance(coord);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_i = i;
                }
            }

            return Tensor2D::fromAngle(angles[best_i]);
        }
    };

    // ===== STEM BASIS FIELD (Tree-like branching hierarchy) =====

    class StemField : public BasisField {
    public:
        double theta;
        float branch_angle;

        StemField(const Vec2& center, double radius, double theta, float branch_angle, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , branch_angle(std::clamp(branch_angle, 0.0f, static_cast<float>(std::numbers::pi)))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 rel = p - center;
            const Vec2 trunk_dir(std::cos(theta), std::sin(theta));
            const Vec2 trunk_n = detail::perp(trunk_dir);

            const double lateral = rel.dot(trunk_n);
            const double trunk_width = std::max(15.0, radius * 0.08);

            if (std::abs(lateral) < trunk_width) {
                return Tensor2D::fromAngle(theta);
            }

            const double side = (lateral < 0.0) ? -1.0 : 1.0;
            const double angle = theta + side * static_cast<double>(branch_angle);
            return Tensor2D::fromAngle(angle);
        }
    };

    // ===== LOOSE GRID BASIS FIELD (Imperfect grid) =====

    class LooseGridField : public BasisField {
    public:
        double theta;
        float jitter; // [0..1]

        LooseGridField(const Vec2& center, double radius, double theta, float jitter, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , jitter(std::clamp(jitter, 0.0f, 1.0f))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const int cell_x = static_cast<int>(p.x / 60.0);  // slightly larger than the strict grid
            const int cell_y = static_cast<int>(p.y / 60.0);

            const bool use_primary = (cell_x + cell_y) % 2 == 0;
            const double base = use_primary ? theta : (theta + std::numbers::pi * 0.5);

            const double jitter_rad = static_cast<double>(jitter) * (std::numbers::pi / 10.0); // up to ~18°
            const double hash = std::sin(cell_x * 12.9898 + cell_y * 78.233) * 43758.5453;
            const double local = (detail::frac(hash) - 0.5) * 2.0;

            return Tensor2D::fromAngle(base + local * jitter_rad);
        }
    };

    // ===== SUBURBAN BASIS FIELD (Cul-de-sac loops) =====

    class SuburbanField : public BasisField {
    public:
        float loop_strength; // [0..1]

        SuburbanField(const Vec2& center, double radius, float loop_strength, double decay)
            : BasisField(center, radius, decay)
            , loop_strength(std::clamp(loop_strength, 0.0f, 1.0f))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 rel = p - center;
            const double dist = std::max(1e-6, rel.length());
            const double ang = std::atan2(rel.y, rel.x);

            const double radial = ang;
            const double tangential = ang + std::numbers::pi * 0.5;

            // Push tangential (loops) closer to the outside as strength increases.
            const double loop_start = 0.45 + 0.25 * (1.0 - static_cast<double>(loop_strength));
            const double t = detail::smoothstep(loop_start, 1.0, dist / radius);
            const double angle = radial * (1.0 - t) + tangential * t;

            return Tensor2D::fromAngle(angle);
        }
    };

    // ===== SUPERBLOCK BASIS FIELD (Large blocks, coarse division) =====

    class SuperblockField : public BasisField {
    public:
        double theta;
        float block_size;

        SuperblockField(const Vec2& center, double radius, double theta, float block_size, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , block_size(std::max(50.0f, block_size))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const double cell = static_cast<double>(block_size);
            const int bx = static_cast<int>(p.x / cell);
            const int by = static_cast<int>(p.y / cell);
            const bool use_primary = (bx + by) % 2 == 0;
            const double angle = use_primary ? theta : (theta + std::numbers::pi * 0.5);
            return Tensor2D::fromAngle(angle);
        }
    };

    // ===== LINEAR BASIS FIELD (Parallel corridors) =====

    class LinearField : public BasisField {
    public:
        double theta;

        LinearField(const Vec2& center, double radius, double theta, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            (void)p;
            return Tensor2D::fromAngle(theta);
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
            // Fix: remove unused parameter warning by casting to void
            (void)p;
            // Strong alignment to grid orientation
            return Tensor2D::fromAngle(theta);
        }
    };

} // namespace RogueCity::Generators
