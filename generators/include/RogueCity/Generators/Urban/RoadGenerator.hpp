#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class RoadGenerator {
    public:
        struct Config {
            StreamlineTracer::Params tracing{};
        };

        [[nodiscard]] static fva::Container<Core::Road> generate(
            const std::vector<Core::Vec2>& seeds,
            const TensorFieldGenerator& field,
            const Config& config = Config{});
    };

} // namespace RogueCity::Generators::Urban

