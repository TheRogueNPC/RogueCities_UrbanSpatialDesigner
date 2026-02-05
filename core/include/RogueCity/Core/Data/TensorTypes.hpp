#pragma once
#define _USE_MATH_DEFINES // Must be before <cmath>

#include "RogueCity/Core/Math/Vec2.hpp"
#include <cmath>

namespace RogueCity::Core {

    /**
     * @brief 2D tensor field element for road network generation
     *
     * Based on "Interactive Procedural Street Modeling" (Chen et al. 2008)
     * and "Tensor Fields for Interactive Procedural Content" (Lechner et al. 2003).
     *
     * Tensors encode directional information (major/minor eigenvectors) used to
     * guide streamline integration for road tracing.
     */
    struct Tensor2D {
        double r{ 0.0 };    ///< Tensor magnitude (anisotropy)
        double m0{ 0.0 };   ///< Tensor component 0 (basis coefficient)
        double m1{ 0.0 };   ///< Tensor component 1 (basis coefficient)

        // Cached angle computation (mutable for lazy evaluation)
        mutable double theta_cache{ 0.0 };
        mutable bool theta_dirty{ true };

        // ===== CONSTRUCTORS =====
        Tensor2D() = default;
        Tensor2D(double r, double m0, double m1)
            : r(r), m0(m0), m1(m1), theta_dirty(true) {
        }

        // ===== FACTORY METHODS =====

        /// Create tensor from angle (radians)
        [[nodiscard]] static Tensor2D fromAngle(double angle_radians) {
            double cos2a = std::cos(2.0 * angle_radians);
            double sin2a = std::sin(2.0 * angle_radians);
            return Tensor2D(1.0, cos2a, sin2a);
        }

        /// Create tensor from direction vector
        [[nodiscard]] static Tensor2D fromVector(const Vec2& v) {
            double angle = std::atan2(v.y, v.x);
            return fromAngle(angle);
        }

        /// Create zero tensor
        [[nodiscard]] static Tensor2D zero() {
            return Tensor2D(0.0, 0.0, 0.0);
        }

        // ===== OPERATIONS =====

        /// Add tensors (with optional smooth blending)
        Tensor2D& add(const Tensor2D& other, bool smooth = false);

        /// Scale tensor magnitude
        Tensor2D& scale(double s) {
            r *= s; m0 *= s; m1 *= s;
            return *this;
        }

        /// Rotate tensor by angle (radians)
        Tensor2D& rotate(double theta_radians);

        // ===== EIGENVECTOR EXTRACTION =====

        /// Get major eigenvector (primary direction)
        [[nodiscard]] Vec2 majorEigenvector() const {
            double t = theta();
            return Vec2(std::cos(t), std::sin(t));
        }

        /// Get minor eigenvector (perpendicular direction)
        [[nodiscard]] Vec2 minorEigenvector() const {
            double t = theta() + 3.14159265358979323846 * 0.5;
            return Vec2(std::cos(t), std::sin(t));
        }

        /// Get principal angle (cached)
        [[nodiscard]] double angle() const { return theta(); }

    private:
        /// Compute angle from tensor components (cached)
        [[nodiscard]] double theta() const {
            if (theta_dirty) {
                theta_cache = 0.5 * std::atan2(m1, m0);
                theta_dirty = false;
            }
            return theta_cache;
        }
    };

} // namespace RogueCity::Core
