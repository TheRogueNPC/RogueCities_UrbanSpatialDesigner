#include "RogueCity/Core/Editor/TexturePainting.hpp"

#include <algorithm>
#include <cmath>

namespace RogueCity::Core::Editor {

    namespace {
        [[nodiscard]] double clamp01(double v) {
            return std::clamp(v, 0.0, 1.0);
        }
    } // namespace

    bool TexturePainting::applyStroke(
        Data::TextureSpace& texture_space,
        const Stroke& stroke,
        Data::DirtyRegion* out_dirty_region) {
        if (stroke.radius_meters <= 0.0) {
            if (out_dirty_region != nullptr) {
                out_dirty_region->clear();
            }
            return false;
        }

        auto& coords = texture_space.coordinateSystem();
        const double meters_per_pixel = std::max(coords.metersPerPixel(), 1e-9);
        const double radius_px = std::max(0.5, stroke.radius_meters / meters_per_pixel);
        const Data::Int2 center = coords.worldToPixel(stroke.world_center);
        const float opacity = static_cast<float>(clamp01(stroke.opacity));
        if (opacity <= 0.0f) {
            if (out_dirty_region != nullptr) {
                out_dirty_region->clear();
            }
            return false;
        }

        Data::Texture2D<uint8_t>* target = nullptr;
        if (stroke.layer == Layer::Zone) {
            target = &texture_space.zoneLayer();
        } else {
            target = &texture_space.materialLayer();
        }

        if (target == nullptr || target->empty()) {
            if (out_dirty_region != nullptr) {
                out_dirty_region->clear();
            }
            return false;
        }

        const int min_x = std::max(0, static_cast<int>(std::floor(static_cast<double>(center.x) - radius_px)));
        const int max_x = std::min(target->width() - 1, static_cast<int>(std::ceil(static_cast<double>(center.x) + radius_px)));
        const int min_y = std::max(0, static_cast<int>(std::floor(static_cast<double>(center.y) - radius_px)));
        const int max_y = std::min(target->height() - 1, static_cast<int>(std::ceil(static_cast<double>(center.y) + radius_px)));

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
                const float blend = opacity * falloff;
                if (blend <= 0.0f) {
                    continue;
                }

                const uint8_t current = target->at(x, y);
                const float mixed =
                    static_cast<float>(current) +
                    (static_cast<float>(stroke.value) - static_cast<float>(current)) * blend;
                const int rounded = static_cast<int>(std::lround(mixed));
                const uint8_t next = static_cast<uint8_t>(std::clamp(rounded, 0, 255));
                if (next != current) {
                    target->at(x, y) = next;
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
