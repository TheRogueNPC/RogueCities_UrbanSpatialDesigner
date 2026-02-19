#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Validation/DeterminismHash.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <array>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Validation::ComputeDeterminismHash;
using RogueCity::Core::Validation::ValidateAgainstBaseline;
using RogueCity::Generators::CityGenerator;

#ifndef ROGUECITY_SOURCE_DIR
#define ROGUECITY_SOURCE_DIR "."
#endif

namespace {

void PopulateGlobalState(const CityGenerator::CityOutput& output, GlobalState& gs) {
    gs.roads.clear();
    for (const auto& road : output.roads) {
        gs.roads.add(road);
    }

    gs.districts.clear();
    for (const auto& district : output.districts) {
        gs.districts.add(district);
    }

    gs.blocks.clear();
    for (const auto& block : output.blocks) {
        gs.blocks.add(block);
    }

    gs.lots.clear();
    for (const auto& lot : output.lots) {
        gs.lots.add(lot);
    }

    gs.buildings.clear();
    for (const auto& building : output.buildings) {
        gs.buildings.push_back(building);
    }
}

CityGenerator::Config BuildConfig(uint32_t seed, int width = 2000, int height = 2000) {
    CityGenerator::Config cfg{};
    cfg.width = width;
    cfg.height = height;
    cfg.cell_size = 10.0;
    cfg.seed = seed;
    cfg.num_seeds = 18;
    cfg.max_districts = 220;
    cfg.max_lots = 18000;
    cfg.max_buildings = 26000;
    cfg.enable_world_constraints = true;
    return cfg;
}

std::vector<CityGenerator::AxiomInput> BuildAllTypeAxioms(int width, int height) {
    std::vector<CityGenerator::AxiomInput> axioms;
    const double cx = static_cast<double>(width) * 0.5;
    const double cy = static_cast<double>(height) * 0.5;

    for (int i = 0; i < static_cast<int>(CityGenerator::AxiomInput::Type::COUNT); ++i) {
        CityGenerator::AxiomInput ax{};
        ax.type = static_cast<CityGenerator::AxiomInput::Type>(i);
        ax.position = RogueCity::Core::Vec2(cx + (i - 4) * 60.0, cy + (i - 4) * 45.0);
        ax.radius = 220.0 + static_cast<double>(i) * 20.0;
        ax.theta = 0.15 * i;
        ax.decay = 2.0;
        ax.radial_spokes = 8;
        axioms.push_back(ax);
    }

    return axioms;
}

RogueCity::Core::Validation::DeterminismHash GenerateHash(
    const std::vector<CityGenerator::AxiomInput>& axioms,
    const CityGenerator::Config& cfg) {
    CityGenerator generator{};
    GlobalState gs{};
    const auto output = generator.generate(axioms, cfg, &gs);
    PopulateGlobalState(output, gs);
    return ComputeDeterminismHash(gs);
}

} // namespace

int main() {
    // 10 seeds deterministic replay.
    for (uint32_t seed = 1001; seed < 1011; ++seed) {
        auto cfg = BuildConfig(seed);
        std::vector<CityGenerator::AxiomInput> axioms;
        CityGenerator::AxiomInput ax{};
        ax.type = CityGenerator::AxiomInput::Type::Radial;
        ax.position = RogueCity::Core::Vec2(1000.0, 1000.0);
        ax.radius = 380.0;
        axioms.push_back(ax);

        const auto h0 = GenerateHash(axioms, cfg);
        const auto h1 = GenerateHash(axioms, cfg);
        assert(h0 == h1);
    }

    // World size variants deterministic replay.
    const std::array<std::pair<int, int>, 3> sizes{{ {1200, 1200}, {1800, 1600}, {2400, 2200} }};
    for (const auto& [w, h] : sizes) {
        auto cfg = BuildConfig(30303u, w, h);
        std::vector<CityGenerator::AxiomInput> axioms;
        CityGenerator::AxiomInput ax{};
        ax.type = CityGenerator::AxiomInput::Type::Grid;
        ax.position = RogueCity::Core::Vec2(static_cast<double>(w) * 0.5, static_cast<double>(h) * 0.5);
        ax.radius = 300.0;
        axioms.push_back(ax);

        const auto h0 = GenerateHash(axioms, cfg);
        const auto h1 = GenerateHash(axioms, cfg);
        assert(h0 == h1);
    }

    // All axiom types in one run deterministic replay.
    {
        auto cfg = BuildConfig(90909u, 2200, 2200);
        auto axioms = BuildAllTypeAxioms(cfg.width, cfg.height);
        const auto h0 = GenerateHash(axioms, cfg);
        const auto h1 = GenerateHash(axioms, cfg);
        assert(h0 == h1);
    }

    // Baseline regression check.
    {
        GlobalState empty{};
        const auto empty_hash = ComputeDeterminismHash(empty);
        const std::string baseline_path =
            std::string(ROGUECITY_SOURCE_DIR) + "/tests/baselines/determinism_v0.10.txt";
        assert(ValidateAgainstBaseline(empty_hash, baseline_path));
    }

    return 0;
}
