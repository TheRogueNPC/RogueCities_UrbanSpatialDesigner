#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <utility>
#include <vector>

namespace RogueCity::Generators {

struct MajorConnectorGraphConfig {
    int max_degree_per_node{ 3 };
    int extra_edges_per_node{ 1 };
    double max_edge_length_factor{ 2.8 };
};

struct MajorConnectorGraphOutput {
    std::vector<std::pair<int, int>> edges{};
};

class MajorConnectorGraph {
public:
    [[nodiscard]] MajorConnectorGraphOutput Build(
        const std::vector<CityGenerator::AxiomInput>& axioms,
        const MajorConnectorGraphConfig& config = {}) const;
};

} // namespace RogueCity::Generators

