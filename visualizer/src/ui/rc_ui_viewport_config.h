// FILE: rc_ui_viewport_config.h
// PURPOSE: Viewport padding/margin configuration to prevent window clipping
// PHASE 2: Addresses viewport clipping issues

#pragma once

#include <imgui.h>

namespace RC_UI {

// Viewport configuration constants
struct ViewportConfig {
    // Padding for viewport windows (prevents clipping at edges)
    static constexpr float VIEWPORT_PADDING = 2.0f;
    
    // Margin between docked viewports
    static constexpr float VIEWPORT_MARGIN = 4.0f;
    
    // Border size for viewport windows
    static constexpr float VIEWPORT_BORDER = 1.0f;
    
    // Minimum viewport size
    static constexpr float MIN_VIEWPORT_SIZE = 100.0f;
};

// RAII helper to apply viewport padding
class ScopedViewportPadding {
public:
    ScopedViewportPadding() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ViewportConfig::VIEWPORT_PADDING, ViewportConfig::VIEWPORT_PADDING));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, ViewportConfig::VIEWPORT_BORDER);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ViewportConfig::VIEWPORT_MARGIN, ViewportConfig::VIEWPORT_MARGIN));
    }
    
    ~ScopedViewportPadding() {
        ImGui::PopStyleVar(3);
    }
    
    // Non-copyable, non-movable
    ScopedViewportPadding(const ScopedViewportPadding&) = delete;
    ScopedViewportPadding& operator=(const ScopedViewportPadding&) = delete;
};

// Helper to begin a viewport window with proper padding
inline bool BeginViewportWindow(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
    // Apply padding before creating window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ViewportConfig::VIEWPORT_PADDING, ViewportConfig::VIEWPORT_PADDING));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, ViewportConfig::VIEWPORT_BORDER);
    
    // Ensure minimum size
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(ViewportConfig::MIN_VIEWPORT_SIZE, ViewportConfig::MIN_VIEWPORT_SIZE),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    
    bool result = ImGui::Begin(name, p_open, flags);
    
    ImGui::PopStyleVar(2);
    
    return result;
}

// Helper to end a viewport window
inline void EndViewportWindow() {
    ImGui::End();
}

} // namespace RC_UI
