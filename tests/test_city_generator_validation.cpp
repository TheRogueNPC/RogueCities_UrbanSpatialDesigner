#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <vector>

int main() {
    using RogueCity::Core::Vec2;
    using RogueCity::Generators::CityGenerator;

    CityGenerator::Config cfg{};
    cfg.width = 100;
    cfg.height = 12000;
    cfg.cell_size = 0.01;
    cfg.num_seeds = 1000;
    cfg.max_texture_resolution = 1024;
    cfg.max_districts = 999999;
    cfg.max_lots = 999999;
    cfg.max_buildings = 999999;

    const auto validated = CityGenerator::ValidateAndClampConfig(cfg);
    assert(validated.ok());
    assert(validated.clamped_config.width >= 500 && validated.clamped_config.width <= 8192);
    assert(validated.clamped_config.height >= 500 && validated.clamped_config.height <= 8192);
    assert(validated.clamped_config.cell_size >= 1.0 && validated.clamped_config.cell_size <= 50.0);
    assert(validated.clamped_config.num_seeds >= 5 && validated.clamped_config.num_seeds <= 200);
    assert(validated.clamped_config.max_districts <= 4096u);

    std::vector<CityGenerator::AxiomInput> empty_axioms;
    std::vector<std::string> errors;
    assert(!CityGenerator::ValidateAxioms(empty_axioms, validated.clamped_config, &errors));
    assert(!errors.empty());

    std::vector<CityGenerator::AxiomInput> bad_axioms;
    CityGenerator::AxiomInput bad{};
    bad.type = CityGenerator::AxiomInput::Type::Radial;
    bad.position = Vec2(-10.0, 15.0);
    bad.radius = -5.0;
    bad.decay = 0.5;
    bad.radial_spokes = 2;
    bad_axioms.push_back(bad);

    errors.clear();
    assert(!CityGenerator::ValidateAxioms(bad_axioms, validated.clamped_config, &errors));
    assert(!errors.empty());

    CityGenerator::AxiomInput good{};
    good.type = CityGenerator::AxiomInput::Type::Grid;
    good.position = Vec2(
        static_cast<double>(validated.clamped_config.width) * 0.5,
        static_cast<double>(validated.clamped_config.height) * 0.5);
    good.radius = 280.0;
    good.decay = 2.0;
    good.warp_lattice.topology_type = 1; // Polygon
    good.warp_lattice.zone_inner_uv = 0.33f;
    good.warp_lattice.zone_middle_uv = 0.67f;
    good.warp_lattice.zone_outer_uv = 1.0f;
    good.warp_lattice.vertices = {
        Vec2(good.position.x - 280.0, good.position.y - 280.0),
        Vec2(good.position.x + 280.0, good.position.y - 280.0),
        Vec2(good.position.x + 280.0, good.position.y + 280.0),
        Vec2(good.position.x - 280.0, good.position.y + 280.0)
    };

    std::vector<CityGenerator::AxiomInput> good_axioms{ good };
    errors.clear();
    assert(CityGenerator::ValidateAxioms(good_axioms, validated.clamped_config, &errors));

    {
        auto invalid = good;
        invalid.warp_lattice.topology_type = 99;
        errors.clear();
        assert(!CityGenerator::ValidateAxioms({ invalid }, validated.clamped_config, &errors));
    }

    {
        auto invalid = good;
        invalid.warp_lattice.zone_inner_uv = 0.7f;
        invalid.warp_lattice.zone_middle_uv = 0.6f;
        errors.clear();
        assert(!CityGenerator::ValidateAxioms({ invalid }, validated.clamped_config, &errors));
    }

    {
        auto invalid = good;
        invalid.warp_lattice.topology_type = 0; // BezierPatch
        invalid.warp_lattice.rows = 4;
        invalid.warp_lattice.cols = 4;
        invalid.warp_lattice.vertices.resize(15); // Should be 16.
        errors.clear();
        assert(!CityGenerator::ValidateAxioms({ invalid }, validated.clamped_config, &errors));
    }

    {
        auto invalid = good;
        invalid.warp_lattice.topology_type = 1; // Polygon
        invalid.warp_lattice.vertices = {
            Vec2(good.position.x, good.position.y),
            Vec2(good.position.x + 5.0, good.position.y)
        };
        errors.clear();
        assert(!CityGenerator::ValidateAxioms({ invalid }, validated.clamped_config, &errors));
    }

    {
        auto valid_patch = good;
        valid_patch.type = CityGenerator::AxiomInput::Type::Organic;
        valid_patch.warp_lattice.topology_type = 0; // BezierPatch
        valid_patch.warp_lattice.rows = 4;
        valid_patch.warp_lattice.cols = 4;
        valid_patch.warp_lattice.vertices.clear();
        valid_patch.warp_lattice.vertices.reserve(16);
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                const double px = valid_patch.position.x - 180.0 + static_cast<double>(x) * 120.0;
                const double py = valid_patch.position.y - 180.0 + static_cast<double>(y) * 120.0;
                valid_patch.warp_lattice.vertices.emplace_back(px, py);
            }
        }
        errors.clear();
        assert(CityGenerator::ValidateAxioms({ valid_patch }, validated.clamped_config, &errors));
    }

    CityGenerator generator;
    const auto output = generator.generate(good_axioms, validated.clamped_config);
    assert(output.tensor_field.getWidth() > 0);

    return 0;
}
