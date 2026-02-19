#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationContext.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using RogueCity::Generators::CancellationToken;
using RogueCity::Generators::CityGenerator;
using RogueCity::Generators::GenerationContext;
using RogueCity::Generators::GenerationStage;
using RogueCity::Generators::MarkStageDirty;
using RogueCity::Generators::StageMask;

constexpr long long kFullGenerationBudgetMs = 5000;
constexpr long long kIncrementalRoadsBudgetMs = 1000;
constexpr long long kCancelResponseBudgetMs = 100;

void EmitMetric(const char* name, long long value_ms) {
    std::cout << "PERF_METRIC " << name << " " << value_ms << "\n";
}

CityGenerator::Config BuildConfig(uint32_t seed) {
    CityGenerator::Config cfg{};
    cfg.width = 2000;
    cfg.height = 2000;
    cfg.cell_size = 10.0;
    cfg.seed = seed;
    cfg.num_seeds = 40;
    cfg.max_districts = 360;
    cfg.max_lots = 30000;
    cfg.max_buildings = 52000;
    cfg.enable_world_constraints = true;
    cfg.adaptive_tracing = true;
    cfg.enforce_road_separation = true;
    return cfg;
}

std::vector<CityGenerator::AxiomInput> BuildAxioms(int width, int height, int count) {
    std::vector<CityGenerator::AxiomInput> axioms;
    axioms.reserve(static_cast<size_t>(count));

    const double cx = static_cast<double>(width) * 0.5;
    const double cy = static_cast<double>(height) * 0.5;
    for (int i = 0; i < count; ++i) {
        CityGenerator::AxiomInput ax{};
        ax.type = static_cast<CityGenerator::AxiomInput::Type>(
            i % static_cast<int>(CityGenerator::AxiomInput::Type::COUNT));
        ax.position = {
            cx + static_cast<double>((i % 10) - 5) * 70.0,
            cy + static_cast<double>((i % 9) - 4) * 75.0
        };
        ax.radius = 210.0 + static_cast<double>((i % 6) * 30);
        ax.theta = 0.08 * static_cast<double>(i % 11);
        ax.decay = 1.8 + 0.05 * static_cast<double>(i % 7);
        ax.radial_spokes = 6 + (i % 8);
        ax.loose_grid_jitter = 0.10f + 0.02f * static_cast<float>(i % 4);
        ax.suburban_loop_strength = 0.4f + 0.03f * static_cast<float>(i % 6);
        ax.stem_branch_angle = 0.45f + 0.04f * static_cast<float>(i % 6);
        ax.superblock_block_size = 170.0f + 12.0f * static_cast<float>(i % 6);
        axioms.push_back(ax);
    }

    return axioms;
}

long long ToMilliseconds(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
}

} // namespace

int main() {
    const auto cfg = BuildConfig(515151u);
    const auto axioms = BuildAxioms(cfg.width, cfg.height, 18);

    CityGenerator generator{};

    // Full generation latency budget (<5s for 2km^2).
    const auto t0 = Clock::now();
    const auto full_output = generator.generate(axioms, cfg);
    const auto t1 = Clock::now();
    assert(full_output.tensor_field.getWidth() > 0);
    const long long full_generation_ms = ToMilliseconds(t0, t1);
    EmitMetric("full_generation_ms", full_generation_ms);

    // Incremental roads regeneration budget (<1s).
    StageMask dirty{};
    MarkStageDirty(dirty, GenerationStage::Roads);

    const auto t2 = Clock::now();
    const auto incremental_output = generator.RegenerateIncremental(axioms, cfg, dirty);
    const auto t3 = Clock::now();
    assert(incremental_output.tensor_field.getWidth() > 0);
    const long long incremental_roads_ms = ToMilliseconds(t2, t3);
    EmitMetric("incremental_roads_ms", incremental_roads_ms);

    // Cancellation response budget (<100ms) once cancellation is signaled.
    const auto heavy_axioms = BuildAxioms(cfg.width, cfg.height, 64);
    auto cancel_cfg = cfg;
    cancel_cfg.enable_world_constraints = false;
    auto cancel_token = std::make_shared<CancellationToken>();
    cancel_token->Cancel();

    GenerationContext cancelled_context{};
    cancelled_context.cancellation = cancel_token;
    cancelled_context.max_iterations = 1'000'000u;

    CityGenerator cancelled_generator{};
    const auto t4 = Clock::now();
    const auto cancelled_output =
        cancelled_generator.GenerateWithContext(heavy_axioms, cancel_cfg, &cancelled_context);
    const auto t5 = Clock::now();
    (void)cancelled_output;
    const long long cancel_response_ms = ToMilliseconds(t4, t5);
    EmitMetric("cancel_response_ms", cancel_response_ms);

    assert(full_generation_ms <= kFullGenerationBudgetMs);
    assert(incremental_roads_ms <= kIncrementalRoadsBudgetMs);
    assert(cancel_response_ms <= kCancelResponseBudgetMs);

    return 0;
}
