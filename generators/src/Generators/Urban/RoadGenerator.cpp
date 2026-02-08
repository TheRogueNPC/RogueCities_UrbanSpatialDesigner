#include "RogueCity/Generators/Urban/RoadGenerator.hpp"

namespace RogueCity::Generators::Urban {

    fva::Container<Core::Road> RoadGenerator::generate(
        const std::vector<Core::Vec2>& seeds,
        const TensorFieldGenerator& field,
        const Config& config) {
        StreamlineTracer tracer;
        return tracer.traceNetwork(seeds, field, config.tracing);
    }

} // namespace RogueCity::Generators::Urban

