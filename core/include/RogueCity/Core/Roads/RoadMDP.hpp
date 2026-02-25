#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <cstdint>

namespace RogueCity::Core::Roads {

enum class RoadAction : uint8_t {
  FOLLOW_TENSOR,
  FORCE_GRID,
  SNAP_TO_INTERSECTION,
  TERMINATE_CUL_DE_SAC
};

struct RoadState {
  Vec2 position;
  Vec2 tangent;
  float distance_to_major;
  float local_density;
  uint8_t zone_type;
};

class IRoadPolicy {
public:
  virtual ~IRoadPolicy() = default;
  [[nodiscard]] virtual RoadAction ChooseAction(const RoadState &state,
                                                uint32_t rng_seed) const = 0;
};

} // namespace RogueCity::Core::Roads
