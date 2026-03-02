#include "RogueCity/Core/Data/TensorTypes.hpp"
#include <algorithm>

namespace RogueCity::Core {

Tensor2D& Tensor2D::add(const Tensor2D& other, bool smooth) {
    const double base_m0 = m0;
    const double base_m1 = m1;
    const double sum_m0 = base_m0 + other.m0;
    const double sum_m1 = base_m1 + other.m1;

    r += other.r;

    if (!smooth) {
        m0 = sum_m0;
        m1 = sum_m1;
        theta_dirty = true;
        return *this;
    }

    const double base_len = std::hypot(base_m0, base_m1);
    const double other_len = std::hypot(other.m0, other.m1);
    const double sum_len = std::hypot(sum_m0, sum_m1);
    constexpr double kEpsilon = 1e-12;
    if (base_len <= kEpsilon || other_len <= kEpsilon) {
        m0 = sum_m0;
        m1 = sum_m1;
        theta_dirty = true;
        return *this;
    }

    const double dot = std::clamp(
        ((base_m0 * other.m0) + (base_m1 * other.m1)) / (base_len * other_len),
        -1.0,
        1.0);

    if (sum_len <= kEpsilon || dot < -0.90) {
        // Near-opposing tensors can cancel into unstable direction flips.
        // Preserve part of the dominant direction while still blending both inputs.
        const bool other_dominant = other_len > base_len;
        const double dominant_m0 = other_dominant ? other.m0 : base_m0;
        const double dominant_m1 = other_dominant ? other.m1 : base_m1;
        constexpr double kDominantPreserve = 0.55;
        m0 = (1.0 - kDominantPreserve) * sum_m0 + (kDominantPreserve * dominant_m0);
        m1 = (1.0 - kDominantPreserve) * sum_m1 + (kDominantPreserve * dominant_m1);
    } else {
        // Mild direction-preserving smoothing keeps weighted-sum semantics
        // while reducing sharp angular jitter in successive blends.
        const double dir_x = sum_m0 / sum_len;
        const double dir_y = sum_m1 / sum_len;
        const double stabilization = (1.0 - dot) * 0.10 * std::min(base_len, other_len);
        const double stabilized_len = sum_len + stabilization;
        m0 = dir_x * stabilized_len;
        m1 = dir_y * stabilized_len;
    }

    theta_dirty = true;
    return *this;
}

Tensor2D& Tensor2D::rotate(double theta_radians) {
    const double c = std::cos(2.0 * theta_radians);
    const double s = std::sin(2.0 * theta_radians);
    const double new_m0 = (m0 * c) - (m1 * s);
    const double new_m1 = (m0 * s) + (m1 * c);
    m0 = new_m0;
    m1 = new_m1;
    theta_dirty = true;
    return *this;
}

} // namespace RogueCity::Core
