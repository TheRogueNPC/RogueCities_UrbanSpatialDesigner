#pragma once

#include "RogueCity/Core/Data/Texture2D.hpp"
#include "RogueCity/Core/Texture/SIMDProcessor.hpp"

#include <vector>

namespace RogueCity::Core::Texture {

    class TextureProcessor {
    public:
        static void boxBlur(Data::Texture2D<float>& texture, int radius);
        static void gaussianBlur(Data::Texture2D<float>& texture, int kernel_size);
        [[nodiscard]] static Data::Texture2D<Vec2> computeGradient(const Data::Texture2D<float>& texture);

    private:
        [[nodiscard]] static std::vector<float> buildGaussianKernel(int kernel_size);
    };

} // namespace RogueCity::Core::Texture
