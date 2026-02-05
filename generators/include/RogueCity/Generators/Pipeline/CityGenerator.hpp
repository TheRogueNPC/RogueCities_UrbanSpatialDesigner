#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Generators/Roads/StreamlineTracer.hpp"
#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include <vector>

namespace RogueCity::Generators {

    using namespace Core;

    /// Orchestrates full city generation pipeline
    class CityGenerator {
    public:
        /// Generation configuration
        struct Config {
            int width{ 2000 };           // City width (meters)
            int height{ 2000 };          // City height (meters)
            double cell_size{ 10.0 };    // Tensor field resolution (meters)
            uint32_t seed{ 12345 };      // RNG seed
            int num_seeds{ 20 };         // Number of streamline seeds
        };

        /// Axiom input (user-placed planning intent)
        struct AxiomInput {
            enum class Type { Radial, Grid, Delta, GridCorrective };
            
            Type type;
            Vec2 position;
            double radius;
            double theta{ 0.0 };  // For Grid/GridCorrective
            DeltaTerminal terminal{ DeltaTerminal::North };  // For Delta
            double decay{ 2.0 };
        };

        /// Generated city output
        struct CityOutput {
            std::vector<Road> roads;
            std::vector<District> districts;
            std::vector<LotToken> lots;
            TensorFieldGenerator tensor_field;
        };

        CityGenerator() = default;

        /// Generate city from axioms
        [[nodiscard]] CityOutput generate(
            const std::vector<AxiomInput>& axioms,
            const Config& config = Config{}
        );

    private:
        Config config_;
        RNG rng_{ 12345 };

        /// Pipeline stages
        TensorFieldGenerator generateTensorField(const std::vector<AxiomInput>& axioms);
        std::vector<Vec2> generateSeeds();
        std::vector<Road> traceRoads(const TensorFieldGenerator& field, const std::vector<Vec2>& seeds);
        std::vector<District> classifyDistricts(const std::vector<Road>& roads);
        std::vector<LotToken> generateLots(const std::vector<Road>& roads, const std::vector<District>& districts);
    };

} // namespace RogueCity::Generators
