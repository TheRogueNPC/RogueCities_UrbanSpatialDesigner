/**
 * @class ViewportSyncManager
 * @brief Manages camera synchronization between primary and minimap viewports.
 *
 * This class provides functionality to synchronize the XY position of cameras between
 * the primary viewport and the minimap viewport. It supports smooth interpolation to
 * reduce jitter during synchronization, and allows toggling of sync mode and adjustment
 * of the smoothness factor.
 *
 * @note Call update(float delta_time) per frame when synchronization is enabled.
 *
 * @param primary Pointer to the primary viewport.
 * @param minimap Pointer to the minimap viewport.
 *
 * @method update Updates the camera synchronization each frame.
 * @method set_sync_enabled Enables or disables synchronization.
 * @method is_sync_enabled Returns whether synchronization is enabled.
 * @method set_smooth_factor Sets the interpolation factor for smooth synchronization.
 */
 
#pragma once
#include "RogueCity/Core/Types.hpp"

namespace RogueCity::App {

class PrimaryViewport;
class MinimapViewport;

/// Manages camera synchronization between primary and minimap viewports
/// Implements XY position sync with smooth interpolation option
class ViewportSyncManager {
public:
    ViewportSyncManager(PrimaryViewport* primary, MinimapViewport* minimap);

    /// Update sync (call per frame when sync enabled)
    void update(float delta_time);

    /// Toggle sync mode
    void set_sync_enabled(bool enabled);
    [[nodiscard]] bool is_sync_enabled() const;

    /// Smooth sync interpolation (reduces jitter)
    void set_smooth_factor(float factor);  // 0.0 = instant, 1.0 = very smooth

private:
    PrimaryViewport* primary_;
    MinimapViewport* minimap_;
    bool sync_enabled_{ true };
    float smooth_factor_{ 0.2f };  // Lerp factor for smooth follow
    Core::Vec2 last_synced_xy_{ 0.0, 0.0 };
};

} // namespace RogueCity::App
