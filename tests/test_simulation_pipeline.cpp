#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"

#include <cassert>
#include <limits>
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

    {
        SimulationConfig non_det_cfg{};
        non_det_cfg.fixed_timestep = 0.1f;
        non_det_cfg.max_substeps = 8u;
        non_det_cfg.deterministic_mode = false;
        GlobalState gs_non_det{};
        SimulationPipeline non_det_pipeline(non_det_cfg);
        const auto non_det = non_det_pipeline.ExecuteStep(gs_non_det, 1.5f);
        assert(non_det.success);
        assert(non_det.executed_substeps == 1u);
        assert(gs_non_det.frame_counter == 1u);
    }

    {
        GlobalState gs_validation{};
        gs_validation.validation_overlay.enabled = true;
        gs_validation.validation_overlay.show_warnings = true;

        RogueCity::Core::LotToken lot{};
        lot.id = 1u;
        lot.centroid = RogueCity::Core::Vec2(0.0, 0.0);
        lot.boundary = {
            RogueCity::Core::Vec2(-1.0, -1.0),
            RogueCity::Core::Vec2(1.0, -1.0),
            RogueCity::Core::Vec2(1.0, 1.0),
            RogueCity::Core::Vec2(-1.0, 1.0)
        };
        gs_validation.lots.add(lot);

        RogueCity::Core::BuildingSite orphan{};
        orphan.id = 10u;
        orphan.lot_id = 99u;
        orphan.position = RogueCity::Core::Vec2(5.0, 5.0);
        gs_validation.buildings.push_back(orphan);

        SimulationPipeline validation_pipeline(cfg);
        const auto validation_result = validation_pipeline.ExecuteStep(gs_validation, cfg.fixed_timestep);
        assert(validation_result.success);
        assert(!gs_validation.validation_overlay.errors.empty());
    }

    {
        GlobalState gs_invalid{};
        RogueCity::Core::Road bad_road{};
        bad_road.id = 1u;
        bad_road.points = {
            RogueCity::Core::Vec2(std::numeric_limits<double>::quiet_NaN(), 0.0),
            RogueCity::Core::Vec2(10.0, 0.0)
        };
        gs_invalid.roads.add(bad_road);

        SimulationPipeline failing_pipeline(cfg);
        const auto failed = failing_pipeline.ExecuteStep(gs_invalid, cfg.fixed_timestep);
        assert(!failed.success);
        assert(!failed.error_message.empty());
    }

    return 0;
}
