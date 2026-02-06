#pragma once

#include <cstdint>

namespace LInput {

namespace LLUtils {

[[nodiscard]] inline constexpr uint32_t FourCC(char a, char b, char c, char d) {
    return (static_cast<uint32_t>(a) << 0u) |
           (static_cast<uint32_t>(b) << 8u) |
           (static_cast<uint32_t>(c) << 16u) |
           (static_cast<uint32_t>(d) << 24u);
}

} // namespace LLUtils

} // namespace LInput

