#include "ui/viewport/rc_viewport_scene_controller.h"

#include "RogueCity/App/Integration/GenerationCoordinator.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "ui/viewport/rc_minimap_renderer.h"

namespace RC_UI::Viewport {

void UpdateSceneController(SceneFrame& scene_frame, const SceneControllerUpdateInput& input) {
    if (input.sync_manager != nullptr) {
        input.sync_manager->update(input.dt);
    }
    if (input.generation_coordinator != nullptr) {
        input.generation_coordinator->Update(input.dt);
    }

    if (input.primary_viewport == nullptr) {
        return;
    }

    const auto* output = input.generation_coordinator != nullptr
        ? input.generation_coordinator->GetOutput()
        : nullptr;
    const uint64_t completed_serial = input.generation_coordinator != nullptr
        ? input.generation_coordinator->LastCompletedSerial()
        : 0u;

    scene_frame = BuildSceneFrame(
        output,
        input.primary_viewport->get_camera_xy(),
        input.primary_viewport->get_camera_z(),
        input.primary_viewport->get_camera_yaw(),
        completed_serial);
    SyncMinimapFromSceneFrame(input.minimap_viewport, scene_frame);
}

} // namespace RC_UI::Viewport
