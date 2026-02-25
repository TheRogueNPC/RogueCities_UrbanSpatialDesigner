#include "RogueCity/Generators/Roads/Policies.hpp"

namespace RogueCity::Generators::Roads {

Core::Roads::RoadAction
GridPolicy::ChooseAction(const Core::Roads::RoadState &state,
                         uint32_t rng_seed) const {
  // Stub implementation, will snap to 90 degrees based on rules
  return Core::Roads::RoadAction::FORCE_GRID;
}

Core::Roads::RoadAction
OrganicPolicy::ChooseAction(const Core::Roads::RoadState &state,
                            uint32_t rng_seed) const {
  return Core::Roads::RoadAction::FOLLOW_TENSOR;
}

Core::Roads::RoadAction
FollowTensorPolicy::ChooseAction(const Core::Roads::RoadState &state,
                                 uint32_t rng_seed) const {
  return Core::Roads::RoadAction::FOLLOW_TENSOR;
}

} // namespace RogueCity::Generators::Roads
