#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Generators/Roads/RoadNoder.hpp"
#include "RogueCity/Generators/Roads/GraphSimplify.hpp"
#include "RogueCity/Generators/Roads/FlowAndControl.hpp"
#include "RogueCity/Generators/Roads/Verticality.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Urban {

    class RoadGenerator {
    public:
        struct Config {
            StreamlineTracer::Params tracing;
            Roads::NoderConfig noder{};
            Roads::SimplifyConfig simplify{};
            Roads::FlowControlConfig flow_control{};
            Roads::VerticalityConfig verticality{};
            uint32_t centrality_samples = 64u;
            bool enable_verticality = true;
        };

        [[nodiscard]] fva::Container<Core::Road> generate(
            const std::vector<Core::Vec2>& seeds,
            const TensorFieldGenerator& field);

        [[nodiscard]] fva::Container<Core::Road> generate(
            const std::vector<Core::Vec2>& seeds,
            const TensorFieldGenerator& field,
            const Config& config);
        
        // Access intersection templates generated during last road generation pass
        [[nodiscard]] const std::vector<Core::IntersectionTemplate>& getIntersectionTemplates() const {
            return stored_intersection_templates_;
        }

    private:
        std::vector<Core::IntersectionTemplate> stored_intersection_templates_;
    };

} // namespace RogueCity::Generators::Urban
