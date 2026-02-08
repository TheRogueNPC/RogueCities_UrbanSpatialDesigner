#include "RogueCity/Generators/Urban/Integrator.hpp"

namespace RogueCity::Generators::Urban {

    Core::Vec2 Integrator::rk4Step(
        const Core::Vec2& pos,
        const TensorFieldGenerator& field,
        bool use_major,
        double step_size) {
        Core::Tensor2D t1 = field.sampleTensor(pos);
        Core::Vec2 k1 = use_major ? t1.majorEigenvector() : t1.minorEigenvector();

        Core::Tensor2D t2 = field.sampleTensor(pos + k1 * (step_size * 0.5));
        Core::Vec2 k2 = use_major ? t2.majorEigenvector() : t2.minorEigenvector();

        Core::Tensor2D t3 = field.sampleTensor(pos + k2 * (step_size * 0.5));
        Core::Vec2 k3 = use_major ? t3.majorEigenvector() : t3.minorEigenvector();

        Core::Tensor2D t4 = field.sampleTensor(pos + k3 * step_size);
        Core::Vec2 k4 = use_major ? t4.majorEigenvector() : t4.minorEigenvector();

        Core::Vec2 velocity = (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (1.0 / 6.0);
        velocity.normalize();
        return pos + velocity * step_size;
    }

} // namespace RogueCity::Generators::Urban

