#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"

namespace RogueCity::Generators::Urban {

    class Integrator {
    public:
        static Core::Vec2 rk4Step(
            const Core::Vec2& pos,
            const TensorFieldGenerator& field,
            bool use_major,
            double step_size);
    };

} // namespace RogueCity::Generators::Urban

