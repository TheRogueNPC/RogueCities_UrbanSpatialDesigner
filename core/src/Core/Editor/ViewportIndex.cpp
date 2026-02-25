/**
 * @file ViewportIndex.cpp
 * @brief Implements functions for managing stable IDs for viewport probe entities in the editor.
 *
 * Contains logic to determine if a probe is mappable, retrieve its stable ID, and rebuild the stable ID mapping
 * for a collection of probes. Utilizes the StableIDRegistry to ensure consistent entity identification across sessions.
 *
 * Functions:
 * - IsMappableProbe: Checks if a probe can be mapped based on its kind.
 * - GetProbeStableID: Retrieves the stable ID for a probe if it is mappable.
 * - RebuildStableIDMapping: Updates the stable ID mapping for a vector of probes and assigns stable IDs accordingly.
 */
 
#include "RogueCity/Core/Editor/ViewportIndex.hpp"

#include "RogueCity/Core/Editor/StableIDRegistry.hpp"

namespace RogueCity::Core::Editor {

namespace {

[[nodiscard]] bool IsMappableProbe(const VpProbeData& probe) {
    return probe.kind != VpEntityKind::Unknown;
}

} // namespace

uint64_t GetProbeStableID(const VpProbeData& probe) {
    if (!IsMappableProbe(probe)) {
        return 0u;
    }
    if (const auto stable = GetStableIDRegistry().GetStableID(probe.kind, probe.id); stable.has_value()) {
        return stable->id;
    }
    return 0u;
}

void RebuildStableIDMapping(std::vector<VpProbeData>& probes) {
    std::vector<ViewportEntityID> active_ids;
    active_ids.reserve(probes.size());
    for (const auto& probe : probes) {
        if (!IsMappableProbe(probe)) {
            continue;
        }
        active_ids.push_back(ViewportEntityID{ probe.kind, probe.id });
    }

    auto& registry = GetStableIDRegistry();
    registry.RebuildMapping(active_ids);

    for (auto& probe : probes) {
        if (!IsMappableProbe(probe)) {
            probe.stable_id = 0u;
            continue;
        }

        if (const auto stable = registry.GetStableID(probe.kind, probe.id); stable.has_value()) {
            probe.stable_id = stable->id;
        } else {
            probe.stable_id = 0u;
        }
    }
}

} // namespace RogueCity::Core::Editor
