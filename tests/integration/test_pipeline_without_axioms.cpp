#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

#include <cassert>
#include <vector>

int main() {
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::GenerationInput;

  CityGenerator::Config cfg{};
  cfg.width = 1600;
  cfg.height = 1600;
  cfg.seed = 4242u;
  cfg.num_seeds = 16;

  GenerationInput input{};
  input.config = cfg;
  input.deterministic_seed = cfg.seed;

  CityGenerator generator{};
  const auto scaffold_output = generator.generate(input);
  assert(scaffold_output.tensor_field.getWidth() > 0);
  assert(scaffold_output.roads.size() > 0u);
  assert(scaffold_output.spatial_reference.isLocalPlanarMeters());

  std::vector<CityGenerator::AxiomInput> empty_axioms;
  const auto legacy_output = generator.generate(empty_axioms, cfg);
  assert(!legacy_output.plan_approved);
  assert(!legacy_output.plan_violations.empty());

  return 0;
}
