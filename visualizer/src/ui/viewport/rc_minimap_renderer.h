#pragma once

#include "ui/viewport/rc_scene_frame.h"

namespace RogueCity::App {
class MinimapViewport;
}

namespace RC_UI::Viewport {

void SyncMinimapFromSceneFrame(RogueCity::App::MinimapViewport* minimap, const SceneFrame& frame);

} // namespace RC_UI::Viewport
