#pragma once
#include "RogueCity/Core/Types.hpp"
#include <memory>

namespace RogueCity::App {

class PrimaryViewport;
class MinimapViewport;
class ViewportSyncManager;

/// Orchestrates dual-viewport system (Primary 3D/2D + Minimap 2D)
/// Enforces Cockpit Doctrine: viewports as control stations on the network
class ViewportManager {
public:
    ViewportManager();
    ~ViewportManager();

    /// Initialize viewports with window context
    void initialize(void* glfw_window);

    /// Update both viewports (call per frame)
    void update(float delta_time);

    /// Render both viewports to ImGui
    void render();

    /// Toggle camera synchronization
    void set_camera_sync(bool enabled);
    [[nodiscard]] bool is_camera_synced() const;

    /// Access individual viewports
    [[nodiscard]] PrimaryViewport& primary();
    [[nodiscard]] MinimapViewport& minimap();

private:
    std::unique_ptr<PrimaryViewport> primary_viewport_;
    std::unique_ptr<MinimapViewport> minimap_viewport_;
    std::unique_ptr<ViewportSyncManager> sync_manager_;
    bool camera_sync_enabled_{ true };
};

} // namespace RogueCity::App
