#include "RogueCity/Core/Texture/TextureProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace RogueCity::Core::Texture {

    namespace {
        [[nodiscard]] int ClampIndex(int idx, int limit) {
            return std::clamp(idx, 0, limit - 1);
        }
    } // namespace

    void TextureProcessor::boxBlur(Data::Texture2D<float>& texture, int radius) {
        if (radius <= 0 || texture.empty()) {
            return;
        }

        if (radius == 1) {
            Data::Texture2D<float> destination{};
            SIMDProcessor::boxBlur3x3(texture, destination);
            texture = std::move(destination);
            return;
        }

        Data::Texture2D<float> source = texture;
        const int width = source.width();
        const int height = source.height();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum = 0.0f;
                int count = 0;
                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        const int sx = ClampIndex(x + kx, width);
                        const int sy = ClampIndex(y + ky, height);
                        sum += source.at(sx, sy);
                        ++count;
                    }
                }
                texture.at(x, y) = (count > 0) ? (sum / static_cast<float>(count)) : source.at(x, y);
            }
        }
    }

    void TextureProcessor::gaussianBlur(Data::Texture2D<float>& texture, int kernel_size) {
        if (texture.empty()) {
            return;
        }

        int size = std::max(3, kernel_size);
        if ((size % 2) == 0) {
            ++size;
        }
        const int radius = size / 2;
        const std::vector<float> kernel = buildGaussianKernel(size);

        const int width = texture.width();
        const int height = texture.height();
        Data::Texture2D<float> source = texture;
        Data::Texture2D<float> horizontal(width, height, 0.0f);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    const int sx = ClampIndex(x + k, width);
                    const float weight = kernel[static_cast<size_t>(k + radius)];
                    sum += source.at(sx, y) * weight;
                }
                horizontal.at(x, y) = sum;
            }
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    const int sy = ClampIndex(y + k, height);
                    const float weight = kernel[static_cast<size_t>(k + radius)];
                    sum += horizontal.at(x, sy) * weight;
                }
                texture.at(x, y) = sum;
            }
        }
    }

    Data::Texture2D<Vec2> TextureProcessor::computeGradient(const Data::Texture2D<float>& texture) {
        const int width = texture.width();
        const int height = texture.height();
        Data::Texture2D<Vec2> gradient(width, height, Vec2{});

        if (texture.empty()) {
            return gradient;
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int xl = ClampIndex(x - 1, width);
                const int xr = ClampIndex(x + 1, width);
                const int yd = ClampIndex(y - 1, height);
                const int yu = ClampIndex(y + 1, height);

                const float dx = 0.5f * (texture.at(xr, y) - texture.at(xl, y));
                const float dy = 0.5f * (texture.at(x, yu) - texture.at(x, yd));
                gradient.at(x, y) = Vec2(static_cast<double>(dx), static_cast<double>(dy));
            }
        }

        return gradient;
    }

    std::vector<float> TextureProcessor::buildGaussianKernel(int kernel_size) {
        const int radius = kernel_size / 2;
        const double sigma = std::max(1.0, static_cast<double>(kernel_size) / 3.0);
        const double two_sigma_sq = 2.0 * sigma * sigma;

        std::vector<float> kernel(static_cast<size_t>(kernel_size), 0.0f);
        double sum = 0.0;
        for (int i = -radius; i <= radius; ++i) {
            const double exponent = -static_cast<double>(i * i) / two_sigma_sq;
            const double weight = std::exp(exponent);
            kernel[static_cast<size_t>(i + radius)] = static_cast<float>(weight);
            sum += weight;
        }

        if (sum > 0.0) {
            for (float& value : kernel) {
                value = static_cast<float>(static_cast<double>(value) / sum);
            }
        }
        return kernel;
    }

} // namespace RogueCity::Core::Texture
