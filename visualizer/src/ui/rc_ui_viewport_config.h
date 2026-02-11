// FILE: rc_ui_viewport_config.h
// PURPOSE: Viewport padding/margin configuration with edge case hardening
// COCKPIT DOCTRINE: Bounds validation, graceful degradation, null safety
#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace RC_UI {

// ============================================================================
// VIEWPORT CONFIGURATION CONSTANTS
// ============================================================================
struct ViewportConfig {
    // Padding for viewport windows (prevents clipping at edges)
    static constexpr float VIEWPORT_PADDING = 2.0f;
    
    // Margin between docked viewports
    static constexpr float VIEWPORT_MARGIN = 4.0f;
    
    // Border size for viewport windows
    static constexpr float VIEWPORT_BORDER = 1.0f;
    
    // Minimum viewport sizes
    static constexpr float MIN_VIEWPORT_WIDTH = 100.0f;
    static constexpr float MIN_VIEWPORT_HEIGHT = 80.0f;
    static constexpr float MIN_VIEWPORT_SIZE = 100.0f;
    
    // Maximum viewport sizes (prevent overflow)
    static constexpr float MAX_VIEWPORT_WIDTH = 8192.0f;
    static constexpr float MAX_VIEWPORT_HEIGHT = 8192.0f;
    
    // Dock node validation thresholds
    static constexpr float MIN_DOCK_NODE_SIZE = 50.0f;
    
    // Responsive 75% shrinkage threshold
    static constexpr float SHRINKAGE_WARNING_RATIO = 0.5f;  // Warn at 50%
    static constexpr float SHRINKAGE_CRITICAL_RATIO = 0.25f; // Critical at 25%
};

// ============================================================================
// VIEWPORT BOUNDS VALIDATION
// ============================================================================
struct ViewportBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    [[nodiscard]] bool IsValid() const {
        return width >= ViewportConfig::MIN_VIEWPORT_WIDTH && 
               height >= ViewportConfig::MIN_VIEWPORT_HEIGHT &&
               std::isfinite(x) && std::isfinite(y) &&
               std::isfinite(width) && std::isfinite(height);
    }
    
    [[nodiscard]] bool IsCritical() const {
        return width < ViewportConfig::MIN_VIEWPORT_WIDTH * ViewportConfig::SHRINKAGE_CRITICAL_RATIO ||
               height < ViewportConfig::MIN_VIEWPORT_HEIGHT * ViewportConfig::SHRINKAGE_CRITICAL_RATIO;
    }
    
    [[nodiscard]] ImVec2 GetMin() const { return ImVec2(x, y); }
    [[nodiscard]] ImVec2 GetMax() const { return ImVec2(x + width, y + height); }
    [[nodiscard]] ImVec2 GetSize() const { return ImVec2(width, height); }
    [[nodiscard]] ImVec2 GetCenter() const { return ImVec2(x + width * 0.5f, y + height * 0.5f); }
    
    // Clamp bounds to valid range
    void Clamp() {
        width = std::clamp(width, ViewportConfig::MIN_VIEWPORT_WIDTH, ViewportConfig::MAX_VIEWPORT_WIDTH);
        height = std::clamp(height, ViewportConfig::MIN_VIEWPORT_HEIGHT, ViewportConfig::MAX_VIEWPORT_HEIGHT);
    }
    
    static ViewportBounds FromImGui() {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        return ViewportBounds{pos.x, pos.y, size.x, size.y};
    }
    
    static ViewportBounds FromContentRegion() {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size = ImGui::GetContentRegionAvail();
        return ViewportBounds{pos.x, pos.y, size.x, size.y};
    }
};

// ============================================================================
// DOCK NODE VALIDATION
// ============================================================================
struct DockNodeValidator {
    // Check if a dock node is valid and usable
    [[nodiscard]] static bool IsNodeValid(ImGuiDockNode* node) {
        if (node == nullptr) return false;
        if (node->ID == 0) return false;
        return true;
    }
    
    // Check if a dock node has adequate size
    [[nodiscard]] static bool IsNodeSizeValid(ImGuiDockNode* node) {
        if (!IsNodeValid(node)) return false;
        return node->Size.x >= ViewportConfig::MIN_DOCK_NODE_SIZE &&
               node->Size.y >= ViewportConfig::MIN_DOCK_NODE_SIZE;
    }
    
    // Get root node safely with null checks
    [[nodiscard]] static ImGuiDockNode* GetRootNodeSafe(ImGuiDockNode* node) {
        if (node == nullptr) return nullptr;
        ImGuiDockNode* current = node;
        int depth = 0;
        const int max_depth = 100; // Prevent infinite loops
        while (current->ParentNode != nullptr && depth < max_depth) {
            current = current->ParentNode;
            ++depth;
        }
        return current;
    }
    
