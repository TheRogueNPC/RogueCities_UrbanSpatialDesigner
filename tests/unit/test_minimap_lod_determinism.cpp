#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Export/TextureReplayHash.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <array>
#include <cassert>

using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Export::TextureReplayDigest;
using RogueCity::Core::Export::TextureReplayHash;
using RogueCity::Generators::CityGenerator;

namespace {
    CityGenerator::Config BuildConfig() {
        CityGenerator::Config config{};
        config.width = 1200;
        config.height = 1200;
        config.cell_size = 10.0;
        config.seed = 991122u;
        config.num_seeds = 18;
        config.max_districts = 140;
        config.max_lots = 7000;
        config.max_buildings = 10000;
        config.enable_world_constraints = true;
        return config;
    }

    std::vector<CityGenerator::AxiomInput> BuildAxioms() {
        std::vector<CityGenerator::AxiomInput> axioms;

        CityGenerator::AxiomInput radial{};
        radial.type = CityGenerator::AxiomInput::Type::Radial;
        radial.position = { 520.0, 610.0 };
        radial.radius = 320.0;
        radial.decay = 2.1;
        radial.radial_spokes = 8;
        axioms.push_back(radial);

        CityGenerator::AxiomInput grid{};
        grid.type = CityGenerator::AxiomInput::Type::Grid;
        grid.position = { 780.0, 430.0 };
        grid.radius = 410.0;
        grid.theta = 0.2;
        grid.decay = 2.0;
        axioms.push_back(grid);

        return axioms;
    }

    void AssertDigestEqual(const TextureReplayDigest& a, const TextureReplayDigest& b) {
        assert(a.height == b.height);
        assert(a.material == b.material);
        assert(a.zone == b.zone);
        assert(a.tensor == b.tensor);
        assert(a.distance == b.distance);
        assert(a.combined == b.combined);
    }
}

int main() {
    const auto config = BuildConfig();
    const auto axioms = BuildAxioms();

    CityGenerator generator;
    GlobalState state{};

    state.minimap_manual_lod = false;
    state.minimap_lod_level = 1u;
    const auto baseline_output = generator.generate(axioms, config, &state);
    assert(baseline_output.world_constraints.isValid());
    assert(state.HasTextureSpace());
    const TextureReplayDigest baseline_digest = TextureReplayHash::compute(state.TextureSpaceRef());

    // LOD state transitions are render-only and must not affect generation/replay hashes.
    const std::array<uint8_t, 4> lod_sequence = { 0u, 1u, 2u, 0u };
    for (const uint8_t lod : lod_sequence) {
        state.minimap_manual_lod = true;
        state.minimap_lod_level = lod;

        const auto output = generator.generate(axioms, config, &state);
        assert(output.world_constraints.isValid());
        assert(state.HasTextureSpace());
        const TextureReplayDigest digest = TextureReplayHash::compute(state.TextureSpaceRef());
        AssertDigestEqual(baseline_digest, digest);
    }

    state.minimap_manual_lod = false;
    state.minimap_lod_level = 1u;
    const auto reset_output = generator.generate(axioms, config, &state);
    assert(reset_output.world_constraints.isValid());
    const TextureReplayDigest reset_digest = TextureReplayHash::compute(state.TextureSpaceRef());
    AssertDigestEqual(baseline_digest, reset_digest);

    return 0;
}
