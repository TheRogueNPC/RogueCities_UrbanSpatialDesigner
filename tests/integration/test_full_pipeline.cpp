#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/App/Integration/CityOutputApplier.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/StableIDRegistry.hpp"
#include "RogueCity/Core/Validation/DeterminismHash.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationContext.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using RogueCity::Core::Editor::GetStableIDRegistry;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::VpEntityKind;
using RogueCity::Core::Validation::ComputeDeterminismHash;
using RogueCity::Generators::CancellationToken;
using RogueCity::Generators::CityGenerator;
using RogueCity::Generators::GenerationContext;
using RogueCity::Generators::GenerationStage;
using RogueCity::Generators::MarkStageDirty;
using RogueCity::Generators::StageMask;

uint64_t MakeViewportKey(VpEntityKind kind, uint32_t id) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(kind)) << 32ull) | static_cast<uint64_t>(id);
}

std::unordered_map<uint64_t, uint64_t> CollectStableIDs(const std::vector<RogueCity::Core::Editor::VpProbeData>& probes) {
    std::unordered_map<uint64_t, uint64_t> stable_ids;
    for (const auto& probe : probes) {
        if (probe.stable_id == 0u) {
            continue;
        }
        stable_ids[MakeViewportKey(probe.kind, probe.id)] = probe.stable_id;
    }
    return stable_ids;
}

CityGenerator::Config BuildConfig(uint32_t seed) {
    CityGenerator::Config cfg{};
    cfg.width = 2000;
    cfg.height = 2000;
    cfg.cell_size = 10.0;
    cfg.seed = seed;
    cfg.num_seeds = 28;
    cfg.max_districts = 320;
    cfg.max_lots = 24000;
    cfg.max_buildings = 48000;
    cfg.enable_world_constraints = true;
    cfg.adaptive_tracing = true;
    cfg.enforce_road_separation = true;
    cfg.min_trace_step_size = 2.0;
    cfg.max_trace_step_size = 10.0;
    cfg.trace_curvature_gain = 1.6;
    return cfg;
}

std::vector<CityGenerator::AxiomInput> BuildAxioms(int width, int height) {
    std::vector<CityGenerator::AxiomInput> axioms;
    const double cx = static_cast<double>(width) * 0.5;
    const double cy = static_cast<double>(height) * 0.5;

    CityGenerator::AxiomInput radial{};
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = { cx - 220.0, cy - 120.0 };
    radial.radius = 360.0;
    radial.radial_spokes = 9;
    radial.decay = 2.1;
    axioms.push_back(radial);

    CityGenerator::AxiomInput grid{};
    grid.type = CityGenerator::AxiomInput::Type::Grid;
    grid.position = { cx + 180.0, cy + 60.0 };
    grid.radius = 420.0;
    grid.theta = 0.3;
    grid.decay = 1.9;
    axioms.push_back(grid);

    CityGenerator::AxiomInput stem{};
    stem.type = CityGenerator::AxiomInput::Type::Stem;
    stem.position = { cx + 20.0, cy - 260.0 };
    stem.radius = 300.0;
    stem.theta = 0.4;
    stem.stem_branch_angle = 0.8f;
    stem.decay = 2.0;
    axioms.push_back(stem);

    return axioms;
}

std::vector<CityGenerator::AxiomInput> BuildComplexAxioms(int width, int height, int count) {
    std::vector<CityGenerator::AxiomInput> axioms;
    axioms.reserve(static_cast<size_t>(count));

    const double cx = static_cast<double>(width) * 0.5;
    const double cy = static_cast<double>(height) * 0.5;
    for (int i = 0; i < count; ++i) {
        CityGenerator::AxiomInput ax{};
        ax.type = static_cast<CityGenerator::AxiomInput::Type>(
            i % static_cast<int>(CityGenerator::AxiomInput::Type::COUNT));
        ax.position = {
            cx + static_cast<double>((i % 8) - 4) * 85.0,
            cy + static_cast<double>((i % 7) - 3) * 92.0
        };
        ax.radius = 200.0 + static_cast<double>((i % 5) * 35);
        ax.theta = 0.1 * static_cast<double>(i % 9);
        ax.decay = 1.8 + 0.05 * static_cast<double>(i % 6);
        ax.radial_spokes = 5 + (i % 10);
        ax.loose_grid_jitter = 0.08f + 0.01f * static_cast<float>(i % 7);
        ax.suburban_loop_strength = 0.35f + 0.03f * static_cast<float>(i % 10);
        ax.stem_branch_angle = 0.4f + 0.05f * static_cast<float>(i % 6);
        ax.superblock_block_size = 180.0f + 10.0f * static_cast<float>(i % 8);
        axioms.push_back(ax);
    }

    return axioms;
}

