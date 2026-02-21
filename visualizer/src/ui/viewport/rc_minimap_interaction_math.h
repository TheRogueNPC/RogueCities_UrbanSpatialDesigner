#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Math/Vec2.hpp"

namespace RC_UI::Viewport {

[[nodiscard]] float ClampMinimapZoom(
    float zoom,
    float min_zoom = 0.5f,
    float max_zoom = 3.0f);

[[nodiscard]] double ComputeMinimapWorldPerPixel(
    double minimap_world_size,
    float minimap_zoom,
    double minimap_pixel_size);

[[nodiscard]] RogueCity::Core::Vec2 MinimapPixelToWorld(
    const RogueCity::Core::Vec2& pixel_pos,
    const RogueCity::Core::Vec2& minimap_pos,
    double minimap_pixel_size,
    const RogueCity::Core::Vec2& camera_pos,
    double minimap_world_size,
    float minimap_zoom);

[[nodiscard]] RogueCity::Core::Vec2 ClampToWorldConstraints(
    const RogueCity::Core::Vec2& world_pos,
    const RogueCity::Core::WorldConstraintField& world_constraints);

// Shared spring clamp used by viewport pan and minimap drag.
// Allows limited overscroll while applying deterministic bounce-back.
[[nodiscard]] RogueCity::Core::Vec2 ClampToWorldConstraintsWithSpring(
    const RogueCity::Core::Vec2& desired_world_pos,
    const RogueCity::Core::WorldConstraintField& world_constraints,
    double delta_seconds,
    RogueCity::Core::Vec2& inout_overscroll,
    RogueCity::Core::Vec2& inout_velocity,
    bool& out_active,
    double max_overscroll = 140.0,
    double spring_strength = 18.0,
    double damping = 9.0);

[[nodiscard]] RogueCity::Core::Vec2 ClampToBoundsWithSpring(
    const RogueCity::Core::Vec2& desired_world_pos,
    const RogueCity::Core::Vec2& min_bound,
    const RogueCity::Core::Vec2& max_bound,
    double delta_seconds,
    RogueCity::Core::Vec2& inout_overscroll,
    RogueCity::Core::Vec2& inout_velocity,
    bool& out_active,
    double max_overscroll = 140.0,
    double spring_strength = 18.0,
    double damping = 9.0);

} // namespace RC_UI::Viewport
