#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationContext.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CancellationToken;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::GenerationContext;

    CityGenerator::Config cfg{};
    cfg.width = 2000;
    cfg.height = 2000;
    cfg.cell_size = 10.0;
    cfg.num_seeds = 40;
    cfg.seed = 123;

    std::vector<CityGenerator::AxiomInput> axioms;
    for (int i = 0; i < 8; ++i) {
        CityGenerator::AxiomInput ax{};
        ax.type = CityGenerator::AxiomInput::Type::Grid;
        ax.position = Vec2(300.0 + i * 200.0, 300.0 + i * 150.0);
        ax.radius = 260.0;
        ax.decay = 2.0;
        axioms.push_back(ax);
    }

    CityGenerator generator;

    auto token = std::make_shared<CancellationToken>();
    GenerationContext cancelled_ctx{};
    cancelled_ctx.cancellation = token;
    token->Cancel();
    const auto cancelled_output = generator.GenerateWithContext(axioms, cfg, &cancelled_ctx);
    assert(!cancelled_output.plan_approved || cancelled_output.roads.size() == 0);

    token->Reset();
    GenerationContext limited_ctx{};
    limited_ctx.cancellation = token;
    limited_ctx.max_iterations = 1u;
    const auto limited_output = generator.GenerateWithContext(axioms, cfg, &limited_ctx);
    assert(!limited_output.plan_approved || limited_ctx.ShouldAbort());

    token->Reset();
    GenerationContext healthy_ctx{};
    healthy_ctx.cancellation = token;
    healthy_ctx.max_iterations = 500000u;
    const auto healthy_output = generator.GenerateWithContext(axioms, cfg, &healthy_ctx);
    assert(healthy_output.tensor_field.getWidth() > 0);

    return 0;
}
