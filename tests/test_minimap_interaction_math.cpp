#include "ui/viewport/rc_minimap_interaction_math.h"

#include "RogueCity/Core/Data/CityTypes.hpp"

#include <cassert>

int main() {
    using RC_UI::Viewport::ClampMinimapZoom;
    using RC_UI::Viewport::ClampToWorldConstraints;
    using RC_UI::Viewport::ComputeMinimapWorldPerPixel;
    using RC_UI::Viewport::MinimapPixelToWorld;
    using RogueCity::Core::Vec2;
    using RogueCity::Core::WorldConstraintField;

    assert(ClampMinimapZoom(0.1f) == 0.5f);
    assert(ClampMinimapZoom(10.0f) == 3.0f);
    assert(ClampMinimapZoom(1.2f) == 1.2f);

    const double wpp_zoom1 = ComputeMinimapWorldPerPixel(2000.0, 1.0f, 250.0);
    const double wpp_zoom2 = ComputeMinimapWorldPerPixel(2000.0, 2.0f, 250.0);
    assert(wpp_zoom1 == 8.0);
    assert(wpp_zoom2 == 16.0);

    const Vec2 camera(100.0, 200.0);
    const Vec2 map_pos(20.0, 40.0);
    const Vec2 center_pixel(145.0, 165.0); // map_pos + (125,125)
    const Vec2 center_world = MinimapPixelToWorld(center_pixel, map_pos, 250.0, camera, 2000.0, 1.0f);
    assert(center_world.distanceTo(camera) < 1e-6);

    const Vec2 top_left_world = MinimapPixelToWorld(Vec2(20.0, 40.0), map_pos, 250.0, camera, 2000.0, 1.0f);
    assert(top_left_world.distanceTo(Vec2(-900.0, -800.0)) < 1e-6);

    WorldConstraintField constraints{};
    constraints.resize(100, 50, 10.0);
    const Vec2 clamped = ClampToWorldConstraints(Vec2(1200.0, -20.0), constraints);
    assert(clamped.x == 1000.0);
    assert(clamped.y == 0.0);

    WorldConstraintField invalid{};
    const Vec2 passthrough = ClampToWorldConstraints(Vec2(12.0, 34.0), invalid);
    assert(passthrough.x == 12.0);
    assert(passthrough.y == 34.0);

    return 0;
}
