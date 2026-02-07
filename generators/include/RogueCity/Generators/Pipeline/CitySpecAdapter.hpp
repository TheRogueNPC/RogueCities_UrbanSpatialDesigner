#pragma once

#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <string>
#include <vector>

namespace RogueCity::Generators {

/// Materialized generation request derived from a high-level CitySpec.
struct CitySpecGenerationRequest {
    CityGenerator::Config config{};
    std::vector<CityGenerator::AxiomInput> axioms;
    std::vector<std::string> tags;
};

/// Converts Core::CitySpec payloads into generator-ready config + axioms.
class CitySpecAdapter {
public:
    static bool TryBuildRequest(
        const Core::CitySpec& spec,
        CitySpecGenerationRequest& outRequest,
        std::string* outError = nullptr);

    static CitySpecGenerationRequest BuildRequest(const Core::CitySpec& spec);
};

} // namespace RogueCity::Generators
