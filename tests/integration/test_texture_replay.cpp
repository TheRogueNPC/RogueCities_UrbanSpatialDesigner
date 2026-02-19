#include "RogueCity/App/Integration/CityOutputApplier.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/TexturePainting.hpp"
#include "RogueCity/Core/Export/TextureReplayHash.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <vector>

namespace {

using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::TexturePainting;
using RogueCity::Core::Export::TextureReplayDigest;
using RogueCity::Core::Export::TextureReplayHash;
using RogueCity::Generators::CityGenerator;

CityGenerator::Config BuildConfig() {
    CityGenerator::Config cfg{};
    cfg.width = 1600;
    cfg.height = 1600;
    cfg.cell_size = 10.0;
    cfg.seed = 606060u;
    cfg.num_seeds = 22;
    cfg.max_districts = 256;
    cfg.max_lots = 20000;
    cfg.max_buildings = 32000;
    cfg.enable_world_constraints = true;
    return cfg;
}

std::vector<CityGenerator::AxiomInput> BuildAxioms() {
    std::vector<CityGenerator::AxiomInput> axioms;

    CityGenerator::AxiomInput radial{};
    radial.type = CityGenerator::AxiomInput::Type::Radial;
    radial.position = { 620.0, 810.0 };
    radial.radius = 340.0;
    radial.decay = 2.0;
    radial.radial_spokes = 8;
    axioms.push_back(radial);

    CityGenerator::AxiomInput loose{};
    loose.type = CityGenerator::AxiomInput::Type::LooseGrid;
    loose.position = { 940.0, 700.0 };
    loose.radius = 300.0;
    loose.theta = 0.35;
    loose.decay = 2.1;
    loose.loose_grid_jitter = 0.15f;
    axioms.push_back(loose);

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

void GenerateAndApplyState(
    const std::vector<CityGenerator::AxiomInput>& axioms,
    const CityGenerator::Config& cfg,
    GlobalState& state) {
    CityGenerator generator{};
    const auto output = generator.generate(axioms, cfg, &state);
    assert(output.tensor_field.getWidth() > 0);
    assert(output.world_constraints.isValid());
    RogueCity::App::ApplyCityOutputToGlobalState(output, state);
    assert(state.HasTextureSpace());
}

void ApplyDeterministicPaintSequence(GlobalState& state) {
    const std::vector<TexturePainting::Stroke> strokes = {
        TexturePainting::Stroke{ {720.0, 760.0}, 70.0, 180u, 0.85f, TexturePainting::Layer::Material },
        TexturePainting::Stroke{ {880.0, 720.0}, 55.0, 120u, 0.90f, TexturePainting::Layer::Material },
        TexturePainting::Stroke{ {760.0, 910.0}, 65.0, 4u, 0.80f, TexturePainting::Layer::Zone },
        TexturePainting::Stroke{ {930.0, 830.0}, 60.0, 7u, 0.75f, TexturePainting::Layer::Zone }
    };

    bool any_applied = false;
    for (const auto& stroke : strokes) {
        any_applied = state.ApplyTexturePaint(stroke) || any_applied;
        state.frame_counter += 1;
    }
    assert(any_applied);
}

} // namespace

int main() {
    const auto cfg = BuildConfig();
    const auto axioms = BuildAxioms();

    // Texture-space hash verification after full pipeline application.
    GlobalState state_a{};
    GenerateAndApplyState(axioms, cfg, state_a);
    const TextureReplayDigest digest_a = TextureReplayHash::compute(state_a.TextureSpaceRef());

    GlobalState state_b{};
    GenerateAndApplyState(axioms, cfg, state_b);
    const TextureReplayDigest digest_b = TextureReplayHash::compute(state_b.TextureSpaceRef());

    AssertDigestEqual(digest_a, digest_b);

    // Deterministic texture painting replay.
    const TextureReplayDigest before_paint_a = TextureReplayHash::compute(state_a.TextureSpaceRef());
    const TextureReplayDigest before_paint_b = TextureReplayHash::compute(state_b.TextureSpaceRef());
    AssertDigestEqual(before_paint_a, before_paint_b);

    ApplyDeterministicPaintSequence(state_a);
    ApplyDeterministicPaintSequence(state_b);

    const TextureReplayDigest painted_a = TextureReplayHash::compute(state_a.TextureSpaceRef());
    const TextureReplayDigest painted_b = TextureReplayHash::compute(state_b.TextureSpaceRef());

    assert(painted_a.combined != before_paint_a.combined);
    AssertDigestEqual(painted_a, painted_b);

    return 0;
}
