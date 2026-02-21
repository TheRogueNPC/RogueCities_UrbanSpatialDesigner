#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <vector>

namespace RogueCity::Generators {

struct AxiomInteractionEdge {
    int axiom_a_id{ 0 };
    int axiom_b_id{ 0 };
    Core::Vec2 border_start{};
    Core::Vec2 border_end{};
};

struct AxiomInteractionResult {
    std::vector<CityGenerator::AxiomInput> resolved_axioms{};
    std::vector<AxiomInteractionEdge> falloff_borders{};
};

class AxiomInteractionResolver {
public:
    [[nodiscard]] AxiomInteractionResult Resolve(
        const std::vector<CityGenerator::AxiomInput>& axioms) const;
};

} // namespace RogueCity::Generators

