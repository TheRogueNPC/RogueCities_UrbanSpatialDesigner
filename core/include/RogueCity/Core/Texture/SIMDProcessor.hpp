#pragma once

#include "RogueCity/Core/Data/Texture2D.hpp"

#include <cstddef>
#include <cstdint>

namespace RogueCity::Core::Texture {

    class SIMDProcessor {
    public:
        struct CPUFeatures {
            bool sse2{ false };
            bool avx{ false };
            bool avx2{ false };
        };

        [[nodiscard]] static const CPUFeatures& cpuFeatures();
        [[nodiscard]] static bool hasAVX2();

        static void boxBlur3x3(
            const Data::Texture2D<float>& source,
            Data::Texture2D<float>& destination);

        static void downsample2x(
            const Data::Texture2D<float>& source,
            Data::Texture2D<float>& destination);

        static void floatToUint8(
            const float* source,
            uint8_t* destination,
            size_t count,
            float min_value,
            float max_value);
    };

} // namespace RogueCity::Core::Texture
