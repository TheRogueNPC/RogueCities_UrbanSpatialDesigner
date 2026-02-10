#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <cstring>
#include <memory>

using RogueCity::App::CommandHistory;
using RogueCity::App::ICommand;
using RogueCity::App::ViewportIndexBuilder;
using RogueCity::Core::Editor::DirtyLayer;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Generators::CityGenerator;

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

void TestViewportIndexIntegrity() {
    CityGenerator generator{};
    CityGenerator::Config config{};
    config.width = 2000;
    config.height = 2000;
    config.cell_size = 10.0;
    config.num_seeds = 18;
    config.seed = 20260210;

    std::vector<CityGenerator::AxiomInput> axioms;
    {
        CityGenerator::AxiomInput radial{};
        radial.type = CityGenerator::AxiomInput::Type::Radial;
        radial.position = RogueCity::Core::Vec2(1000.0, 1000.0);
        radial.radius = 450.0;
        axioms.push_back(radial);
    }
    {
        CityGenerator::AxiomInput grid{};
        grid.type = CityGenerator::AxiomInput::Type::Grid;
        grid.position = RogueCity::Core::Vec2(700.0, 1300.0);
        grid.radius = 320.0;
        grid.theta = 0.35;
        axioms.push_back(grid);
    }

    const auto out_a = generator.generate(axioms, config);
    const auto out_b = generator.generate(axioms, config);

    GlobalState gs_a{};
    GlobalState gs_b{};
    PopulateGlobalState(out_a, gs_a);
    PopulateGlobalState(out_b, gs_b);
    ViewportIndexBuilder::Build(gs_a);
    ViewportIndexBuilder::Build(gs_b);

    assert(gs_a.viewport_index.size() == gs_b.viewport_index.size());
    for (size_t i = 0; i < gs_a.viewport_index.size(); ++i) {
        const auto& a = gs_a.viewport_index[i];
        const auto& b = gs_b.viewport_index[i];
        assert(a.kind == b.kind);
        assert(a.id == b.id);
        assert(a.parent == b.parent);
        assert(a.first_child == b.first_child);
        assert(a.child_count == b.child_count);
    }

    for (size_t i = 0; i < gs_a.viewport_index.size(); ++i) {
        const auto& probe = gs_a.viewport_index[i];
        if (probe.child_count == 0) {
            continue;
        }
        const uint32_t first = probe.first_child;
        const uint32_t last = first + static_cast<uint32_t>(probe.child_count);
        assert(last <= gs_a.viewport_index.size());
        for (uint32_t child = first; child < last; ++child) {
            assert(gs_a.viewport_index[child].parent == i);
        }
    }
}

void TestDirtyLayerPropagation() {
    GlobalState gs{};
    gs.dirty_layers.MarkAllClean();
    assert(!gs.dirty_layers.AnyDirty());

    gs.dirty_layers.MarkFromAxiomEdit();
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Axioms));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Tensor));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Roads));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Districts));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Lots));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::Buildings));
    assert(gs.dirty_layers.IsDirty(DirtyLayer::ViewportIndex));

    gs.dirty_layers.MarkAllClean();
    assert(!gs.dirty_layers.AnyDirty());
}

void TestUndoRedoDeterminism() {
    struct BlobState {
        uint32_t id{ 7 };
        float weight{ 1.25f };
        char name[16]{ "baseline" };
    };

    struct BlobCommand final : ICommand {
        BlobState* state{ nullptr };
        BlobState before{};
        BlobState after{};

        void Execute() override { *state = after; }
        void Undo() override { *state = before; }
        const char* GetDescription() const override { return "Blob Mutation"; }
    };

    BlobState state{};
    const BlobState baseline = state;

    BlobState mutated{};
    mutated.id = 42;
    mutated.weight = 9.5f;
    strcpy_s(mutated.name, sizeof(mutated.name), "mutated");

    CommandHistory history{};
    auto cmd = std::make_unique<BlobCommand>();
    cmd->state = &state;
    cmd->before = state;
    cmd->after = mutated;

    history.Execute(std::move(cmd));
    assert(state.id == mutated.id);
    assert(state.weight == mutated.weight);

    history.Undo();
    assert(std::memcmp(&state, &baseline, sizeof(BlobState)) == 0);

    history.Redo();
    assert(state.id == mutated.id);
    assert(state.weight == mutated.weight);
}

} // namespace

int main() {
    TestViewportIndexIntegrity();
    TestDirtyLayerPropagation();
    TestUndoRedoDeterminism();
    return 0;
}
