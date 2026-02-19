#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationContext.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CancellationToken;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::GenerationContext;
    using RogueCity::Generators::GenerationStage;
    using RogueCity::Generators::MarkStageDirty;
    using RogueCity::Generators::StageMask;

    CityGenerator::Config cfg{};
    cfg.width = 2200;
    cfg.height = 2200;
    cfg.cell_size = 10.0;
    cfg.seed = 42;
    cfg.num_seeds = 26;

    std::vector<CityGenerator::AxiomInput> axioms;
    CityGenerator::AxiomInput ax{};
    ax.type = CityGenerator::AxiomInput::Type::Radial;
    ax.position = Vec2(1100.0, 1100.0);
    ax.radius = 500.0;
    axioms.push_back(ax);

    const std::vector<GenerationStage> stages{
        GenerationStage::Terrain,
        GenerationStage::TensorField,
        GenerationStage::Roads,
        GenerationStage::Districts,
        GenerationStage::Blocks,
        GenerationStage::Lots,
        GenerationStage::Buildings,
        GenerationStage::Validation
    };

    for (const auto stage : stages) {
        CityGenerator generator{};
        StageMask dirty{};
        MarkStageDirty(dirty, stage);

        GenerationContext context{};
        context.cancellation = std::make_shared<CancellationToken>();
        context.max_iterations = 1u;

        const auto output = generator.RegenerateIncremental(axioms, cfg, dirty, nullptr, &context);
        const bool produced_partial =
            output.roads.size() > 0 || !output.districts.empty() || !output.lots.empty();
        assert(context.ShouldAbort() || !output.plan_approved || produced_partial);
    }

    // Rapid cancel/restart stress cycles (1000 total cancel operations).
    // Keep full restarts periodic so the test remains practical in CI.
    CityGenerator generator{};
    for (int i = 0; i < 1000; ++i) {
        auto token = std::make_shared<CancellationToken>();
        GenerationContext cancelled{};
        cancelled.cancellation = token;
        token->Cancel();
        (void)generator.GenerateWithContext(axioms, cfg, &cancelled);

        if ((i % 100) == 0) {
            token->Reset();
            GenerationContext restarted{};
            restarted.cancellation = token;
            restarted.max_iterations = 200000u;
            const auto output = generator.GenerateWithContext(axioms, cfg, &restarted);
            assert(output.tensor_field.getWidth() > 0);
        }
    }

    return 0;
}
