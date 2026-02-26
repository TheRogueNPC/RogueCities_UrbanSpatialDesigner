#include "RogueCity/Core/Data/TensorTypes.hpp"

namespace RogueCity::Core {

Tensor2D& Tensor2D::add(const Tensor2D& other, [[maybe_unused]] bool smooth) {
    r += other.r;
    m0 += other.m0;
    m1 += other.m1;
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
