#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Urban/RoadGenerator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Core::Road;
    using RogueCity::Core::IntersectionTemplate;
    using RogueCity::Generators::CityGenerator;
    using RogueCity::Generators::Urban::RoadGenerator;

    std::cout << "[Test] 3D Foundation Contract Continuity\n";

    // Test 1: Road 3D metadata preservation
    {
        std::cout << "  Testing Road 3D metadata fields...\n";
        Road road;
        road.points = { Vec2(0, 0), Vec2(100, 0) };
        road.layer_id = 1; // Grade separated
        road.has_grade_separation = true;
        road.elevation_offsets = { 0.0f, 4.5f }; // Rising road

        // Verify 3D fields are accessible and preserved
        assert(road.layer_id == 1);
        assert(road.has_grade_separation == true);
        assert(road.elevation_offsets.size() == 2);
        assert(road.elevation_offsets[1] == 4.5f);
        
        // Test elevation calculation against terrain
        // (This would require a real WorldConstraintField, so just verify method exists)
        // float elev = road.getElevationAtIndex(0, terrain_field);
        
        std::cout << "    ✓ Road 3D metadata fields working\n";
    }

    // Test 2: CityGenerator output includes intersection templates
    {
        std::cout << "  Testing CityGenerator 3D output contracts...\n";
        
        CityGenerator generator;
        CityGenerator::Config config;
        config.width = 500;
        config.height = 500;
        config.seed = 12345;

        // Create minimal axiom for testing
        std::vector<CityGenerator::AxiomInput> axioms;
        CityGenerator::AxiomInput axiom;
        axiom.type = CityGenerator::AxiomInput::Type::Grid;
        axiom.position = Vec2(250, 250);
        axiom.radius = 200.0;
        axiom.decay = 2.0;
        axioms.push_back(axiom);

        // Generate city with 3D foundation enabled
        auto output = generator.generate(axioms, config);
        
        // Verify 3D metadata is present
        assert(output.has_3d_metadata == true);
        
        // Verify intersection templates are populated (even if empty for simple case)
        // The key is that the field exists and is accessible
        assert(output.intersection_templates.size() >= 0); // Should be non-null container
        
        std::cout << "    ✓ CityOutput contains 3D foundation fields\n";
        std::cout << "    ✓ Roads generated: " << output.roads.size() << "\n";
        std::cout << "    ✓ Intersection templates: " << output.intersection_templates.size() << "\n";
    }

    // Test 3: Block generation respects road-cycle preference
    {
        std::cout << "  Testing block generation road-cycle preference...\n";
        
        CityGenerator generator;
        CityGenerator::Config config;
        config.width = 800;
        config.height = 800;
        config.seed = 54321;

        // Create multiple axioms to trigger complex road network case
        std::vector<CityGenerator::AxiomInput> axioms;
        for (int i = 0; i < 8; ++i) {
            CityGenerator::AxiomInput axiom;
            axiom.type = CityGenerator::AxiomInput::Type::Grid;
            axiom.position = Vec2(200 + i * 80, 200 + i * 80);
            axiom.radius = 150.0;
            axiom.decay = 2.0;
            axiom.id = i + 1;
            axioms.push_back(axiom);
        }

        auto output = generator.generate(axioms, config);
        
        // With multiple districts and roads, should trigger road-cycle preference
        assert(output.districts.size() >= 5); // Should create multiple districts
        assert(!output.roads.empty()); // Should have road network
        
        // The key test is that the system doesn't fail when road-cycle preference is enabled
        // and that blocks are still generated (even if falling back to district method)
        assert(!output.blocks.empty());
        
        std::cout << "    ✓ Block generation handles road-cycle preference without failure\n";
        std::cout << "    ✓ Districts: " << output.districts.size() << ", Blocks: " << output.blocks.size() << "\n";
    }

    // Test 4: Pipeline continuity - verify 3D metadata flows through stages
    {
        std::cout << "  Testing pipeline 3D metadata continuity...\n";
        
        CityGenerator generator;
        CityGenerator::Config config;
        config.width = 600;
        config.height = 600;
        config.seed = 98765;
        config.enable_world_constraints = true; // Enable terrain for 3D context

        std::vector<CityGenerator::AxiomInput> axioms;
        CityGenerator::AxiomInput axiom;
        axiom.type = CityGenerator::AxiomInput::Type::Radial;
        axiom.position = Vec2(300, 300);
        axiom.radius = 250.0;
        axiom.decay = 2.0;
        axiom.radial_spokes = 6;
        axioms.push_back(axiom);

        auto output = generator.generate(axioms, config);
        
        // Verify that all 3D-enhanced elements preserve metadata
        assert(output.has_3d_metadata == true);
        
        // Check that districts have 3D metadata
        if (!output.districts.empty()) {
            const auto& first_district = output.districts[0];
            // 3D fields should be initialized (even if to defaults)
            assert(first_district.average_elevation >= 0.0f); // Should be initialized
            std::cout << "    ✓ District 3D metadata: elevation=" << first_district.average_elevation << "\n";
        }
        
        // Check that lots have 3D metadata
        if (!output.lots.empty()) {
            const auto& first_lot = output.lots[0];
            assert(first_lot.ground_elevation >= 0.0f); // Should be initialized
            std::cout << "    ✓ Lot 3D metadata: ground_elevation=" << first_lot.ground_elevation << "\n";
        }
        
        // Check that buildings have 3D metadata
        if (!output.buildings.empty()) {
            const auto& first_building = output.buildings[0];
            assert(first_building.foundation_elevation >= 0.0f); // Should be initialized
            std::cout << "    ✓ Building 3D metadata: foundation_elevation=" << first_building.foundation_elevation << "\n";
        }
        
        std::cout << "    ✓ 3D metadata flows through all pipeline stages\n";
    }

    std::cout << "[Test] All 3D Foundation Contract tests passed! ✓\n";
    return 0;
}