#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"

#include <cassert>
#include <vector>

int main() {
    using RogueCity::Core::Editor::GlobalState;
    using RogueCity::Core::Simulation::SimulationConfig;
    using RogueCity::Core::Simulation::SimulationPipeline;

    SimulationConfig cfg{};
    cfg.fixed_timestep = 0.1f;
    cfg.max_substeps = 4u;
    cfg.deterministic_mode = true;

    GlobalState gs{};
    SimulationPipeline pipeline(cfg);
    assert(pipeline.config().deterministic_mode);

    auto r0 = pipeline.ExecuteStep(gs, 0.05f);
    assert(r0.success);
    assert(r0.executed_substeps == 0u);
    assert(gs.frame_counter == 0u);

    auto r1 = pipeline.ExecuteStep(gs, 0.05f);
    assert(r1.success);
    assert(r1.executed_substeps == 1u);
    assert(gs.frame_counter == 1u);

    auto r2 = pipeline.ExecuteStep(gs, 0.4f);
    assert(r2.success);
    assert(r2.executed_substeps == 4u);
    assert(gs.frame_counter == 5u);

    pipeline.Reset();
    auto r3 = pipeline.ExecuteStep(gs, cfg.fixed_timestep);
    assert(r3.success);
    assert(r3.executed_substeps == 1u);

    GlobalState gs_a{};
    GlobalState gs_b{};
    SimulationPipeline pa(cfg);
    SimulationPipeline pb(cfg);
    const std::vector<float> sequence{ 0.03f, 0.07f, 0.1f, 0.2f, 0.11f, 0.39f };

    for (const float dt : sequence) {
        const auto a = pa.ExecuteStep(gs_a, dt);
        const auto b = pb.ExecuteStep(gs_b, dt);
        assert(a.success && b.success);
        assert(a.executed_substeps == b.executed_substeps);
        assert(gs_a.frame_counter == gs_b.frame_counter);
    }

    return 0;
}
