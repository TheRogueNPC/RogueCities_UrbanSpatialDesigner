#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <vector>

namespace RogueCity::Generators {

class DefaultAxiomScaffoldPreset {
public:
  [[nodiscard]] static std::vector<CityGenerator::AxiomInput>
  Build(const CityGenerator::Config &config);
};

} // namespace RogueCity::Generators
