#include "RogueCity/Generators/Pipeline/TerrainConstraintGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace RogueCity::Generators {

namespace {

constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kRadToDeg = 57.2957795130823208768;

[[nodiscard]] float Hash01(int x, int y, uint32_t seed) {
    uint64_t v = static_cast<uint64_t>(static_cast<uint32_t>(x)) * 0x9E3779B185EBCA87ull;
    v ^= static_cast<uint64_t>(static_cast<uint32_t>(y)) * 0xC2B2AE3D27D4EB4Full;
    v ^= static_cast<uint64_t>(seed) * 0x165667B19E3779F9ull;
    v ^= (v >> 33);
    v *= 0xFF51AFD7ED558CCDull;
    v ^= (v >> 33);
    v *= 0xC4CEB9FE1A85EC53ull;
    v ^= (v >> 33);
    return static_cast<float>(v & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}

[[nodiscard]] float FractalNoise(float x, float y, uint32_t seed) {
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    float sum = 0.0f;
    float amp = 0.55f;
    float freq = 1.0f;
    for (int octave = 0; octave < 3; ++octave) {
        const int sx = static_cast<int>(std::floor(static_cast<double>(ix) * freq));
        const int sy = static_cast<int>(std::floor(static_cast<double>(iy) * freq));
        sum += Hash01(sx, sy, seed + static_cast<uint32_t>(octave * 131)) * amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    return std::clamp(sum, 0.0f, 1.0f);
}

[[nodiscard]] float HeightSampleClamped(
    const std::vector<float>& height,
    int width,
    int rows,
    int x,
    int y) {
    const int cx = std::clamp(x, 0, width - 1);
    const int cy = std::clamp(y, 0, rows - 1);
    const size_t idx = static_cast<size_t>(cy * width + cx);
    return height[idx];
}

void ApplyDeterministicErosion(
    std::vector<float>& height,
    int width,
    int rows,
    uint32_t seed,
    int iterations) {
    if (height.empty() || width <= 0 || rows <= 0 || iterations <= 0) {
        return;
    }

    std::vector<float> scratch = height;
    for (int pass = 0; pass < iterations; ++pass) {
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t idx = static_cast<size_t>(y * width + x);
                const float c = HeightSampleClamped(height, width, rows, x, y);
                const float l = HeightSampleClamped(height, width, rows, x - 1, y);
                const float r = HeightSampleClamped(height, width, rows, x + 1, y);
                const float u = HeightSampleClamped(height, width, rows, x, y - 1);
                const float d = HeightSampleClamped(height, width, rows, x, y + 1);
                const float neighbor_avg = (l + r + u + d) * 0.25f;
                const float blend = 0.14f + 0.08f * Hash01(x + pass * 17, y - pass * 11, seed + 701u);
                scratch[idx] = c + (neighbor_avg - c) * blend;
            }
        }
        height.swap(scratch);
    }
}

[[nodiscard]] Core::GenerationMode SelectMode(
    const Core::SiteProfile& profile,
    const float min_buildable_fraction) {
    if (profile.buildable_fraction < std::max(0.12f, min_buildable_fraction * 0.55f)) {
        return Core::GenerationMode::ConservationOnly;
    }
    if (profile.hostile_terrain) {
        return Core::GenerationMode::HillTown;
    }
    if (profile.brownfield_pockets) {
        return Core::GenerationMode::BrownfieldCore;
    }
    if (profile.policy_vs_physics) {
        return Core::GenerationMode::CompromisePlan;
    }
    if (profile.awkward_geometry) {
        return Core::GenerationMode::Patchwork;
    }
    return Core::GenerationMode::Standard;
}

} // namespace

