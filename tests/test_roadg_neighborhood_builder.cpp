#include "RogueCity/Generators/Urban/RoadgNeighborhoodBuilder.hpp"

#include <array>
#include <cassert>

int main() {
  using RogueCity::Core::Vec2;
  using RogueCity::Generators::Urban::RoadgNeighborhoodBuilder;
  using RogueCity::Generators::Urban::RoadgRegion;

  RoadgRegion region{};
  region.id = 7u;
  region.boundary = {
      Vec2(100.0, 100.0),
      Vec2(500.0, 100.0),
      Vec2(500.0, 500.0),
      Vec2(100.0, 500.0),
  };

  RoadgNeighborhoodBuilder::Input input{};
  input.regions.push_back(region);

  RoadgNeighborhoodBuilder::Config cfg{};
  cfg.seed = 123u;
  cfg.grid_spacing = 100.0;

  const auto out_a = RoadgNeighborhoodBuilder::Build(input, cfg);
  const auto out_b = RoadgNeighborhoodBuilder::Build(input, cfg);

  assert(out_a.inner_roads.size() > 0u);
  assert(out_a.stitch_edges.size() >= 4u);
  assert(out_a.inner_roads.size() == out_b.inner_roads.size());
  assert(out_a.stitch_edges.size() == out_b.stitch_edges.size());
  assert(out_a.metadata.size() == 1u);

  const auto &meta = out_a.metadata.front();
  assert(meta.connected);
  for (const uint32_t per_side : meta.stitches_per_side) {
    assert(per_side >= 1u);
  }

  return 0;
}
