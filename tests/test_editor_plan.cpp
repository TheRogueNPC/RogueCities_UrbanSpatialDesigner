#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cassert>
#include <array>
#include <cstring>
#include <memory>
#include <span>

using RogueCity::App::CommandHistory;
using RogueCity::App::ICommand;
using RogueCity::App::ViewportIndexBuilder;
using RogueCity::App::EditorManipulation::ApplyRotate;
using RogueCity::App::EditorManipulation::ApplyScale;
using RogueCity::App::EditorManipulation::ApplyTranslate;
using RogueCity::App::EditorManipulation::BuildCatmullRomSpline;
using RogueCity::App::EditorManipulation::SplineOptions;
using RogueCity::Core::Editor::DirtyLayer;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::SelectionItem;
using RogueCity::Core::Editor::ValidationSeverity;
using RogueCity::Core::Editor::VpEntityKind;
using RogueCity::Core::Validation::CollectOverlayValidationErrors;
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
        assert(a.stable_id == b.stable_id);
        assert(a.parent == b.parent);
        assert(a.first_child == b.first_child);
        assert(a.child_count == b.child_count);
        assert(a.layer_id == b.layer_id);
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

void TestLayerVisibilityMapping() {
    GlobalState gs{};
    RogueCity::Core::Road road{};
    road.id = 100u;
    road.points = {
        RogueCity::Core::Vec2(0.0, 0.0),
        RogueCity::Core::Vec2(10.0, 0.0)
    };
    gs.roads.add(road);

    gs.SetEntityLayer(VpEntityKind::Road, 100u, 2u);
    assert(gs.GetEntityLayer(VpEntityKind::Road, 100u) == 2u);
    assert(gs.IsEntityVisible(VpEntityKind::Road, 100u));

    auto* layer = gs.layer_manager.layers.size() > 2 ? &gs.layer_manager.layers[2] : nullptr;
    assert(layer != nullptr);
    layer->visible = false;
    assert(!gs.IsEntityVisible(VpEntityKind::Road, 100u));
}

void TestSplineDeterminism() {
    std::vector<RogueCity::Core::Vec2> control{
        RogueCity::Core::Vec2(0.0, 0.0),
        RogueCity::Core::Vec2(20.0, 5.0),
        RogueCity::Core::Vec2(40.0, -8.0),
        RogueCity::Core::Vec2(70.0, 0.0)
    };

    SplineOptions options{};
    options.closed = false;
    options.samples_per_segment = 7;
    options.tension = 0.25f;

    const auto a = BuildCatmullRomSpline(control, options);
    const auto b = BuildCatmullRomSpline(control, options);
    assert(a.size() == b.size());
    assert(!a.empty());
    for (size_t i = 0; i < a.size(); ++i) {
        assert(a[i] == b[i]);
    }
}

void TestValidationCollector() {
    GlobalState gs{};

    RogueCity::Core::Road r1{};
    r1.id = 1u;
    r1.points = {
        RogueCity::Core::Vec2(0.0, 0.0),
        RogueCity::Core::Vec2(10.0, 10.0)
    };
    gs.roads.add(r1);

    RogueCity::Core::Road r2{};
    r2.id = 2u;
    r2.points = {
        RogueCity::Core::Vec2(10.0, 0.0),
        RogueCity::Core::Vec2(0.0, 10.0)
    };
    gs.roads.add(r2);

    RogueCity::Core::LotToken lot{};
    lot.id = 9u;
    lot.area = 30.0f;
    lot.centroid = RogueCity::Core::Vec2(5.0, 5.0);
    lot.boundary = {
        RogueCity::Core::Vec2(4.0, 4.0),
        RogueCity::Core::Vec2(6.0, 4.0),
        RogueCity::Core::Vec2(6.0, 6.0),
        RogueCity::Core::Vec2(4.0, 6.0)
    };
    gs.lots.add(lot);

    RogueCity::Core::BuildingSite b{};
    b.id = 90u;
    b.lot_id = 999u; // Missing lot link to force critical.
    b.position = RogueCity::Core::Vec2(50.0, 50.0);
    gs.buildings.push_back(b);

    auto errors = CollectOverlayValidationErrors(gs, 80.0f);
    assert(!errors.empty());

    bool saw_critical = false;
    bool saw_lot_warning = false;
    bool saw_road_warning = false;
    for (const auto& e : errors) {
        if (e.severity == ValidationSeverity::Critical && e.entity_kind == VpEntityKind::Building) {
            saw_critical = true;
        }
        if (e.entity_kind == VpEntityKind::Lot) {
            saw_lot_warning = true;
        }
        if (e.entity_kind == VpEntityKind::Road) {
            saw_road_warning = true;
        }
    }

    assert(saw_critical);
    assert(saw_lot_warning);
    assert(saw_road_warning);
}

void TestGizmoTransformRoundTrip() {
    GlobalState gs{};

    RogueCity::Core::Road road{};
    road.id = 7u;
    road.points = {
        RogueCity::Core::Vec2(10.0, 10.0),
        RogueCity::Core::Vec2(20.0, 10.0)
    };
    gs.roads.add(road);

    RogueCity::Core::BuildingSite building{};
    building.id = 42u;
    building.position = RogueCity::Core::Vec2(15.0, 25.0);
    const auto building_sid = gs.buildings.push_back(building);

    const auto baseline_road = road.points;
    const auto baseline_building = gs.buildings[building_sid].position;

    std::array<SelectionItem, 2> selection{
        SelectionItem{ VpEntityKind::Road, 7u },
        SelectionItem{ VpEntityKind::Building, 42u }
    };

    const RogueCity::Core::Vec2 delta(3.0, -2.0);
    const RogueCity::Core::Vec2 pivot(15.0, 15.0);
    assert(ApplyTranslate(gs, std::span<const SelectionItem>(selection), delta));
    assert(ApplyRotate(gs, std::span<const SelectionItem>(selection), pivot, 0.25));
    assert(ApplyScale(gs, std::span<const SelectionItem>(selection), pivot, 1.2));

    assert(ApplyScale(gs, std::span<const SelectionItem>(selection), pivot, 1.0 / 1.2));
    assert(ApplyRotate(gs, std::span<const SelectionItem>(selection), pivot, -0.25));
    assert(ApplyTranslate(gs, std::span<const SelectionItem>(selection), RogueCity::Core::Vec2(-delta.x, -delta.y)));

    RogueCity::Core::Road* restored_road = nullptr;
    for (auto& candidate : gs.roads) {
        if (candidate.id == 7u) {
            restored_road = &candidate;
            break;
        }
    }
    assert(restored_road != nullptr);
    assert(restored_road->points.size() == baseline_road.size());
    for (size_t i = 0; i < baseline_road.size(); ++i) {
        assert(restored_road->points[i].distanceTo(baseline_road[i]) < 1e-4);
    }
    assert(gs.buildings[building_sid].position.distanceTo(baseline_building) < 1e-4);
}

} // namespace

int main() {
    TestViewportIndexIntegrity();
    TestDirtyLayerPropagation();
    TestUndoRedoDeterminism();
    TestLayerVisibilityMapping();
    TestSplineDeterminism();
    TestValidationCollector();
    TestGizmoTransformRoundTrip();
    return 0;
}
