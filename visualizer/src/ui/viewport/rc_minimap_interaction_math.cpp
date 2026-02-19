#include "ui/viewport/rc_minimap_interaction_math.h"

#include <algorithm>

namespace RC_UI::Viewport {

float ClampMinimapZoom(float zoom, float min_zoom, float max_zoom) {
    if (min_zoom > max_zoom) {
        std::swap(min_zoom, max_zoom);
    }
    return std::clamp(zoom, min_zoom, max_zoom);
}

double ComputeMinimapWorldPerPixel(
    double minimap_world_size,
    float minimap_zoom,
    double minimap_pixel_size) {
    const double safe_pixel_size = std::max(1e-6, minimap_pixel_size);
    const double safe_zoom = std::max(0.01, static_cast<double>(minimap_zoom));
    return (minimap_world_size * safe_zoom) / safe_pixel_size;
}

RogueCity::Core::Vec2 MinimapPixelToWorld(
    const RogueCity::Core::Vec2& pixel_pos,
    const RogueCity::Core::Vec2& minimap_pos,
    double minimap_pixel_size,
    const RogueCity::Core::Vec2& camera_pos,
    double minimap_world_size,
    float minimap_zoom) {
    const double safe_pixel_size = std::max(1e-6, minimap_pixel_size);
    const double world_span = minimap_world_size * std::max(0.01, static_cast<double>(minimap_zoom));
    const double u = (pixel_pos.x - minimap_pos.x) / safe_pixel_size;
    const double v = (pixel_pos.y - minimap_pos.y) / safe_pixel_size;

    const double world_x = camera_pos.x + (u - 0.5) * world_span;
    const double world_y = camera_pos.y + (v - 0.5) * world_span;
    return RogueCity::Core::Vec2(world_x, world_y);
}

RogueCity::Core::Vec2 ClampToWorldConstraints(
    const RogueCity::Core::Vec2& world_pos,
    const RogueCity::Core::WorldConstraintField& world_constraints) {
    if (!world_constraints.isValid()) {
        return world_pos;
    }

    const double max_x =
        static_cast<double>(world_constraints.width) * world_constraints.cell_size;
    const double max_y =
        static_cast<double>(world_constraints.height) * world_constraints.cell_size;

    return {
        std::clamp(world_pos.x, 0.0, std::max(0.0, max_x)),
        std::clamp(world_pos.y, 0.0, std::max(0.0, max_y)),
    };
}

} // namespace RC_UI::Viewport
