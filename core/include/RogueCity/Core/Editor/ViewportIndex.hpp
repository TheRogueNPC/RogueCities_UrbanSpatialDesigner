// FILE: ViewportIndex.hpp
// PURPOSE: Flat, cache-friendly viewport index for selection/inspection.
#pragma once

#include <array>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace RogueCity::Core::Editor {

struct StableID;

enum class VpEntityKind : uint8_t {
    Unknown = 0,
    Axiom,
    Road,
    District,
    Lot,
    Building,
    Water,
    Block
};

inline constexpr uint32_t kViewportIndexInvalid = 0xFFFFFFFFu;

struct VpProbeData {
    VpEntityKind kind{ VpEntityKind::Unknown };
    uint32_t id{ 0 };
    uint64_t stable_id{ 0 };
    uint32_t parent{ kViewportIndexInvalid };
    uint32_t first_child{ 0 };
    uint16_t child_count{ 0 };
    uint16_t _pad{ 0 };
    uint32_t axiom_id{ 0 };
    uint32_t road_id{ 0 };
    uint32_t district_id{ 0 };
    uint32_t block_id{ 0 };
    float slope{ 0.0f };
    float flood{ 0.0f };
    std::array<float, 4> aesp{ {0.0f, 0.0f, 0.0f, 0.0f} };
    uint8_t frontage_family{ 0 };
    uint8_t afg_preset{ 0 };
    uint8_t road_hierarchy{ 0 };
    uint8_t zone_mask{ 0 };
    uint8_t layer_id{ 0 };
    uint8_t _pad2[3]{ 0, 0, 0 };
    float afg_band{ 0.0f };
    char label[32]{};
};

inline void SetViewportLabel(VpProbeData& probe, const char* text) {
    if (!text) {
        probe.label[0] = '\0';
        return;
    }
    std::snprintf(probe.label, sizeof(probe.label), "%s", text);
}

[[nodiscard]] uint64_t GetProbeStableID(const VpProbeData& probe);
void RebuildStableIDMapping(std::vector<VpProbeData>& probes);

} // namespace RogueCity::Core::Editor
