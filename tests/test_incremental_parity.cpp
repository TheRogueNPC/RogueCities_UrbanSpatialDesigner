#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

#include <cassert>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::FullStageMask;
    using RogueCity::Generators::GenerationStage;
    using RogueCity::Generators::MarkStageDirty;
    using RogueCity::Generators::StageMask;

    CityGenerator::Config cfg{};
    cfg.width = 2100;
    cfg.height = 1800;
    cfg.cell_size = 10.0;
    cfg.seed = 5151;
    cfg.num_seeds = 24;

    std::vector<CityGenerator::AxiomInput> axioms;
    CityGenerator::AxiomInput radial{};
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = Vec2(980.0, 860.0);
    radial.radius = 420.0;
    axioms.push_back(radial);

    CityGenerator::AxiomInput grid{};
    grid.type = CityGenerator::AxiomInput::Type::Grid;
    grid.position = Vec2(1350.0, 650.0);
    grid.radius = 350.0;
    grid.theta = 0.35;
    axioms.push_back(grid);

    CityGenerator generator{};
    CityGenerator::StageOptions full_opts{};
    full_opts.stages_to_run = FullStageMask();
    full_opts.use_cache = false;

    const auto full_a = generator.GenerateStages(axioms, cfg, full_opts);
    const auto full_b = generator.generate(axioms, cfg);
    assert(full_a.roads.size() == full_b.roads.size());
    assert(full_a.districts.size() == full_b.districts.size());
    assert(full_a.lots.size() == full_b.lots.size());
    assert(full_a.buildings.size() == full_b.buildings.size());

    StageMask dirty{};
    MarkStageDirty(dirty, GenerationStage::TensorField);
    const auto inc_a = generator.RegenerateIncremental(axioms, cfg, dirty);
    const auto full_c = generator.generate(axioms, cfg);
    assert(inc_a.roads.size() == full_c.roads.size());
    assert(inc_a.districts.size() == full_c.districts.size());
    assert(inc_a.lots.size() == full_c.lots.size());
    assert(inc_a.buildings.size() == full_c.buildings.size());

    return 0;
}
