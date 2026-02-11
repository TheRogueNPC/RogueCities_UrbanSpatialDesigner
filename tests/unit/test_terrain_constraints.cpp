#include "RogueCity/Generators/Pipeline/TerrainConstraintGenerator.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

using RogueCity::Core::WorldConstraintField;
using RogueCity::Generators::TerrainConstraintGenerator;

namespace {
    constexpr double kRadToDeg = 57.2957795130823208768;

    [[nodiscard]] float sampleHeightClamped(const WorldConstraintField& field, int x, int y) {
        const int cx = std::clamp(x, 0, field.width - 1);
        const int cy = std::clamp(y, 0, field.height - 1);
        const size_t idx = static_cast<size_t>(field.toIndex(cx, cy));
        return field.height_meters[idx];
    }

    [[nodiscard]] float recomputeSlope(const WorldConstraintField& field, int x, int y) {
        const float h_l = sampleHeightClamped(field, x - 1, y);
        const float h_r = sampleHeightClamped(field, x + 1, y);
        const float h_d = sampleHeightClamped(field, x, y - 1);
        const float h_u = sampleHeightClamped(field, x, y + 1);

        const double dzdx = (static_cast<double>(h_r) - static_cast<double>(h_l)) / (2.0 * field.cell_size);
        const double dzdy = (static_cast<double>(h_u) - static_cast<double>(h_d)) / (2.0 * field.cell_size);
        const double grad = std::sqrt(dzdx * dzdx + dzdy * dzdy);
        return static_cast<float>(std::atan(grad) * kRadToDeg);
    }
}

int main() {
    TerrainConstraintGenerator generator{};
    TerrainConstraintGenerator::Input input{};
    input.world_width = 640;
    input.world_height = 640;
    input.cell_size = 10.0;
    input.seed = 1337u;

    TerrainConstraintGenerator::Config config{};
    config.erosion_iterations = 3;

    const auto a = generator.generate(input, config);
    const auto b = generator.generate(input, config);
    assert(a.constraints.isValid());
    assert(b.constraints.isValid());
    assert(a.constraints.height_meters == b.constraints.height_meters);
    assert(a.constraints.slope_degrees == b.constraints.slope_degrees);
    assert(a.constraints.no_build_mask == b.constraints.no_build_mask);

    const auto [min_it, max_it] = std::minmax_element(
        a.constraints.height_meters.begin(),
        a.constraints.height_meters.end());
    assert(max_it != a.constraints.height_meters.end());
    assert(min_it != a.constraints.height_meters.end());
    assert((*max_it - *min_it) > 1e-3f);

    TerrainConstraintGenerator::Input changed_seed = input;
    changed_seed.seed = input.seed + 1u;
    const auto c = generator.generate(changed_seed, config);
    assert(c.constraints.isValid());

    bool any_height_delta = false;
    for (size_t i = 0; i < a.constraints.height_meters.size(); ++i) {
        if (std::abs(a.constraints.height_meters[i] - c.constraints.height_meters[i]) > 1e-5f) {
            any_height_delta = true;
            break;
        }
    }
    assert(any_height_delta);

    const int step_x = std::max(1, a.constraints.width / 8);
    const int step_y = std::max(1, a.constraints.height / 8);
    for (int y = 0; y < a.constraints.height; y += step_y) {
        for (int x = 0; x < a.constraints.width; x += step_x) {
            const float expected = std::clamp(recomputeSlope(a.constraints, x, y), 0.0f, 89.0f);
            const float actual = a.constraints.slope_degrees[static_cast<size_t>(a.constraints.toIndex(x, y))];
            assert(std::abs(expected - actual) < 1e-4f);
        }
    }

    TerrainConstraintGenerator::Config no_erosion = config;
    no_erosion.erosion_iterations = 0;
    const auto raw = generator.generate(input, no_erosion);
    assert(raw.constraints.isValid());

    bool any_erosion_delta = false;
    for (size_t i = 0; i < a.constraints.height_meters.size(); ++i) {
        if (std::abs(a.constraints.height_meters[i] - raw.constraints.height_meters[i]) > 1e-5f) {
            any_erosion_delta = true;
            break;
        }
    }
    assert(any_erosion_delta);

    return 0;
}
