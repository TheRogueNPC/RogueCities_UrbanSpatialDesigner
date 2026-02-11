#include "RogueCity/Core/Data/TextureSpace.hpp"
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"

#include <cassert>
#include <cmath>

using RogueCity::Core::Bounds;
using RogueCity::Core::Tensor2D;
using RogueCity::Core::Vec2;
using RogueCity::Core::Data::TextureSpace;
using RogueCity::Generators::TensorFieldGenerator;

namespace {
    [[nodiscard]] double cosineAbs(const Vec2& a, const Vec2& b) {
        const double la = a.length();
        const double lb = b.length();
        if (la <= 1e-9 || lb <= 1e-9) {
            return 0.0;
        }
        return std::abs((a.dot(b)) / (la * lb));
    }
}

int main() {
    TensorFieldGenerator::Config field_cfg{};
    field_cfg.width = 40;
    field_cfg.height = 40;
    field_cfg.cell_size = 10.0;

    TensorFieldGenerator generator(field_cfg);
    generator.addLinearField(Vec2(200.0, 200.0), 2000.0, 0.0, 2.0);
    generator.generateField();

    Bounds bounds{};
    bounds.min = Vec2(0.0, 0.0);
    bounds.max = Vec2(
        static_cast<double>(field_cfg.width) * field_cfg.cell_size,
        static_cast<double>(field_cfg.height) * field_cfg.cell_size);

    TextureSpace texture_space(bounds, 64);
    generator.writeToTextureSpace(texture_space);

    const auto& tensor = texture_space.tensorLayer();
    assert(!tensor.empty());

    size_t non_zero_samples = 0;
    for (int y = 0; y < tensor.height(); y += 4) {
        for (int x = 0; x < tensor.width(); x += 4) {
            if (tensor.at(x, y).lengthSquared() > 1e-7) {
                ++non_zero_samples;
            }
        }
    }
    assert(non_zero_samples > 0);

    const Vec2 sample_world(175.0, 225.0);
    const Tensor2D pre_bind = generator.sampleTensor(sample_world);
    const Vec2 pre_dir = pre_bind.majorEigenvector();
    const Vec2 sample_uv = texture_space.coordinateSystem().worldToUV(sample_world);
    const Vec2 texture_dir = texture_space.tensorLayer().sampleBilinearTyped<Vec2>(sample_uv);
    assert(cosineAbs(pre_dir, texture_dir) > 0.95);

    generator.bindTextureSpace(&texture_space);
    assert(generator.usesTextureSpace());
    const Tensor2D post_bind = generator.sampleTensor(sample_world);
    const Vec2 post_dir = post_bind.majorEigenvector();

    assert(cosineAbs(pre_dir, post_dir) > 0.95);
    assert(generator.lastSampleUsedTexture());
    assert(!generator.lastSampleUsedFallback());

    generator.clearTextureSpaceBinding();
    assert(!generator.usesTextureSpace());

    return 0;
}
