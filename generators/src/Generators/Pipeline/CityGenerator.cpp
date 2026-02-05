#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <cmath>

namespace RogueCity::Generators {

    CityGenerator::CityOutput CityGenerator::generate(
        const std::vector<AxiomInput>& axioms,
        const Config& config
    ) {
        config_ = config;
        rng_ = RNG(config_.seed);

        CityOutput output;

        // Stage 1: Generate tensor field from axioms
        output.tensor_field = generateTensorField(axioms);

        // Stage 2: Generate seed points for streamline tracing
        std::vector<Vec2> seeds = generateSeeds();

        // Stage 3: Trace road network
        output.roads = traceRoads(output.tensor_field, seeds);

        // Stage 4: Classify districts (Phase 3)
        // output.districts = classifyDistricts(output.roads);

        // Stage 5: Generate lots (Phase 3)
        // output.lots = generateLots(output.roads, output.districts);

        return output;
    }

    TensorFieldGenerator CityGenerator::generateTensorField(
        const std::vector<AxiomInput>& axioms
    ) {
        TensorFieldGenerator::Config field_config;
        field_config.width = static_cast<int>(config_.width / config_.cell_size);
        field_config.height = static_cast<int>(config_.height / config_.cell_size);
        field_config.cell_size = config_.cell_size;

        TensorFieldGenerator field(field_config);

        // Add basis fields from axioms
        for (const auto& axiom : axioms) {
            switch (axiom.type) {
                case AxiomInput::Type::Radial:
                    field.addRadialField(axiom.position, axiom.radius, axiom.decay);
                    break;

                case AxiomInput::Type::Grid:
                    field.addGridField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;

                case AxiomInput::Type::Delta:
                    field.addDeltaField(axiom.position, axiom.radius, axiom.terminal, axiom.decay);
                    break;

                case AxiomInput::Type::GridCorrective:
                    field.addGridCorrective(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;
            }
        }

        // Generate field
        field.generateField();

        return field;
    }

    std::vector<Vec2> CityGenerator::generateSeeds() {
        std::vector<Vec2> seeds;
        seeds.reserve(config_.num_seeds);

        // Generate random seed points within city bounds
        for (int i = 0; i < config_.num_seeds; ++i) {
            Vec2 seed(
                rng_.uniform(0.0, static_cast<double>(config_.width)),
                rng_.uniform(0.0, static_cast<double>(config_.height))
            );
            seeds.push_back(seed);
        }

        return seeds;
    }

    std::vector<Road> CityGenerator::traceRoads(
        const TensorFieldGenerator& field,
        const std::vector<Vec2>& seeds
    ) {
        StreamlineTracer tracer;
        StreamlineTracer::Params params;
        params.step_size = 5.0;
        params.max_length = 800.0;
        params.bidirectional = true;

        return tracer.traceNetwork(seeds, field, params);
    }

    std::vector<District> CityGenerator::classifyDistricts(
        const std::vector<Road>& roads
    ) {
        // Placeholder for Phase 3 - block extraction + AESP classification
        return std::vector<District>();
    }

    std::vector<LotToken> CityGenerator::generateLots(
        const std::vector<Road>& roads,
        const std::vector<District>& districts
    ) {
        // Placeholder for Phase 3 - lot subdivision
        return std::vector<LotToken>();
    }

} // namespace RogueCity::Generators
