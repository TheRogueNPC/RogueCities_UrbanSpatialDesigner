#include "RogueCity/Core/Editor/StableIDRegistry.hpp"

#include <cassert>
#include <vector>

int main() {
    using RogueCity::Core::Editor::GetProbeStableID;
    using RogueCity::Core::Editor::GetStableIDRegistry;
    using RogueCity::Core::Editor::RebuildStableIDMapping;
    using RogueCity::Core::Editor::StableIDRegistry;
    using RogueCity::Core::Editor::VpEntityKind;
    using RogueCity::Core::Editor::VpProbeData;
    using RogueCity::Core::Editor::ViewportEntityID;

    StableIDRegistry& registry = GetStableIDRegistry();
    registry.Clear();

    const auto s0 = registry.AllocateStableID(VpEntityKind::Road, 42u);
    const auto s1 = registry.AllocateStableID(VpEntityKind::Road, 42u);
    const auto s2 = registry.AllocateStableID(VpEntityKind::Road, 43u);
    assert(s0 == s1);
    assert(s0 != s2);

    const auto roundtrip = registry.GetViewportID(s0);
    assert(roundtrip.has_value());
    assert(roundtrip->kind == VpEntityKind::Road);
    assert(roundtrip->id == 42u);

    std::vector<ViewportEntityID> base_ids{
        { VpEntityKind::Road, 1u },
        { VpEntityKind::District, 2u }
    };
    registry.RebuildMapping(base_ids);

    const auto base_road = registry.GetStableID(VpEntityKind::Road, 1u);
    assert(base_road.has_value());

    std::vector<ViewportEntityID> expanded_ids{
        { VpEntityKind::Road, 1u },
        { VpEntityKind::District, 2u },
        { VpEntityKind::Road, 3u }
    };
    registry.RebuildMapping(expanded_ids);
    const auto road_after_expand = registry.GetStableID(VpEntityKind::Road, 1u);
    assert(road_after_expand.has_value());
    assert(*road_after_expand == *base_road);

    std::vector<ViewportEntityID> reduced_ids{
        { VpEntityKind::Road, 1u },
        { VpEntityKind::Road, 3u }
    };
    registry.RebuildMapping(reduced_ids);
    assert(!registry.GetStableID(VpEntityKind::District, 2u).has_value());

    const std::string serialized = registry.Serialize();
    StableIDRegistry restored{};
    assert(restored.Deserialize(serialized));
    const auto restored_road = restored.GetStableID(VpEntityKind::Road, 1u);
    assert(restored_road.has_value());
    assert(*restored_road == *registry.GetStableID(VpEntityKind::Road, 1u));

    std::vector<VpProbeData> probes;
    VpProbeData road_probe{};
    road_probe.kind = VpEntityKind::Road;
    road_probe.id = 77u;
    probes.push_back(road_probe);

    VpProbeData district_probe{};
    district_probe.kind = VpEntityKind::District;
    district_probe.id = 88u;
    probes.push_back(district_probe);

    RebuildStableIDMapping(probes);
    assert(probes[0].stable_id != 0u);
    assert(probes[1].stable_id != 0u);

    const auto stable_probe = GetProbeStableID(probes[0]);
    assert(stable_probe != 0u);
    assert(stable_probe == probes[0].stable_id);

    registry.Clear();
    return 0;
}
