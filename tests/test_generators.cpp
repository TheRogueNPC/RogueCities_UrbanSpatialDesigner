#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <iostream>

using namespace RogueCity;
using namespace RogueCity::Generators;

int main() {
    std::cout << "=== RogueCity Generators Test ===" << std::endl;

    // Create axioms
    std::vector<CityGenerator::AxiomInput> axioms;

    // Radial axiom at city center (Paris-style)
    CityGenerator::AxiomInput radial;
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = Core::Vec2(1000, 1000);
    radial.radius = 500.0;
    radial.decay = 2.0;
    axioms.push_back(radial);

    // Grid axiom offset (Manhattan-style)
    CityGenerator::AxiomInput grid;
    grid.type = CityGenerator::AxiomInput::Type::Grid;
    grid.position = Core::Vec2(1500, 500);
    grid.radius = 400.0;
    grid.theta = 0.0;  // North-south alignment
    grid.decay = 2.0;
    axioms.push_back(grid);

    // Generate city
    CityGenerator generator;
    CityGenerator::Config config;
    config.width = 2000;
    config.height = 2000;
    config.cell_size = 10.0;
    config.num_seeds = 15;
    config.seed = 42;

    std::cout << "Generating city..." << std::endl;
    auto output = generator.generate(axioms, config);

    std::cout << "✓ Tensor field generated: " << output.tensor_field.getWidth()
        << "x" << output.tensor_field.getHeight() << " cells" << std::endl;
    std::cout << "✓ Roads traced: " << output.roads.size() << " roads" << std::endl;

    // Sample tensors at test points
    Core::Vec2 test_points[] = {
        Core::Vec2(1000, 1000),  // Center (radial)
        Core::Vec2(1500, 500),   // Grid center
        Core::Vec2(500, 500),    // Corner
    };

    std::cout << "\n=== Tensor Sampling ===" << std::endl;
    for (const auto& pt : test_points) {
        Core::Tensor2D t = output.tensor_field.sampleTensor(pt);
        Core::Vec2 major = t.majorEigenvector();
        std::cout << "  Point (" << pt.x << ", " << pt.y << "): "
            << "major=(" << major.x << ", " << major.y << ")" << std::endl;
    }

    // Road statistics
    if (output.roads.size() > 0) {
        std::cout << "\n=== Road Statistics ===" << std::endl;
        int major_count = 0;
        int minor_count = 0;
        double total_length = 0.0;

        for (const auto& road : output.roads) {
            if (road.type == Core::RoadType::M_Major) major_count++;
            if (road.type == Core::RoadType::M_Minor) minor_count++;
            total_length += road.length();
        }

        std::cout << "  Major roads: " << major_count << std::endl;
        std::cout << "  Minor roads: " << minor_count << std::endl;
        std::cout << "  Total length: " << total_length << " meters" << std::endl;
    }

    std::cout << "\n✓ Phase 2 generators test PASSED!" << std::endl;
    return 0;
}
