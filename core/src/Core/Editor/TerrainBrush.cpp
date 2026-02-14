#include "RogueCity/Core/Editor/TerrainBrush.hpp"

#include <algorithm>
#include <cmath>
// Implementation of a simple terrain editing brush that modifies a heightmap texture based on user input strokes.
namespace RogueCity::Core::Editor {

    namespace {
        [[nodiscard]] double clamp01(double v) {
            return std::clamp(v, 0.0, 1.0);
        }
    } // namespace

    bool TerrainBrush::applyStroke(
        Data::TextureSpace& texture_space,
        const Stroke& stroke,
        Data::DirtyRegion* out_dirty_region) {
        auto& height = texture_space.heightLayer();
        if (height.empty() || stroke.radius_meters <= 0.0) {
            if (out_dirty_region != nullptr) {
                out_dirty_region->clear();
            }
            return false;
        }

        const auto& coords = texture_space.coordinateSystem();
        const double meters_per_pixel = std::max(coords.metersPerPixel(), 1e-9);
        const double radius_px = std::max(0.5, stroke.radius_meters / meters_per_pixel);
        const Data::Int2 center = coords.worldToPixel(stroke.world_center);

        const int min_x = std::max(0, static_cast<int>(std::floor(static_cast<double>(center.x) - radius_px)));
        const int max_x = std::min(height.width() - 1, static_cast<int>(std::ceil(static_cast<double>(center.x) + radius_px)));
        const int min_y = std::max(0, static_cast<int>(std::floor(static_cast<double>(center.y) - radius_px)));
        const int max_y = std::min(height.height() - 1, static_cast<int>(std::ceil(static_cast<double>(center.y) + radius_px)));

        const float signed_strength = std::abs(stroke.strength);
        const float blend_strength = static_cast<float>(clamp01(static_cast<double>(signed_strength)));
        const float flatten_target = stroke.use_flatten_height
            ? stroke.flatten_height
            : height.at(center.x, center.y);

        Data::Texture2D<float> source_for_smooth{};
        if (stroke.mode == Mode::Smooth) {
            source_for_smooth = height;
        }

        bool changed = false;
        Data::DirtyRegion dirty_region{};
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const double dx = static_cast<double>(x - center.x);
                const double dy = static_cast<double>(y - center.y);
                const double dist = std::sqrt((dx * dx) + (dy * dy));
                if (dist > radius_px) {
                    continue;
                }

                const float falloff = static_cast<float>(1.0 - clamp01(dist / radius_px));
                const float current = height.at(x, y);
                float next = current;

                switch (stroke.mode) {
                    case Mode::Raise:
                        next = current + (signed_strength * falloff);
                        break;
                    case Mode::Lower:
                        next = current - (signed_strength * falloff);
                        break;
                    case Mode::Flatten: {
                        const float blend = blend_strength * falloff;
                        next = current + (flatten_target - current) * blend;
                        break;
                    }
                    case Mode::Smooth: {
                        float neighborhood_sum = 0.0f;
                        int neighborhood_count = 0;
                        for (int oy = -1; oy <= 1; ++oy) {
                            const int sy = std::clamp(y + oy, 0, height.height() - 1);
                            for (int ox = -1; ox <= 1; ++ox) {
                                const int sx = std::clamp(x + ox, 0, height.width() - 1);
                                neighborhood_sum += source_for_smooth.at(sx, sy);
                                ++neighborhood_count;
                            }
                        }
                        const float average = neighborhood_sum / static_cast<float>(std::max(1, neighborhood_count));
                        const float blend = blend_strength * falloff;
                        next = current + (average - current) * blend;
                        break;
                    }
                }

                if (std::abs(next - current) > 1e-6f) {
                    height.at(x, y) = next;
                    dirty_region.include(x, y);
                    changed = true;
                }
            }
        }

        if (out_dirty_region != nullptr) {
            if (changed) {
                *out_dirty_region = dirty_region;
            } else {
                out_dirty_region->clear();
            }
        }

        return changed;
    }

} // namespace RogueCity::Core::Editor
