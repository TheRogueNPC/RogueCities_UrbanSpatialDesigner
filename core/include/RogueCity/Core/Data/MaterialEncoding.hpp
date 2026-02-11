#pragma once

#include <cstdint>

namespace RogueCity::Core::Data {

    // Material layer bit layout contract:
    // bits [0..1] = flood mask (0 none, 1 minor, 2 severe, 3 reserved)
    // bit  [7]    = no-build hard constraint
    constexpr uint8_t kMaterialFloodMaskBits = 0x03u;
    constexpr uint8_t kMaterialNoBuildBit = 0x80u;

    [[nodiscard]] constexpr uint8_t EncodeMaterialSample(uint8_t flood_mask, bool no_build) {
        return static_cast<uint8_t>((flood_mask & kMaterialFloodMaskBits) | (no_build ? kMaterialNoBuildBit : 0u));
    }

    [[nodiscard]] constexpr uint8_t DecodeMaterialFloodMask(uint8_t material) {
        return static_cast<uint8_t>(material & kMaterialFloodMaskBits);
    }

    [[nodiscard]] constexpr bool DecodeMaterialNoBuild(uint8_t material) {
        return (material & kMaterialNoBuildBit) != 0u;
    }

} // namespace RogueCity::Core::Data
