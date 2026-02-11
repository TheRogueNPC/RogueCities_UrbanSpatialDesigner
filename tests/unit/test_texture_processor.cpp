#include "RogueCity/Core/Data/Texture2D.hpp"
#include "RogueCity/Core/Texture/TextureProcessor.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

using RogueCity::Core::Data::Texture2D;
using RogueCity::Core::Texture::TextureProcessor;

namespace {
    [[nodiscard]] float variance(const Texture2D<float>& tex) {
        const size_t n = tex.size();
        if (n == 0) {
            return 0.0f;
        }

        double sum = 0.0;
        for (float v : tex.data()) {
            sum += static_cast<double>(v);
        }
        const double mean = sum / static_cast<double>(n);

        double accum = 0.0;
        for (float v : tex.data()) {
            const double d = static_cast<double>(v) - mean;
            accum += d * d;
        }
        return static_cast<float>(accum / static_cast<double>(n));
    }
}

int main() {
    Texture2D<float> impulse(7, 7, 0.0f);
    impulse.at(3, 3) = 1.0f;
    const float var_before = variance(impulse);
    TextureProcessor::gaussianBlur(impulse, 5);
    const float var_after = variance(impulse);
    assert(var_after < var_before);

    const auto minmax = std::minmax_element(impulse.data().begin(), impulse.data().end());
    assert(*minmax.first >= -1e-6f);
    assert(*minmax.second <= 1.0f + 1e-6f);

    Texture2D<float> ramp(8, 8, 0.0f);
    for (int y = 0; y < ramp.height(); ++y) {
        for (int x = 0; x < ramp.width(); ++x) {
            ramp.at(x, y) = static_cast<float>(x);
        }
    }

    const auto gradient = TextureProcessor::computeGradient(ramp);
    for (int y = 1; y < ramp.height() - 1; ++y) {
        for (int x = 1; x < ramp.width() - 1; ++x) {
            const auto& g = gradient.at(x, y);
            assert(std::abs(g.x - 1.0) < 1e-5);
            assert(std::abs(g.y) < 1e-5);
        }
    }

    return 0;
}

