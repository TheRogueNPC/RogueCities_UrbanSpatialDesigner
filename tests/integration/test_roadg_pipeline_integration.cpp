#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <vector>

int main() {
  using RogueCity::Core::Vec2;
  using RogueCity::Generators::CityGenerator;

  CityGenerator::AxiomInput axiom{};
  axiom.id = 1;
  axiom.type = CityGenerator::AxiomInput::Type::Grid;
  axiom.position = Vec2(900.0, 900.0);
  axiom.radius = 360.0;
  axiom.decay = 2.0;

  std::vector<CityGenerator::AxiomInput> axioms{axiom};

  CityGenerator::Config cfg{};
  cfg.width = 1800;
  cfg.height = 1800;
  cfg.seed = 2026u;
  cfg.num_seeds = 20;

  CityGenerator baseline_gen{};
  const auto baseline = baseline_gen.generate(axioms, cfg);

  cfg.enable_roadg_neighborhood_builder = true;
  cfg.roadg_grid_spacing = 120.0;

  CityGenerator roadg_gen{};
  const auto roadg_out = roadg_gen.generate(axioms, cfg);
  const auto roadg_repeat = roadg_gen.generate(axioms, cfg);

  assert(roadg_out.roads.size() >= baseline.roads.size());
  assert(roadg_out.roads.size() == roadg_repeat.roads.size());
  assert(roadg_out.connector_debug_edges.size() >= baseline.connector_debug_edges.size());

  return 0;
}
