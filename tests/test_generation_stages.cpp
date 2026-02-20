#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::GenerationStage;
    using RogueCity::Generators::MarkStageDirty;
    using RogueCity::Generators::StageMask;

    CityGenerator::Config cfg{};
    cfg.width = 1800;
    cfg.height = 1800;
    cfg.cell_size = 10.0;
    cfg.seed = 42;
    cfg.num_seeds = 20;

    std::vector<CityGenerator::AxiomInput> axioms;
    CityGenerator::AxiomInput radial{};
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = Vec2(900.0, 900.0);
    radial.radius = 420.0;
    axioms.push_back(radial);

    CityGenerator generator;

    CityGenerator::StageOptions full{};
    full.stages_to_run.set();
    full.use_cache = false;
    const auto staged_full = generator.GenerateStages(axioms, cfg, full);
    assert(staged_full.tensor_field.getWidth() > 0);

    const auto legacy_full = generator.generate(axioms, cfg);
    assert(legacy_full.roads.size() == staged_full.roads.size());
    assert(legacy_full.districts.size() == staged_full.districts.size());
    assert(legacy_full.lots.size() == staged_full.lots.size());
    assert(legacy_full.buildings.size() == staged_full.buildings.size());

    StageMask dirty{};
    MarkStageDirty(dirty, GenerationStage::TensorField);
    MarkStageDirty(dirty, GenerationStage::Roads);
    const auto incremental = generator.RegenerateIncremental(axioms, cfg, dirty);
    assert(incremental.tensor_field.getWidth() > 0);

    StageMask validation_only{};
    MarkStageDirty(validation_only, GenerationStage::Validation);
    const auto revalidated = generator.RegenerateIncremental(axioms, cfg, validation_only);
    assert(revalidated.roads.size() == incremental.roads.size());

    // Non-cascading partial execution must only emit requested layers.
    CityGenerator::StageOptions preview{};
    preview.use_cache = false;
    preview.cascade_downstream = false;
    preview.stages_to_run.reset();
    MarkStageDirty(preview.stages_to_run, GenerationStage::Terrain);
    MarkStageDirty(preview.stages_to_run, GenerationStage::TensorField);
    MarkStageDirty(preview.stages_to_run, GenerationStage::Roads);
    const auto preview_output = generator.GenerateStages(axioms, cfg, preview);
    assert(preview_output.roads.size() > 0);
    assert(preview_output.districts.empty());
    assert(preview_output.blocks.empty());
    assert(preview_output.lots.empty());
    assert(preview_output.buildings.empty());
    assert(preview_output.plan_violations.empty());

    // Stage dependency cascade: tensor dirty should regenerate downstream and match a full generation.
    auto moved_axioms = axioms;
    moved_axioms[0].position = Vec2(820.0, 980.0);
    StageMask tensor_only{};
    MarkStageDirty(tensor_only, GenerationStage::TensorField);
    const auto cascade_output = generator.RegenerateIncremental(moved_axioms, cfg, tensor_only);
    const auto cascade_full = generator.generate(moved_axioms, cfg);
    assert(cascade_output.roads.size() == cascade_full.roads.size());
    assert(cascade_output.districts.size() == cascade_full.districts.size());
    assert(cascade_output.lots.size() == cascade_full.lots.size());

    // Cache invalidation on config changes: validation-only dirty mask should still yield full parity.
    auto cfg_changed = cfg;
    cfg_changed.num_seeds = cfg.num_seeds + 4;
    const auto invalidated = generator.RegenerateIncremental(moved_axioms, cfg_changed, validation_only);
    const auto invalidated_full = generator.generate(moved_axioms, cfg_changed);
    assert(invalidated.roads.size() == invalidated_full.roads.size());
    assert(invalidated.districts.size() == invalidated_full.districts.size());
    assert(invalidated.lots.size() == invalidated_full.lots.size());

    return 0;
}
