#include "ui/viewport/rc_minimap_renderer.h"

#include "RogueCity/App/Viewports/MinimapViewport.hpp"

namespace RC_UI::Viewport {

void SyncMinimapFromSceneFrame(RogueCity::App::MinimapViewport* minimap, const SceneFrame& frame) {
    if (minimap == nullptr) {
        return;
    }
    minimap->set_city_output(frame.output);
}

} // namespace RC_UI::Viewport
