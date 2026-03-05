#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/DefaultAxiomScaffoldPreset.hpp"
#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

#include <cassert>
#include <cstdint>

namespace {

uint64_t HashRoadTopology(const RogueCity::Generators::CityGenerator::CityOutput &city) {
  uint64_t hash = 14695981039346656037ull;
  constexpr uint64_t prime = 1099511628211ull;
  auto mix = [&](uint64_t value) {
    hash ^= value;
    hash *= prime;
  };

  mix(static_cast<uint64_t>(city.roads.size()));
  for (const auto &road : city.roads) {
    mix(static_cast<uint64_t>(road.points.size()));
    for (const auto &point : road.points) {
      const int64_t qx = static_cast<int64_t>(point.x * 100.0);
      const int64_t qy = static_cast<int64_t>(point.y * 100.0);
      mix(static_cast<uint64_t>(qx));
      mix(static_cast<uint64_t>(qy));
    }
  }
  return hash;
}

} // namespace

int main() {
  using RogueCity::Generators::CityGenerator;
  using RogueCity::Generators::DefaultAxiomScaffoldPreset;
  using RogueCity::Generators::GenerationInput;

  CityGenerator::Config cfg{};
  cfg.width = 2000;
  cfg.height = 1800;
  cfg.seed = 777u;
  cfg.num_seeds = 20;

  const auto scaffold_a = DefaultAxiomScaffoldPreset::Build(cfg);
  const auto scaffold_b = DefaultAxiomScaffoldPreset::Build(cfg);
  assert(!scaffold_a.empty());
  assert(scaffold_a.size() == scaffold_b.size());
  for (size_t i = 0; i < scaffold_a.size(); ++i) {
    assert(scaffold_a[i].type == scaffold_b[i].type);
    assert(scaffold_a[i].position.equals(scaffold_b[i].position));
    assert(scaffold_a[i].radius == scaffold_b[i].radius);
  }

  GenerationInput input{};
  input.config = cfg;
  input.deterministic_seed = cfg.seed;

  CityGenerator generator{};
  const auto out_a = generator.generate(input);
  const auto out_b = generator.generate(input);

  assert(out_a.tensor_field.getWidth() > 0);
  assert(out_a.roads.size() > 0u);
  assert(out_a.roads.size() == out_b.roads.size());
  assert(HashRoadTopology(out_a) == HashRoadTopology(out_b));

  return 0;
}
