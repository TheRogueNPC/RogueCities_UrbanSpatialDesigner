#include "RogueCity/Core/Data/TensorTypes.hpp"

#include <cmath>
#include <iostream>

#define RC_EXPECT(expr)          \
    do {                         \
        if (!(expr)) {           \
            std::cerr << "RC_EXPECT failed at line " << __LINE__ << ": " << #expr << '\n'; \
            return 1;            \
        }                        \
    } while (false)

int main() {
    using RogueCity::Core::Tensor2D;

    {
        Tensor2D a = Tensor2D::fromAngle(0.0);
        Tensor2D b = Tensor2D::fromAngle(0.0);
        a.add(b, false);
        RC_EXPECT(std::abs(a.m0 - 2.0) < 1e-6);
        RC_EXPECT(std::abs(a.m1) < 1e-6);
    }

    {
        Tensor2D a = Tensor2D::fromAngle(0.0);
        Tensor2D b = Tensor2D::fromAngle(0.0);
        a.add(b, true);
        RC_EXPECT(std::abs(a.m0 - 2.0) < 1e-6);
        RC_EXPECT(std::abs(a.m1) < 1e-6);
    }

    {
        Tensor2D unsmoothed = Tensor2D::fromAngle(0.0);
        unsmoothed.add(Tensor2D::fromAngle(3.14159265358979323846 * 0.5), false);
        const double unsmoothed_len = std::hypot(unsmoothed.m0, unsmoothed.m1);
        RC_EXPECT(unsmoothed_len < 1e-4);

        Tensor2D smoothed = Tensor2D::fromAngle(0.0);
        smoothed.add(Tensor2D::fromAngle(3.14159265358979323846 * 0.5), true);
        const double smoothed_len = std::hypot(smoothed.m0, smoothed.m1);
        RC_EXPECT(smoothed_len > 0.1);
    }

    {
        Tensor2D jitter_a = Tensor2D::fromAngle(0.2);
        Tensor2D jitter_b = Tensor2D::fromAngle(0.25);
        Tensor2D smooth = jitter_a;
        Tensor2D plain = jitter_a;
        smooth.add(jitter_b, true);
        plain.add(jitter_b, false);
        RC_EXPECT(std::isfinite(smooth.angle()));
        RC_EXPECT(std::isfinite(plain.angle()));
    }

    return 0;
}

#undef RC_EXPECT
