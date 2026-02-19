#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <chrono>
#include <vector>

int main() {
    using Clock = std::chrono::steady_clock;
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::GenerationStage;
    using RogueCity::Generators::MarkStageDirty;
    using RogueCity::Generators::StageMask;

    CityGenerator::Config cfg{};
    cfg.width = 2200;
    cfg.height = 2200;
    cfg.cell_size = 10.0;
    cfg.seed = 77;
    cfg.num_seeds = 28;

    std::vector<CityGenerator::AxiomInput> axioms;
    CityGenerator::AxiomInput radial{};
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = Vec2(1100.0, 1100.0);
    radial.radius = 460.0;
    axioms.push_back(radial);

    CityGenerator generator;

    CityGenerator::StageOptions full{};
    full.stages_to_run.set();
    full.use_cache = false;

    const auto t0 = Clock::now();
    const auto baseline = generator.GenerateStages(axioms, cfg, full);
    const auto t1 = Clock::now();
    assert(baseline.tensor_field.getWidth() > 0);

    StageMask validation_only{};
    MarkStageDirty(validation_only, GenerationStage::Validation);
    const auto t2 = Clock::now();
    const auto cached = generator.RegenerateIncremental(axioms, cfg, validation_only);
    const auto t3 = Clock::now();
    assert(cached.tensor_field.getWidth() > 0);

    const auto full_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const auto cached_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

    // Keep this tolerant to CI variance while still catching pathological regressions.
    if (full_ms > 0) {
        assert(cached_ms <= full_ms * 3 + 10);
    }

    return 0;
}
