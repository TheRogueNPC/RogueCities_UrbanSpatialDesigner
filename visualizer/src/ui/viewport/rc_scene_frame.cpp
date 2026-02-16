#include "ui/viewport/rc_scene_frame.h"

#include <algorithm>

namespace RC_UI::Viewport {

SceneFrame BuildSceneFrame(
    const RogueCity::Generators::CityGenerator::CityOutput* output,
    const RogueCity::Core::Vec2& camera_xy,
    float camera_z,
    float camera_yaw,
    uint64_t generation_serial) {
    SceneFrame frame{};
    frame.output = output;
    frame.camera_xy = camera_xy;
    frame.camera_z = camera_z;
    frame.camera_yaw = camera_yaw;
    frame.viewport_zoom = 500.0f / std::max(100.0f, camera_z);
    frame.generation_serial = generation_serial;
    return frame;
}

} // namespace RC_UI::Viewport