void CopyStateEntitiesForReload(const GlobalState& src, GlobalState& dst) {
    dst.roads.clear();
    for (const auto& road : src.roads) {
        dst.roads.add(road);
    }

    dst.districts.clear();
    for (const auto& district : src.districts) {
        dst.districts.add(district);
    }

    dst.blocks.clear();
    for (const auto& block : src.blocks) {
        dst.blocks.add(block);
    }

    dst.lots.clear();
    for (const auto& lot : src.lots) {
        dst.lots.add(lot);
    }

    dst.buildings.clear();
    for (const auto& building : src.buildings) {
        dst.buildings.push_back(building);
    }

    dst.waterbodies.clear();
    for (const auto& water : src.waterbodies) {
        dst.waterbodies.add(water);
    }
}

} // namespace

int main() {
    auto& registry = GetStableIDRegistry();
    registry.Clear();

    const auto cfg = BuildConfig(424242u);
    const auto axioms = BuildAxioms(cfg.width, cfg.height);

    // End-to-end generation with output validation and deterministic hash stability.
    CityGenerator full_generator{};
    GlobalState full_state{};
    const auto full_output = full_generator.generate(axioms, cfg, &full_state);
    assert(full_output.tensor_field.getWidth() > 0);
    assert(full_output.tensor_field.getHeight() > 0);
    assert(full_output.world_constraints.isValid());

    RogueCity::App::ApplyCityOutputToGlobalState(full_output, full_state);
    assert(full_state.roads.size() == full_output.roads.size());
    assert(full_state.districts.size() == full_output.districts.size());
    assert(full_state.lots.size() == full_output.lots.size());
    assert(full_state.buildings.size() == full_output.buildings.size());
    assert(full_state.HasTextureSpace());
    assert(!full_state.viewport_index.empty());

    const auto hash_a = ComputeDeterminismHash(full_state);
    const auto hash_b = ComputeDeterminismHash(full_state);
    assert(hash_a == hash_b);

    // Multi-stage incremental generation parity against full generation.
    StageMask dirty{};
    MarkStageDirty(dirty, GenerationStage::TensorField);
    MarkStageDirty(dirty, GenerationStage::Roads);

    CityGenerator incremental_generator{};
    (void)incremental_generator.generate(axioms, cfg);
    const auto incremental_output = incremental_generator.RegenerateIncremental(axioms, cfg, dirty);

    CityGenerator reference_generator{};
    const auto reference_output = reference_generator.generate(axioms, cfg);

    assert(incremental_output.roads.size() == reference_output.roads.size());
    assert(incremental_output.districts.size() == reference_output.districts.size());
    assert(incremental_output.blocks.size() == reference_output.blocks.size());
    assert(incremental_output.lots.size() == reference_output.lots.size());
    assert(incremental_output.buildings.size() == reference_output.buildings.size());

    // Cancellation during complex generation should abort and avoid fully approved output.
    const auto complex_axioms = BuildComplexAxioms(cfg.width, cfg.height, 48);
    GenerationContext cancelled_context{};
    cancelled_context.cancellation = std::make_shared<CancellationToken>();
    cancelled_context.max_iterations = 48u;

    CityGenerator cancellation_generator{};
    const auto cancelled_output =
        cancellation_generator.GenerateWithContext(complex_axioms, cfg, &cancelled_context);
    assert(cancelled_context.ShouldAbort());
    assert(!cancelled_output.plan_approved || cancelled_output.buildings.empty());

    // Save/load stable IDs through registry serialization + viewport rebuild.
    registry.Clear();

    GlobalState save_state{};
    RogueCity::App::ApplyCityOutputToGlobalState(reference_output, save_state);
    assert(!save_state.viewport_index.empty());
    const auto stable_before = CollectStableIDs(save_state.viewport_index);
    assert(!stable_before.empty());

    const std::string serialized_registry = registry.Serialize();
    assert(!serialized_registry.empty());

    GlobalState loaded_state{};
    CopyStateEntitiesForReload(save_state, loaded_state);

    registry.Clear();
    assert(registry.Deserialize(serialized_registry));
    RogueCity::App::ViewportIndexBuilder::Build(loaded_state);
    const auto stable_after = CollectStableIDs(loaded_state.viewport_index);

    for (const auto& [entity_key, stable_id] : stable_before) {
        const auto it = stable_after.find(entity_key);
        assert(it != stable_after.end());
        assert(it->second == stable_id);
    }

    registry.Clear();
    return 0;
}
