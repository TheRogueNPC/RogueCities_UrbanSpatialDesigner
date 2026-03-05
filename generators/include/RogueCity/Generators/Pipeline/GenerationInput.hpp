#pragma once

#include "RogueCity/Core/Data/SpatialReference.hpp"
#include "RogueCity/Core/Util/FastVectorArray.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <optional>
#include <vector>

namespace RogueCity::Generators {

struct ProspectLayers {
  float accessibility_bias{1.0f};
  float exposure_bias{1.0f};
  float serviceability_bias{1.0f};
  float privacy_bias{1.0f};

  [[nodiscard]] static ProspectLayers Neutral() { return {}; }
};

struct ImportedNetwork {
  fva::Container<Core::Road> roads{};
  std::vector<Core::District> regions{};
  Core::Data::SpatialReference spatial_reference{
      Core::Data::SpatialReference::LocalPlanarMeters()};

  [[nodiscard]] bool empty() const {
    return roads.size() == 0 && regions.empty();
  }
};

struct GenerationInput {
  std::optional<std::vector<CityGenerator::AxiomInput>> axioms{};
  std::optional<ProspectLayers> prospect_layers{};
  std::optional<ImportedNetwork> imported_network{};

  CityGenerator::Config config{};
  uint32_t deterministic_seed{0};

  [[nodiscard]] bool hasAxioms() const {
    return axioms.has_value() && !axioms->empty();
  }

  [[nodiscard]] bool hasProspectLayers() const {
    return prospect_layers.has_value();
  }

  [[nodiscard]] bool hasImportedNetwork() const {
    return imported_network.has_value() && !imported_network->empty();
  }

  [[nodiscard]] CityGenerator::Config effectiveConfig() const;
  [[nodiscard]] ProspectLayers effectiveProspects() const;
};

} // namespace RogueCity::Generators
