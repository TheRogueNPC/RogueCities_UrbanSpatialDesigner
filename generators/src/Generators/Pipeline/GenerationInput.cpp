#include "RogueCity/Generators/Pipeline/GenerationInput.hpp"

namespace RogueCity::Generators {

CityGenerator::Config GenerationInput::effectiveConfig() const {
  CityGenerator::Config resolved = config;
  if (deterministic_seed != 0u) {
    resolved.seed = deterministic_seed;
  }
  return resolved;
}

ProspectLayers GenerationInput::effectiveProspects() const {
  if (prospect_layers.has_value()) {
    return *prospect_layers;
  }
  return ProspectLayers::Neutral();
}

} // namespace RogueCity::Generators
