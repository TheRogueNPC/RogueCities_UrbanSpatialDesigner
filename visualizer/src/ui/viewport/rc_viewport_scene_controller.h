#pragma once

#include "ui/viewport/rc_scene_frame.h"

namespace RogueCity::App {
class GenerationCoordinator;
class MinimapViewport;
class PrimaryViewport;
class ViewportSyncManager;
}

namespace RC_UI::Viewport {

struct SceneControllerUpdateInput {
    float dt{ 0.0f };
    RogueCity::App::PrimaryViewport* primary_viewport{ nullptr };
    RogueCity::App::ViewportSyncManager* sync_manager{ nullptr };
    RogueCity::App::GenerationCoordinator* generation_coordinator{ nullptr };
    RogueCity::App::MinimapViewport* minimap_viewport{ nullptr };
};

void UpdateSceneController(SceneFrame& scene_frame, const SceneControllerUpdateInput& input);

} // namespace RC_UI::Viewport
