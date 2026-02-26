/**
 * @file Policies.cpp
 * @brief Implements the Road MDP (Markov Decision Process) policies for road
 * network generation.
 *
 * This file defines the concrete behaviors for different urban design patterns:
 * - GridPolicy: Forces strict grid alignment near major roads, otherwise snaps
 * to intersections.
 * - OrganicPolicy: Follows the underlying tensor field loosely, terminating in
 * cul-de-sacs if density gets too high.
 * - FollowTensorPolicy: A pure streamline trace that strictly adheres to the
 * tensor field without constraint.
 */

#include "RogueCity/Generators/Roads/Policies.hpp"

namespace RogueCity::Generators::Roads {

Core::Roads::RoadAction
GridPolicy::ChooseAction(const Core::Roads::RoadState &state,
                         uint32_t rng_seed) const {
  // If close to a major road, force a strict grid alignment
  // 50.0f relates to typical secondary block penetration depths
  if (state.distance_to_major < 50.0f) {
    return Core::Roads::RoadAction::FORCE_GRID;
  }
  // Otherwise default to snapping to form clean grids
  return Core::Roads::RoadAction::SNAP_TO_INTERSECTION;
}

Core::Roads::RoadAction
OrganicPolicy::ChooseAction(const Core::Roads::RoadState &state,
                            uint32_t rng_seed) const {
  // If the area is becoming too dense, spawn a cul-de-sac to terminate growth
  if (state.local_density > 0.8f) {
    return Core::Roads::RoadAction::TERMINATE_CUL_DE_SAC;
  }
  // Otherwise freely follow the tensor field for organic curvature
  return Core::Roads::RoadAction::FOLLOW_TENSOR;
}

Core::Roads::RoadAction
FollowTensorPolicy::ChooseAction(const Core::Roads::RoadState &state,
                                 uint32_t rng_seed) const {
  return Core::Roads::RoadAction::FOLLOW_TENSOR;
}

} // namespace RogueCity::Generators::Roads
