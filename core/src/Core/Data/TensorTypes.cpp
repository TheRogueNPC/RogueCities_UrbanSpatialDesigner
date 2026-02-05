#include "RogueCity/Core/Data/TensorTypes.hpp"
#include <cmath>

namespace RogueCity::Core {

    Tensor2D& Tensor2D::add(const Tensor2D& other, [[maybe_unsed]] bool smooth) {
        (void)smooth;  // Unused in core, used in generators layer [[maybe_unsed]] silences warning

        // Simplified addition (full smooth blending in generators layer)
        r += other.r;
        m0 += other.m0;
        m1 += other.m1;
        theta_dirty = true;
        return *this;
    }


    Tensor2D& Tensor2D::rotate(double theta_radians) {
        double cos2t = std::cos(2.0 * theta_radians);
        double sin2t = std::sin(2.0 * theta_radians);
        double new_m0 = m0 * cos2t - m1 * sin2t;
        double new_m1 = m0 * sin2t + m1 * cos2t;
        m0 = new_m0;
        m1 = new_m1;
        theta_dirty = true;
        return *this;
    }

} // namespace RogueCity::Core