TerrainConstraintGenerator::Output TerrainConstraintGenerator::generate(
    const Input& input,
    const Config& config) const {
    Output output{};

    const double cell_size = std::max(1.0, input.cell_size);
    const int cells_x = std::max(1, static_cast<int>(std::round(static_cast<double>(input.world_width) / cell_size)));
    const int cells_y = std::max(1, static_cast<int>(std::round(static_cast<double>(input.world_height) / cell_size)));

    output.constraints.resize(cells_x, cells_y, cell_size);
    const int denom_x = std::max(1, cells_x - 1);
    const int denom_y = std::max(1, cells_y - 1);
    const double seed_phase = static_cast<double>(input.seed % 10000u) * 0.001;

    uint64_t buildable_cells = 0;
    uint64_t steep_buildable_cells = 0;
    uint64_t brownfield_cells = 0;
    float buildable_slope_sum = 0.0f;

    // First pass: deterministic height synthesis.
    for (int y = 0; y < cells_y; ++y) {
        for (int x = 0; x < cells_x; ++x) {
            const float nx = static_cast<float>(x) / static_cast<float>(denom_x);
            const float ny = static_cast<float>(y) / static_cast<float>(denom_y);

            const float ridge = static_cast<float>(
                std::abs(std::sin((nx * kTwoPi * 1.25) + seed_phase)) *
                std::abs(std::cos((ny * kTwoPi * 0.9) - seed_phase * 0.65)));
            const float terrain_noise = FractalNoise(nx * 64.0f, ny * 64.0f, input.seed + 17u);
            const size_t idx = static_cast<size_t>(output.constraints.toIndex(x, y));
            const float basin_bias = std::clamp(1.0f - ny + 0.20f * (terrain_noise - 0.5f), 0.0f, 1.0f);
            const float base_height =
                14.0f +
                70.0f * (0.56f * ridge + 0.44f * terrain_noise) -
                26.0f * basin_bias;
            output.constraints.height_meters[idx] = base_height;
        }
    }

    ApplyDeterministicErosion(output.constraints.height_meters, cells_x, cells_y, input.seed, 3);

    float min_height = output.constraints.height_meters.front();
    float max_height = output.constraints.height_meters.front();
    for (const float h : output.constraints.height_meters) {
        min_height = std::min(min_height, h);
        max_height = std::max(max_height, h);
    }
    const float height_range = std::max(1e-3f, max_height - min_height);

    // Second pass: derive slope and policy masks from authoritative height field.
    for (int y = 0; y < cells_y; ++y) {
        for (int x = 0; x < cells_x; ++x) {
            const size_t idx = static_cast<size_t>(output.constraints.toIndex(x, y));
            const float h_l = HeightSampleClamped(output.constraints.height_meters, cells_x, cells_y, x - 1, y);
            const float h_r = HeightSampleClamped(output.constraints.height_meters, cells_x, cells_y, x + 1, y);
            const float h_d = HeightSampleClamped(output.constraints.height_meters, cells_x, cells_y, x, y - 1);
            const float h_u = HeightSampleClamped(output.constraints.height_meters, cells_x, cells_y, x, y + 1);

            const double dzdx = (static_cast<double>(h_r) - static_cast<double>(h_l)) / (2.0 * cell_size);
            const double dzdy = (static_cast<double>(h_u) - static_cast<double>(h_d)) / (2.0 * cell_size);
            const double grad = std::sqrt(dzdx * dzdx + dzdy * dzdy);
            const float slope = std::clamp(static_cast<float>(std::atan(grad) * kRadToDeg), 0.0f, 89.0f);

            const float elevation01 = std::clamp((output.constraints.height_meters[idx] - min_height) / height_range, 0.0f, 1.0f);
            const float nx = static_cast<float>(x) / static_cast<float>(denom_x);
            const float ny = static_cast<float>(y) / static_cast<float>(denom_y);
            const float flood_noise = FractalNoise(nx * 48.0f + 13.0f, ny * 48.0f - 9.0f, input.seed + 89u);
            const float flood_score = std::clamp(
                0.66f * (1.0f - elevation01) + 0.34f * flood_noise - 0.12f * (slope / 45.0f),
                0.0f,
                1.0f);
            const uint8_t flood_mask = (flood_score > 0.78f) ? 2u : ((flood_score > 0.56f) ? 1u : 0u);

            const float soil_noise = FractalNoise(nx * 72.0f - 5.0f, ny * 72.0f + 7.0f, input.seed + 133u);
            const float soil_strength = std::clamp(
                1.0f - (slope / 50.0f) + 0.25f * (soil_noise - 0.5f) - (flood_mask == 2u ? 0.22f : 0.0f),
                0.0f,
                1.0f);

            const float nature_noise = FractalNoise(nx * 80.0f + 3.0f, ny * 80.0f - 2.0f, input.seed + 211u);
            const float nature_score = std::clamp(0.45f * (1.0f - elevation01) + 0.55f * nature_noise, 0.0f, 1.0f);

            uint8_t history = 0u;
            const float tag_base = Hash01(x, y, input.seed + 307u);
            if (tag_base > 0.965f) {
                history |= Core::ToHistoryMask(Core::HistoryTag::Brownfield);
            }
            if (tag_base < 0.022f) {
                history |= Core::ToHistoryMask(Core::HistoryTag::SacredSite);
            }
            if (Hash01(x + 83, y - 41, input.seed + 911u) > 0.976f) {
                history |= Core::ToHistoryMask(Core::HistoryTag::Contaminated);
            }
            if (Hash01(x - 59, y + 17, input.seed + 1213u) > 0.971f) {
                history |= Core::ToHistoryMask(Core::HistoryTag::UtilityLegacy);
            }

            const bool no_build = slope > config.max_buildable_slope_deg ||
                flood_mask >= 2u ||
                Core::HasHistoryTag(history, Core::HistoryTag::SacredSite);

            output.constraints.slope_degrees[idx] = slope;
            output.constraints.flood_mask[idx] = flood_mask;
            output.constraints.soil_strength[idx] = soil_strength;
            output.constraints.nature_score[idx] = nature_score;
            output.constraints.history_tags[idx] = history;
            output.constraints.no_build_mask[idx] = no_build ? 1u : 0u;

            if (!no_build) {
                ++buildable_cells;
                buildable_slope_sum += slope;
                if (slope > config.hostile_terrain_slope_deg) {
                    ++steep_buildable_cells;
                }
            }

            if (Core::HasHistoryTag(history, Core::HistoryTag::Brownfield) ||
                Core::HasHistoryTag(history, Core::HistoryTag::Contaminated)) {
                ++brownfield_cells;
            }
        }
    }

    uint64_t transitions = 0;
    uint64_t adjacency = 0;
    for (int y = 0; y < cells_y; ++y) {
        for (int x = 0; x < cells_x; ++x) {
            const uint8_t current = output.constraints.no_build_mask[static_cast<size_t>(output.constraints.toIndex(x, y))];
            if (x + 1 < cells_x) {
                const uint8_t right = output.constraints.no_build_mask[static_cast<size_t>(output.constraints.toIndex(x + 1, y))];
                ++adjacency;
                if (current != right) {
                    ++transitions;
                }
            }
            if (y + 1 < cells_y) {
                const uint8_t down = output.constraints.no_build_mask[static_cast<size_t>(output.constraints.toIndex(x, y + 1))];
                ++adjacency;
                if (current != down) {
                    ++transitions;
                }
            }
        }
    }

    const float total_cells = static_cast<float>(std::max<size_t>(1u, output.constraints.cellCount()));
    const float buildable_fraction = static_cast<float>(buildable_cells) / total_cells;
    const float avg_buildable_slope = (buildable_cells > 0u)
        ? (buildable_slope_sum / static_cast<float>(buildable_cells))
        : 45.0f;
    const float fragmentation = (adjacency > 0u)
        ? (static_cast<float>(transitions) / static_cast<float>(adjacency))
        : 0.0f;
    const float steep_pressure = (buildable_cells > 0u)
        ? (static_cast<float>(steep_buildable_cells) / static_cast<float>(buildable_cells))
        : 1.0f;

    float target_density = 0.55f;
    if (input.city_spec.has_value() && !input.city_spec->districts.empty()) {
        float density_sum = 0.0f;
        for (const auto& district : input.city_spec->districts) {
            density_sum += std::clamp(district.density, 0.0f, 1.0f);
        }
        target_density = density_sum / static_cast<float>(input.city_spec->districts.size());
    }

    const float policy_friction = std::clamp(
        (1.0f - buildable_fraction) * 0.45f +
        fragmentation * 0.32f +
        steep_pressure * 0.17f +
        target_density * (1.0f - buildable_fraction) * 0.20f,
        0.0f,
        1.0f);

    output.profile.buildable_fraction = buildable_fraction;
    output.profile.average_buildable_slope = avg_buildable_slope;
    output.profile.buildable_fragmentation = fragmentation;
    output.profile.policy_friction = policy_friction;
    output.profile.hostile_terrain =
        avg_buildable_slope > config.hostile_terrain_slope_deg ||
        buildable_fraction < config.min_buildable_fraction;
    output.profile.policy_vs_physics = policy_friction > config.policy_friction_threshold;
    output.profile.awkward_geometry = fragmentation > config.fragmentation_threshold;
    output.profile.brownfield_pockets =
        static_cast<float>(brownfield_cells) / total_cells > 0.04f;
    output.profile.mode = SelectMode(output.profile, config.min_buildable_fraction);

    return output;
}

} // namespace RogueCity::Generators
