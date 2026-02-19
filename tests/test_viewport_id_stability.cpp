#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/StableIDRegistry.hpp"

#include <cassert>
#include <string>
#include <unordered_map>

namespace {

using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Editor::GetStableIDRegistry;
using RogueCity::Core::Editor::VpEntityKind;

uint64_t Key(VpEntityKind kind, uint32_t id) {
    return (static_cast<uint64_t>(static_cast<uint8_t>(kind)) << 32ull) | id;
}

} // namespace

int main() {
    auto& registry = GetStableIDRegistry();
    registry.Clear();

    GlobalState gs{};

    RogueCity::Core::District district{};
    district.id = 7u;
    district.border = {
        RogueCity::Core::Vec2(100.0, 100.0),
        RogueCity::Core::Vec2(250.0, 100.0),
        RogueCity::Core::Vec2(250.0, 250.0),
        RogueCity::Core::Vec2(100.0, 250.0)
    };
    gs.districts.add(district);

    RogueCity::Core::LotToken lot{};
    lot.id = 22u;
    lot.district_id = district.id;
    lot.centroid = RogueCity::Core::Vec2(170.0, 170.0);
    lot.boundary = district.border;
    gs.lots.add(lot);

    RogueCity::Core::Road road{};
    road.id = 5u;
    road.points = {
        RogueCity::Core::Vec2(0.0, 0.0),
        RogueCity::Core::Vec2(100.0, 20.0)
    };
    gs.roads.add(road);

    RogueCity::App::ViewportIndexBuilder::Build(gs);

    std::unordered_map<uint64_t, uint64_t> stable_before;
    for (const auto& probe : gs.viewport_index) {
        if (probe.stable_id != 0u) {
            stable_before[Key(probe.kind, probe.id)] = probe.stable_id;
        }
    }
    assert(!stable_before.empty());

    // Add unrelated entity and rebuild: existing stable IDs should remain unchanged.
    RogueCity::Core::Road road2{};
    road2.id = 55u;
    road2.points = {
        RogueCity::Core::Vec2(20.0, 20.0),
        RogueCity::Core::Vec2(180.0, 35.0)
    };
    gs.roads.add(road2);
    RogueCity::App::ViewportIndexBuilder::Build(gs);

    for (const auto& probe : gs.viewport_index) {
        const auto it = stable_before.find(Key(probe.kind, probe.id));
        if (it != stable_before.end()) {
            assert(probe.stable_id == it->second);
        }
    }

    // Serialization round-trip should preserve mapping.
    const std::string snapshot = registry.Serialize();
    registry.Clear();
    assert(registry.Deserialize(snapshot));

    for (const auto& [entity_key, stable_id] : stable_before) {
        const auto kind = static_cast<VpEntityKind>(static_cast<uint8_t>(entity_key >> 32ull));
        const auto id = static_cast<uint32_t>(entity_key & 0xFFFFFFFFu);
        const auto restored = registry.GetStableID(kind, id);
        assert(restored.has_value());
        assert(restored->id == stable_id);
    }

    registry.Clear();
    return 0;
}
