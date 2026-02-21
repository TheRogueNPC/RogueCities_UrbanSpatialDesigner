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

RogueCity::Core::Vec2 ClampToWorldConstraintsWithSpring(
    const RogueCity::Core::Vec2& desired_world_pos,
    const RogueCity::Core::WorldConstraintField& world_constraints,
    double delta_seconds,
    RogueCity::Core::Vec2& inout_overscroll,
    RogueCity::Core::Vec2& inout_velocity,
    bool& out_active,
    double max_overscroll,
    double spring_strength,
    double damping) {
    if (!world_constraints.isValid()) {
        inout_overscroll = {};
        inout_velocity = {};
        out_active = false;
        return desired_world_pos;
    }

    const RogueCity::Core::Vec2 min_bound(0.0, 0.0);
    const RogueCity::Core::Vec2 max_bound(
        std::max(0.0, static_cast<double>(world_constraints.width) * world_constraints.cell_size),
        std::max(0.0, static_cast<double>(world_constraints.height) * world_constraints.cell_size));
    return ClampToBoundsWithSpring(
        desired_world_pos,
        min_bound,
        max_bound,
        delta_seconds,
        inout_overscroll,
        inout_velocity,
        out_active,
        max_overscroll,
        spring_strength,
        damping);
}

RogueCity::Core::Vec2 ClampToBoundsWithSpring(
    const RogueCity::Core::Vec2& desired_world_pos,
    const RogueCity::Core::Vec2& min_bound,
    const RogueCity::Core::Vec2& max_bound,
    double delta_seconds,
    RogueCity::Core::Vec2& inout_overscroll,
    RogueCity::Core::Vec2& inout_velocity,
    bool& out_active,
    double max_overscroll,
    double spring_strength,
    double damping) {
    const double dt = std::clamp(delta_seconds, 0.0, 0.08);
    const RogueCity::Core::Vec2 hard_clamped{
        std::clamp(desired_world_pos.x, min_bound.x, max_bound.x),
        std::clamp(desired_world_pos.y, min_bound.y, max_bound.y)
    };

    RogueCity::Core::Vec2 overscroll = desired_world_pos - hard_clamped;
    if (overscroll.lengthSquared() > 1e-8) {
        inout_overscroll = overscroll;
    }

    inout_overscroll.x = std::clamp(inout_overscroll.x, -max_overscroll, max_overscroll);
    inout_overscroll.y = std::clamp(inout_overscroll.y, -max_overscroll, max_overscroll);

    if (dt > 0.0) {
        const RogueCity::Core::Vec2 accel =
            (inout_overscroll * -spring_strength) - (inout_velocity * damping);
        inout_velocity += accel * dt;
        inout_overscroll += inout_velocity * dt;
    }

    const double eps = 0.02;
    if (std::abs(inout_overscroll.x) < eps && std::abs(inout_velocity.x) < eps) {
        inout_overscroll.x = 0.0;
        inout_velocity.x = 0.0;
    }
    if (std::abs(inout_overscroll.y) < eps && std::abs(inout_velocity.y) < eps) {
        inout_overscroll.y = 0.0;
        inout_velocity.y = 0.0;
    }

    out_active = inout_overscroll.lengthSquared() > 1e-6 || inout_velocity.lengthSquared() > 1e-6;
    return hard_clamped + inout_overscroll;
}

} // namespace RC_UI::Viewport
