#include "RogueCity/Core/Data/Texture2D.hpp"
#include "RogueCity/Core/Texture/SIMDProcessor.hpp"
#include "RogueCity/Core/Texture/TextureProcessor.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

using RogueCity::Core::Data::Texture2D;
using RogueCity::Core::Texture::SIMDProcessor;
using RogueCity::Core::Texture::TextureProcessor;

namespace {
    [[nodiscard]] bool nearlyEqual(float a, float b, float eps = 1e-5f) {
        return std::abs(a - b) <= eps;
    }
}

int main() {
    Texture2D<float> source(12, 10, 0.0f);
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x) {
            source.at(x, y) = static_cast<float>((x * 2) + (y * 3));
        }
    }

    Texture2D<float> simd_blur{};
    SIMDProcessor::boxBlur3x3(source, simd_blur);

    Texture2D<float> reference = source;
    TextureProcessor::boxBlur(reference, 1);
    assert(simd_blur.width() == reference.width());
    assert(simd_blur.height() == reference.height());
    for (size_t i = 0; i < reference.size(); ++i) {
        assert(nearlyEqual(simd_blur.data()[i], reference.data()[i]));
    }

    Texture2D<float> four_by_four(4, 4, 0.0f);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            four_by_four.at(x, y) = static_cast<float>((y * 4) + x);
        }
    }
    Texture2D<float> downsampled{};
    SIMDProcessor::downsample2x(four_by_four, downsampled);
    assert(downsampled.width() == 2);
    assert(downsampled.height() == 2);
    assert(nearlyEqual(downsampled.at(0, 0), 2.5f));
    assert(nearlyEqual(downsampled.at(1, 0), 4.5f));
    assert(nearlyEqual(downsampled.at(0, 1), 10.5f));
    assert(nearlyEqual(downsampled.at(1, 1), 12.5f));

    const std::vector<float> samples{ -1.0f, 0.0f, 0.5f, 1.0f, 2.0f };
    std::vector<uint8_t> packed(samples.size(), 0u);
    SIMDProcessor::floatToUint8(samples.data(), packed.data(), samples.size(), 0.0f, 1.0f);
    assert(packed[0] == 0u);
    assert(packed[1] == 0u);
    assert(packed[2] == 128u);
    assert(packed[3] == 255u);
    assert(packed[4] == 255u);

    return 0;
}

