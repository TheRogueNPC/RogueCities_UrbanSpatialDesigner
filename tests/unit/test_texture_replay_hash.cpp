#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Export/TextureReplayHash.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Export::TextureReplayHash;
using RogueCity::Generators::CityGenerator;

namespace {
    constexpr uint64_t kGoldenCombinedHash = 0xf249898f6753e6ccull;

    CityGenerator::Config BuildConfig() {
        CityGenerator::Config config{};
        config.width = 1200;
        config.height = 1200;
        config.cell_size = 10.0;
        config.seed = 424242u;
        config.num_seeds = 16;
        config.max_districts = 128;
        config.max_lots = 6000;
        config.max_buildings = 9000;
        config.enable_world_constraints = true;
        return config;
    }

    std::vector<CityGenerator::AxiomInput> BuildAxioms() {
        std::vector<CityGenerator::AxiomInput> axioms;

        CityGenerator::AxiomInput radial{};
        radial.type = CityGenerator::AxiomInput::Type::Radial;
        radial.position = { 540.0, 620.0 };
        radial.radius = 310.0;
        radial.decay = 2.2;
        radial.radial_spokes = 7;
        axioms.push_back(radial);

        CityGenerator::AxiomInput grid{};
        grid.type = CityGenerator::AxiomInput::Type::Grid;
        grid.position = { 760.0, 460.0 };
        grid.radius = 380.0;
        grid.theta = 0.35;
        grid.decay = 1.9;
        axioms.push_back(grid);

        return axioms;
    }
}

int main() {
    const auto config = BuildConfig();
    const auto axioms = BuildAxioms();

    CityGenerator generator;
    GlobalState state_a{};
    const auto output_a = generator.generate(axioms, config, &state_a);
    assert(output_a.world_constraints.isValid());
    assert(state_a.HasTextureSpace());
    const auto digest_a = TextureReplayHash::compute(state_a.TextureSpaceRef());

    GlobalState state_b{};
    const auto output_b = generator.generate(axioms, config, &state_b);
    assert(output_b.world_constraints.isValid());
    assert(state_b.HasTextureSpace());
    const auto digest_b = TextureReplayHash::compute(state_b.TextureSpaceRef());

    assert(digest_a.height == digest_b.height);
    assert(digest_a.material == digest_b.material);
    assert(digest_a.zone == digest_b.zone);
    assert(digest_a.tensor == digest_b.tensor);
    assert(digest_a.distance == digest_b.distance);
    assert(digest_a.combined == digest_b.combined);

    if (digest_a.combined != kGoldenCombinedHash) {
        std::cerr << "Golden combined hash mismatch.\n";
        std::cerr << "  expected: " << TextureReplayHash::toHex(kGoldenCombinedHash) << "\n";
        std::cerr << "  actual:   " << TextureReplayHash::toHex(digest_a.combined) << "\n";
        return 1;
    }

    const std::string manifest_path = "texture_replay_manifest_test.json";
    assert(TextureReplayHash::writeManifest(state_a.TextureSpaceRef(), manifest_path));
    std::ifstream manifest(manifest_path);
    assert(manifest.good());
    std::string line;
    bool saw_combined = false;
    while (std::getline(manifest, line)) {
        if (line.find("\"combined\"") != std::string::npos) {
            saw_combined = true;
            break;
        }
    }
    assert(saw_combined);

    return 0;
}
