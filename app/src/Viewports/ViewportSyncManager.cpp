#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include <algorithm>
#include <cmath>

namespace RogueCity::App {

ViewportSyncManager::ViewportSyncManager(PrimaryViewport* primary, MinimapViewport* minimap)
    : primary_(primary)
    , minimap_(minimap)
{
}

void ViewportSyncManager::update(float delta_time) {
    if (!sync_enabled_ || !primary_ || !minimap_) return;

    // Get primary viewport XY camera position
    const Core::Vec2 primary_xy = primary_->get_camera_xy();

    // Smooth interpolation to reduce jitter
    if (smooth_factor_ > 0.0f) {
        const float base_blend = std::clamp(smooth_factor_ + 0.1f, 0.0f, 1.0f);
        const float frame_scale = std::max(1.0f, delta_time * 60.0f);
        const float blend = 1.0f - std::pow(1.0f - base_blend, frame_scale);
        last_synced_xy_.x += (primary_xy.x - last_synced_xy_.x) * blend;
        last_synced_xy_.y += (primary_xy.y - last_synced_xy_.y) * blend;
    } else {
        last_synced_xy_ = primary_xy;
    }

    // Apply synced position to minimap
    minimap_->set_camera_position(last_synced_xy_);
}

void ViewportSyncManager::set_sync_enabled(bool enabled) {
    sync_enabled_ = enabled;
    
    // Initialize sync position when enabling
    if (enabled && primary_) {
        last_synced_xy_ = primary_->get_camera_xy();
    }
}

bool ViewportSyncManager::is_sync_enabled() const {
    return sync_enabled_;
}

void ViewportSyncManager::set_smooth_factor(float factor) {
    smooth_factor_ = std::max(0.0f, std::min(factor, 1.0f));
}

} // namespace RogueCity::App
