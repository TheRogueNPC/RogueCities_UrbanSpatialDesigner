#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include <vector>

namespace RogueCity::Generators {

    using namespace Core;

    /// Traces streamlines through tensor field using RK4 integration
    class StreamlineTracer {
    public:
        /// Tracing parameters
        struct Params {
            double step_size{ 5.0 };        // Integration step (meters)
            double max_length{ 1000.0 };    // Maximum road length (meters)
            double min_separation{ 10.0 };  // Minimum distance between roads
            int max_iterations{ 500 };      // Safety limit for infinite loops
            bool bidirectional{ true };     // Trace forward AND backward from seed
        };

        StreamlineTracer() = default;

        /// Trace single streamline along major eigenvector
        [[nodiscard]] std::vector<Vec2> traceMajor(
            const Vec2& seed,
            const TensorFieldGenerator& field,
            const Params& params
        );

        [[nodiscard]] std::vector<Vec2> traceMajor(
            const Vec2& seed,
            const TensorFieldGenerator& field
        ) {
            return traceMajor(seed, field, Params{});
        }

        /// Trace single streamline along minor eigenvector
        [[nodiscard]] std::vector<Vec2> traceMinor(
            const Vec2& seed,
            const TensorFieldGenerator& field,
            const Params& params
        );

        [[nodiscard]] std::vector<Vec2> traceMinor(
            const Vec2& seed,
            const TensorFieldGenerator& field
        ) {
            return traceMinor(seed, field, Params{});
        }

        /// Trace full road network from seed points
        [[nodiscard]] fva::Container<Road> traceNetwork(
            const std::vector<Vec2>& seeds,
            const TensorFieldGenerator& field,
            const Params& params
        );

        [[nodiscard]] fva::Container<Road> traceNetwork(
            const std::vector<Vec2>& seeds,
            const TensorFieldGenerator& field
        ) {
            return traceNetwork(seeds, field, Params{});
        }

    private:
        /// RK4 integration step
        [[nodiscard]] Vec2 integrateRK4(
            const Vec2& pos,
            const TensorFieldGenerator& field,
            bool use_major,
            double dt
        );

        /// Trace in single direction from seed
        [[nodiscard]] std::vector<Vec2> traceDirection(
            const Vec2& seed,
            const TensorFieldGenerator& field,
            bool use_major,
            const Params& params,
            bool forward
        );

        /// Check if point is too close to existing polyline
        [[nodiscard]] bool tooCloseToExisting(
            const Vec2& p,
            const std::vector<Vec2>& existing,
            double min_dist
        );
    };

} // namespace RogueCity::Generators
