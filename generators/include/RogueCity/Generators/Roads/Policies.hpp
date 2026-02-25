#pragma once

#include "RogueCity/Core/Roads/RoadMDP.hpp"

namespace RogueCity::Generators::Roads {

class GridPolicy : public Core::Roads::IRoadPolicy {
public:
  [[nodiscard]] Core::Roads::RoadAction
  ChooseAction(const Core::Roads::RoadState &state,
               uint32_t rng_seed) const override;
};

class OrganicPolicy : public Core::Roads::IRoadPolicy {
public:
  [[nodiscard]] Core::Roads::RoadAction
  ChooseAction(const Core::Roads::RoadState &state,
               uint32_t rng_seed) const override;
};

class FollowTensorPolicy : public Core::Roads::IRoadPolicy {
public:
  [[nodiscard]] Core::Roads::RoadAction
  ChooseAction(const Core::Roads::RoadState &state,
               uint32_t rng_seed) const override;
};

} // namespace RogueCity::Generators::Roads
