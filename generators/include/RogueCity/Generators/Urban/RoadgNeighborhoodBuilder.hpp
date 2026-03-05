#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"

#include <array>
#include <vector>

namespace RogueCity::Generators::Urban {

struct RoadgRegion {
  uint32_t id{0};
  std::vector<Core::Vec2> boundary{};
};

class RoadgNeighborhoodBuilder {
public:
  enum class BoundarySide : uint8_t { North = 0, East, South, West };

  struct Config {
    uint32_t seed{12345u};
    double grid_spacing{120.0};
    double boundary_inset{12.0};
    bool enforce_stitch_per_side{true};
  };

  struct Input {
    fva::Container<Core::Road> outer_roads{};
    std::vector<RoadgRegion> regions{};
  };

  struct RegionMetadata {
    uint32_t region_id{0};
    uint32_t inner_road_count{0};
    uint32_t stitch_count{0};
    std::array<uint32_t, 4> stitches_per_side{{0u, 0u, 0u, 0u}};
    bool connected{false};
  };

  struct Output {
    fva::Container<Core::Road> inner_roads{};
    fva::Container<Core::Road> stitch_edges{};
    std::vector<RegionMetadata> metadata{};
  };

  [[nodiscard]] static std::vector<RoadgRegion>
  InferRegionsFromAxiomSkeleton(const fva::Container<Core::Road> &outer_roads);

  [[nodiscard]] static Output Build(const Input &input, const Config &config);
};

} // namespace RogueCity::Generators::Urban