    // Validate dock node belongs to expected dockspace
    [[nodiscard]] static bool BelongsToDockspace(ImGuiDockNode* node, ImGuiID dockspace_id) {
        if (node == nullptr || dockspace_id == 0) return false;
        ImGuiDockNode* root = GetRootNodeSafe(node);
        return root != nullptr && root->ID == dockspace_id;
    }
    
    // Check if window has valid dock assignment
    [[nodiscard]] static bool IsWindowDockValid(ImGuiWindow* window, ImGuiID dockspace_id) {
        if (window == nullptr) return false;
        if (window->DockId == 0) return true; // Not docked is valid
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(window->DockId);
        return node != nullptr && BelongsToDockspace(node, dockspace_id);
    }
};

// ============================================================================
// RAII VIEWPORT PADDING
// ============================================================================
class ScopedViewportPadding {
public:
    ScopedViewportPadding() : m_count(0) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, 
            ImVec2(ViewportConfig::VIEWPORT_PADDING, ViewportConfig::VIEWPORT_PADDING));
        ++m_count;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, ViewportConfig::VIEWPORT_BORDER);
        ++m_count;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, 
            ImVec2(ViewportConfig::VIEWPORT_MARGIN, ViewportConfig::VIEWPORT_MARGIN));
        ++m_count;
    }
    
    ~ScopedViewportPadding() {
        if (m_count > 0) {
            ImGui::PopStyleVar(m_count);
        }
    }
    
    // Non-copyable, non-movable
    ScopedViewportPadding(const ScopedViewportPadding&) = delete;
    ScopedViewportPadding& operator=(const ScopedViewportPadding&) = delete;
    
private:
    int m_count;
};

// ============================================================================
// SAFE VIEWPORT WINDOW HELPERS
// ============================================================================

// Begin a viewport window with bounds validation and edge case guards
inline bool BeginViewportWindowSafe(
    const char* name, 
    bool* p_open = nullptr, 
    ImGuiWindowFlags flags = 0,
    ViewportBounds* out_bounds = nullptr
) {
    // Null check on name
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    
    // Apply padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, 
        ImVec2(ViewportConfig::VIEWPORT_PADDING, ViewportConfig::VIEWPORT_PADDING));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, ViewportConfig::VIEWPORT_BORDER);
    
    // Set size constraints with valid min/max
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(ViewportConfig::MIN_VIEWPORT_WIDTH, ViewportConfig::MIN_VIEWPORT_HEIGHT),
        ImVec2(ViewportConfig::MAX_VIEWPORT_WIDTH, ViewportConfig::MAX_VIEWPORT_HEIGHT)
    );
    
    bool result = ImGui::Begin(name, p_open, flags);
    
    ImGui::PopStyleVar(2);
    
    // Populate bounds if requested
    if (out_bounds != nullptr) {
        *out_bounds = ViewportBounds::FromImGui();
        out_bounds->Clamp();
    }
    
    return result;
}

// Legacy compatibility wrapper
inline bool BeginViewportWindow(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
    return BeginViewportWindowSafe(name, p_open, flags, nullptr);
}

inline void EndViewportWindow() {
    ImGui::End();
}

// ============================================================================
// CONTENT REGION VALIDATION
// ============================================================================

// Check if content region is usable (not too small)
[[nodiscard]] inline bool IsContentRegionUsable(float min_width = 0.0f, float min_height = 0.0f) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float req_w = min_width > 0.0f ? min_width : ViewportConfig::MIN_VIEWPORT_WIDTH;
    const float req_h = min_height > 0.0f ? min_height : ViewportConfig::MIN_VIEWPORT_HEIGHT;
    return avail.x >= req_w && avail.y >= req_h;
}

// Get content region with validation
[[nodiscard]] inline ViewportBounds GetContentRegionValidated() {
    ViewportBounds bounds = ViewportBounds::FromContentRegion();
    bounds.Clamp();
    return bounds;
}

// ============================================================================
// DOCKING SAFETY UTILITIES
// ============================================================================

// Safely get dock node, returns nullptr on any error
[[nodiscard]] inline ImGuiDockNode* GetDockNodeSafe(ImGuiID dock_id) {
    if (dock_id == 0) return nullptr;
    return ImGui::DockBuilderGetNode(dock_id);
}

// Check if dockspace exists and is valid
[[nodiscard]] inline bool IsDockspaceValid(ImGuiID dockspace_id) {
    if (dockspace_id == 0) return false;
    ImGuiDockNode* node = GetDockNodeSafe(dockspace_id);
    return DockNodeValidator::IsNodeValid(node);
}

// Safely dock a window with validation
inline bool DockWindowSafe(const char* window_name, ImGuiID dock_id) {
    if (window_name == nullptr || window_name[0] == '\0') return false;
    if (dock_id == 0) return false;
    
    ImGuiDockNode* node = GetDockNodeSafe(dock_id);
    if (!DockNodeValidator::IsNodeSizeValid(node)) return false;
    
    ImGui::DockBuilderDockWindow(window_name, dock_id);
    return true;
}

} // namespace RC_UI
