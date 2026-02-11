#include "RogueCity/Core/Data/CoordinateSystem.hpp"
#include "RogueCity/Core/Data/Texture2D.hpp"
#include "RogueCity/Core/Data/TextureSpace.hpp"

#include <cassert>
#include <cmath>

using RogueCity::Core::Bounds;
using RogueCity::Core::Vec2;
using RogueCity::Core::Data::CoordinateSystem;
using RogueCity::Core::Data::Texture2D;
using RogueCity::Core::Data::TextureLayer;
using RogueCity::Core::Data::TextureSpace;

namespace {
    [[nodiscard]] bool nearlyEqual(double a, double b, double epsilon = 1e-6) {
        return std::abs(a - b) <= epsilon;
    }
}

int main() {
    Bounds bounds{};
    bounds.min = Vec2(10.0, 20.0);
    bounds.max = Vec2(110.0, 220.0);

    CoordinateSystem coords(bounds, 128);
    const Vec2 world(65.0, 130.0);
    const Vec2 uv = coords.worldToUV(world);
    const Vec2 roundtrip = coords.uvToWorld(uv);
    assert(nearlyEqual(world.x, roundtrip.x, 1e-9));
    assert(nearlyEqual(world.y, roundtrip.y, 1e-9));

    const auto p0 = coords.worldToPixel(Vec2(-500.0, -500.0));
    const auto p1 = coords.worldToPixel(Vec2(9999.0, 9999.0));
    assert(p0.x == 0 && p0.y == 0);
    assert(p1.x == 127 && p1.y == 127);
    assert(coords.isInBounds(world));
    assert(!coords.isInBounds(Vec2(1000.0, 1000.0)));

    Texture2D<float> pattern(2, 2, 0.0f);
    pattern.at(0, 0) = 0.0f;
    pattern.at(1, 0) = 10.0f;
    pattern.at(0, 1) = 20.0f;
    pattern.at(1, 1) = 30.0f;
    const float bilinear_center = pattern.sampleBilinear(Vec2(0.5, 0.5));
    assert(nearlyEqual(bilinear_center, 15.0f, 1e-6));

    TextureSpace texture_space(bounds, 64);
    assert(texture_space.isDirty(TextureLayer::Height));
    texture_space.clearAllDirty();
    assert(!texture_space.isDirty(TextureLayer::Height));
    texture_space.markDirty(TextureLayer::Tensor);
    assert(texture_space.isDirty(TextureLayer::Tensor));

    return 0;
}

