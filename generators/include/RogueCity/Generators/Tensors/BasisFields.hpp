#pragma once
#include "RogueCity/Core/Types.hpp"
#include <array>
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

        /// Compute influence weight at position.
        /// Contract: weight is 1.0 at center and falls to 0.0 exactly at radius.
        [[nodiscard]] virtual double getWeight(const Vec2& p) const {
            const double safe_radius = std::max(1e-6, radius);
            const double dist = p.distanceTo(center);
            if (dist >= safe_radius) {
                return 0.0;
            }

            // Smooth radial falloff [center=1 .. edge=0], then shape with decay.
            const double t = std::clamp(1.0 - (dist / safe_radius), 0.0, 1.0);
            const double smooth = t * t * (3.0 - 2.0 * t);
            const double exponent = std::max(0.1, decay);
            return std::pow(smooth, exponent);
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

        [[nodiscard]] inline double wrap_angle_pi(double angle) {
            constexpr double kPi = std::numbers::pi;
            while (angle > kPi * 0.5) {
                angle -= kPi;
            }
            while (angle < -kPi * 0.5) {
                angle += kPi;
            }
            return angle;
        }

        [[nodiscard]] inline double lerp_angle_pi(double from, double to, double t) {
            const double delta = wrap_angle_pi(to - from);
            return from + delta * std::clamp(t, 0.0, 1.0);
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
        double ring_rotation;
        std::array<std::array<float, 4>, 3> ring_knob_weights;

        RadialHubSpokeField(const Vec2& center, double radius, int spokes, double decay)
            : BasisField(center, radius, decay)
            , spokes(std::max(3, spokes))
            , ring_rotation(0.0)
            , ring_knob_weights{{
                {{1.0f, 1.0f, 1.0f, 1.0f}},
                {{1.0f, 1.0f, 1.0f, 1.0f}},
                {{1.0f, 1.0f, 1.0f, 1.0f}}
            }}
        {}

        RadialHubSpokeField(
            const Vec2& center,
            double radius,
            int spokes,
            double ring_rotation,
            const std::array<std::array<float, 4>, 3>& ring_knob_weights,
            double decay)
            : BasisField(center, radius, decay)
            , spokes(std::max(3, spokes))
            , ring_rotation(ring_rotation)
            , ring_knob_weights(ring_knob_weights)
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 d = p - center;
            const double dist = std::max(1e-6, d.length());
            const double ang = std::atan2(d.y, d.x);
            const double normalized_radius = std::clamp(dist / std::max(1e-6, radius), 0.0, 1.5);

            const auto sampled_knob = [&](int ring_index, double angle) {
                const double shifted = angle - ring_rotation;
                const double wrapped = std::fmod(shifted + 2.0 * std::numbers::pi, 2.0 * std::numbers::pi);
                const double sector = wrapped / (std::numbers::pi * 0.5);
                const int i0 = static_cast<int>(std::floor(sector)) % 4;
                const int i1 = (i0 + 1) % 4;
                const double t = sector - std::floor(sector);
                const double a = std::max(0.25, static_cast<double>(ring_knob_weights[ring_index][static_cast<size_t>(i0)]));
                const double b = std::max(0.25, static_cast<double>(ring_knob_weights[ring_index][static_cast<size_t>(i1)]));
                return a + (b - a) * t;
            };

            const double inner_t = detail::smoothstep(0.0, 0.5, normalized_radius);
            const double outer_t = detail::smoothstep(0.5, 1.0, normalized_radius);
            const double k0 = sampled_knob(0, ang);
            const double k1 = sampled_knob(1, ang);
            const double k2 = sampled_knob(2, ang);
            const double ring_knob = (1.0 - inner_t) * k0 + inner_t * (1.0 - outer_t) * k1 + inner_t * outer_t * k2;
            const double effective_radius = std::max(6.0, radius * ring_knob);

            // Create crisp spoke directions by snapping angle into spoke bins.
            const double bin = (2.0 * std::numbers::pi) / static_cast<double>(spokes);
            const double spoke_ang = std::round(ang / bin) * bin;

            // Near the hub: spokes (radial). Toward the outside: crescents (tangent).
            const double t = detail::smoothstep(0.15, 0.85, dist / effective_radius);
            const double radial = spoke_ang;
            const double tangent = ang + std::numbers::pi * 0.5;
            const double angle = detail::lerp_angle_pi(radial, tangent, t);

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

    // ===== FEATURE SUPPORT FIELDS =====

    class StrongLinearCorridorField : public BasisField {
    public:
        double theta;
        double corridor_half_width;

        StrongLinearCorridorField(const Vec2& center, double radius, double theta, double corridor_half_width, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , corridor_half_width(std::max(6.0, corridor_half_width))
        {}

        [[nodiscard]] double getCorridorWeight(const Vec2& p) const {
            const Vec2 dir(std::cos(theta), std::sin(theta));
            const Vec2 normal = detail::perp(dir);
            const double lateral = std::abs((p - center).dot(normal));
            const double lane = 1.0 - std::clamp(lateral / corridor_half_width, 0.0, 1.0);
            return lane * lane;
        }

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            Tensor2D tensor = Tensor2D::fromAngle(theta);
            const double w = getCorridorWeight(p);
            tensor.scale(0.25 + w * 1.75);
            return tensor;
        }
    };

    class ShearPlaneField : public BasisField {
    public:
        double theta;
        double shear_angle;

        ShearPlaneField(const Vec2& center, double radius, double theta, double shear_angle, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , shear_angle(shear_angle)
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 dir(std::cos(theta), std::sin(theta));
            const Vec2 normal = detail::perp(dir);
            const double side = ((p - center).dot(normal) >= 0.0) ? 1.0 : -1.0;
            return Tensor2D::fromAngle(theta + side * shear_angle);
        }
    };

    class BoundarySeamField : public BasisField {
    public:
        double theta;
        double seam_band_ratio;

        BoundarySeamField(const Vec2& center, double radius, double theta, double seam_band_ratio, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , seam_band_ratio(std::clamp(seam_band_ratio, 0.05, 0.35))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 rel = p - center;
            const double dist = std::max(1e-6, rel.length());
            const double rim_start = radius * (1.0 - seam_band_ratio);
            const double seam_t = detail::smoothstep(rim_start, radius, dist);
            const double tangent = std::atan2(rel.y, rel.x) + std::numbers::pi * 0.5;
            const double angle = detail::lerp_angle_pi(theta, tangent, seam_t);
            return Tensor2D::fromAngle(angle);
        }
    };

    class CenterVoidOverrideField : public BasisField {
    public:
        double theta;
        double void_ratio;

        CenterVoidOverrideField(const Vec2& center, double radius, double theta, double void_ratio, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , void_ratio(std::clamp(void_ratio, 0.05, 0.8))
        {}

        [[nodiscard]] double getWeight(const Vec2& p) const override {
            const double safe_radius = std::max(1e-6, radius * void_ratio);
            const double dist = p.distanceTo(center);
            if (dist >= safe_radius) {
                return 0.0;
            }
            const double t = 1.0 - (dist / safe_radius);
            return t * t;
        }

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 rel = p - center;
            const double tangent = std::atan2(rel.y, rel.x) + std::numbers::pi * 0.5;
            const double angle = detail::lerp_angle_pi(theta, tangent, 0.8);
            return Tensor2D::fromAngle(angle);
        }
    };

    class NoisePatchField : public BasisField {
    public:
        double theta;
        double frequency;
        double angle_strength;

        NoisePatchField(const Vec2& center, double radius, double theta, double frequency, double angle_strength, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , frequency(std::max(1e-5, frequency))
            , angle_strength(angle_strength)
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const double n =
                std::sin((p.x + center.y) * frequency) +
                std::cos((p.y - center.x) * frequency * 1.31) +
                0.5 * std::sin((p.x + p.y) * frequency * 0.71);
            return Tensor2D::fromAngle(theta + angle_strength * (n / 2.5));
        }
    };

    class ParallelLinearFieldBundle : public BasisField {
    public:
        double theta;
        double spacing;
        int lanes;

        ParallelLinearFieldBundle(const Vec2& center, double radius, double theta, double spacing, int lanes, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , spacing(std::max(8.0, spacing))
            , lanes(std::clamp(lanes, 2, 8))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 dir(std::cos(theta), std::sin(theta));
            const Vec2 normal = detail::perp(dir);
            const double line_coord = (p - center).dot(normal);
            const double lane_id = std::round(line_coord / spacing);
            const double clamped_lane = std::clamp(lane_id, -static_cast<double>(lanes), static_cast<double>(lanes));
            const double local = std::abs(line_coord - clamped_lane * spacing);
            const double lane_weight = 1.0 - std::clamp(local / (spacing * 0.5), 0.0, 1.0);

            Tensor2D tensor = Tensor2D::fromAngle(theta);
            tensor.scale(0.2 + lane_weight * 1.8);
            return tensor;
        }
    };

    class PeriodicRungField : public BasisField {
    public:
        double theta;
        double spacing;
        double rung_width_ratio;

        PeriodicRungField(const Vec2& center, double radius, double theta, double spacing, double rung_width_ratio, double decay)
            : BasisField(center, radius, decay)
            , theta(theta)
            , spacing(std::max(8.0, spacing))
            , rung_width_ratio(std::clamp(rung_width_ratio, 0.05, 0.45))
        {}

        [[nodiscard]] Tensor2D sample(const Vec2& p) const override {
            const Vec2 dir(std::cos(theta), std::sin(theta));
            const double axial = (p - center).dot(dir);
            const double coord = axial / spacing;
            const double line = detail::nearest_line_distance(coord);
            const double threshold = 0.5 * rung_width_ratio;
            const double rung = 1.0 - detail::smoothstep(threshold, threshold + 0.08, line);
            const double angle = detail::lerp_angle_pi(theta, theta + std::numbers::pi * 0.5, rung);
            return Tensor2D::fromAngle(angle);
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
