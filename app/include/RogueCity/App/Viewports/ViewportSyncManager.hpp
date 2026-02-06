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
