// FILE: rc_ui_responsive.h
// PURPOSE: Responsive layout utilities enforcing Cockpit Doctrine
// RULE: 75% margin shrinkage with graceful culling at 25% boundary
#pragma once

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace RC_UI {

// ============================================================================
// RESPONSIVE LAYOUT CONSTANTS (Cockpit Doctrine)
// ============================================================================
// All UI elements must respect these constraints for dynamic responsiveness

struct ResponsiveConstants {
    // === MARGIN SHRINKAGE SYSTEM ===
    // Elements shrink up to 75% of their base size before culling
    static constexpr float MAX_SHRINKAGE_RATIO = 0.75f;     // Max shrink to 75% of base
    static constexpr float CULL_THRESHOLD_RATIO = 0.25f;    // Cull at 25% remaining space
    
    // === MINIMUM SIZES (pixels) ===
    static constexpr float MIN_BUTTON_WIDTH = 32.0f;        // Smallest interactive button
    static constexpr float MIN_BUTTON_HEIGHT = 24.0f;       // Minimum touch target
    static constexpr float MIN_PANEL_WIDTH = 120.0f;        // Minimum panel before collapse
    static constexpr float MIN_PANEL_HEIGHT = 80.0f;        // Minimum panel height
    static constexpr float MIN_CHIP_WIDTH = 48.0f;          // Minimum deck chip
    
    // === BREAKPOINTS ===
    static constexpr float COMPACT_BREAKPOINT = 400.0f;     // Switch to compact mode
    static constexpr float ICON_ONLY_BREAKPOINT = 200.0f;   // Icon-only mode
    static constexpr float COLLAPSE_BREAKPOINT = 120.0f;    // Collapse entirely
    
    // === SPACING MULTIPLIERS ===
    static constexpr float SPACING_NORMAL = 1.0f;           // Base spacing
    static constexpr float SPACING_COMPACT = 0.5f;          // Compact mode spacing
    static constexpr float SPACING_DENSE = 0.25f;           // Dense mode spacing
};

// ============================================================================
// RESPONSIVE LAYOUT MODE
// ============================================================================
enum class LayoutMode {
    Full,       // Full labels + icons
    Compact,    // Shortened labels + icons  
    IconOnly,   // Icons only
    Collapsed   // Hidden/minimal
};

// ============================================================================
// RESPONSIVE LAYOUT CALCULATOR
// ============================================================================
// Calculates optimal layout based on available space

class ResponsiveLayout {
public:
    // Calculate layout mode based on available width
    [[nodiscard]] static LayoutMode GetLayoutMode(float available_width) {
        if (available_width < ResponsiveConstants::COLLAPSE_BREAKPOINT) {
            return LayoutMode::Collapsed;
        }
        if (available_width < ResponsiveConstants::ICON_ONLY_BREAKPOINT) {
            return LayoutMode::IconOnly;
        }
        if (available_width < ResponsiveConstants::COMPACT_BREAKPOINT) {
            return LayoutMode::Compact;
        }
        return LayoutMode::Full;
    }
    
    // Calculate responsive dimension with 75% shrinkage rule
    [[nodiscard]] static float CalculateResponsiveSize(
        float base_size,
        float available_space,
        float min_size
    ) {
        // Calculate shrinkage ratio
        const float shrink_ratio = available_space / base_size;
        
        // If we have more space than needed, use base size
        if (shrink_ratio >= 1.0f) {
            return base_size;
        }
        
        // Apply 75% shrinkage limit
        const float min_shrunk = base_size * (1.0f - ResponsiveConstants::MAX_SHRINKAGE_RATIO);
        const float shrunk_size = std::max(min_shrunk, min_size);
        
        // Interpolate between base and minimum based on available space
        const float t = std::clamp(shrink_ratio, 0.0f, 1.0f);
        return std::max(min_size, base_size * t + shrunk_size * (1.0f - t));
    }
    
    // Calculate how many items fit with responsive sizing
    [[nodiscard]] static int CalculateVisibleItems(
        int total_items,
        float base_item_width,
        float available_width,
        float spacing
    ) {
        if (available_width <= 0.0f || total_items <= 0) {
            return 0;
        }
        
        // Calculate minimum item width (25% of base = cull threshold)
        const float min_item_width = base_item_width * ResponsiveConstants::CULL_THRESHOLD_RATIO;
        
        // If can't fit even one item at minimum, return 0
        if (available_width < min_item_width) {
            return 0;
        }
        
        // Try to fit all items first
        const float total_needed = (base_item_width + spacing) * total_items - spacing;
        if (total_needed <= available_width) {
            return total_items;
        }
        
        // Calculate how many items fit with shrinkage
        const float shrunk_item_width = CalculateResponsiveSize(
            base_item_width, 
            available_width / total_items - spacing,
            min_item_width
        );
        
        // If shrunk items are below cull threshold, reduce count
        if (shrunk_item_width < min_item_width) {
            const int items_at_min = static_cast<int>(available_width / (min_item_width + spacing));
            return std::max(1, items_at_min);
        }
        
        return total_items;
    }
    
    // Calculate item width for responsive bundling (like library buttons)
    [[nodiscard]] static float CalculateBundledItemWidth(
        int visible_items,
        float available_width,
        float spacing,
        float min_width,
        float max_width
    ) {
        if (visible_items <= 0) {
            return min_width;
        }
        
        const float total_spacing = spacing * (visible_items - 1);
        const float available_for_items = available_width - total_spacing;
        const float item_width = available_for_items / visible_items;
        
        return std::clamp(item_width, min_width, max_width);
    }
    
    // Get spacing multiplier based on layout mode
    [[nodiscard]] static float GetSpacingMultiplier(LayoutMode mode) {
        switch (mode) {
            case LayoutMode::Full:      return ResponsiveConstants::SPACING_NORMAL;
            case LayoutMode::Compact:   return ResponsiveConstants::SPACING_COMPACT;
            case LayoutMode::IconOnly:  return ResponsiveConstants::SPACING_DENSE;
            case LayoutMode::Collapsed: return 0.0f;
        }
        return ResponsiveConstants::SPACING_NORMAL;
    }
    
    // Check if element should be culled based on available space
    [[nodiscard]] static bool ShouldCull(float available_space, float min_required) {
        return available_space < min_required * ResponsiveConstants::CULL_THRESHOLD_RATIO;
    }
};

// ============================================================================
// RESPONSIVE CONTAINER - RAII helper for responsive sections
// ============================================================================
class ScopedResponsiveContainer {
public:
    explicit ScopedResponsiveContainer(const char* id, float min_width = 0.0f, float min_height = 0.0f)
        : m_valid(false)
    {
        m_available = ImGui::GetContentRegionAvail();
        m_mode = ResponsiveLayout::GetLayoutMode(m_available.x);
        
        // Check minimum bounds
        if (m_available.x < min_width || m_available.y < min_height) {
            m_valid = false;
            return;
        }
        
        if (m_mode == LayoutMode::Collapsed) {
            m_valid = false;
            return;
        }
        
        m_valid = ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), false, 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }
    
    ~ScopedResponsiveContainer() {
        if (m_valid) {
            ImGui::EndChild();
        }
    }
    
    [[nodiscard]] bool IsValid() const { return m_valid; }
    [[nodiscard]] LayoutMode GetMode() const { return m_mode; }
    [[nodiscard]] ImVec2 GetAvailable() const { return m_available; }
    
    // Non-copyable
    ScopedResponsiveContainer(const ScopedResponsiveContainer&) = delete;
    ScopedResponsiveContainer& operator=(const ScopedResponsiveContainer&) = delete;
    
private:
    bool m_valid;
    LayoutMode m_mode;
    ImVec2 m_available;
};

// ============================================================================
// RESPONSIVE BUTTON LAYOUT HELPER
// ============================================================================
struct ResponsiveButtonLayout {
    int visible_count;          // How many buttons to show
    float button_width;         // Width of each button
    float button_height;        // Height of each button
    float spacing;              // Spacing between buttons
    LayoutMode mode;            // Current layout mode
    bool show_labels;           // Whether to show text labels
    
    // Calculate layout for a row of buttons
    [[nodiscard]] static ResponsiveButtonLayout Calculate(
        int total_buttons,
        float available_width,
        float available_height,
        float base_button_width,
        float base_button_height,
        float base_spacing
    ) {
        ResponsiveButtonLayout layout{};
        layout.mode = ResponsiveLayout::GetLayoutMode(available_width);
        
        if (layout.mode == LayoutMode::Collapsed) {
            layout.visible_count = 0;
            layout.button_width = 0.0f;
            layout.button_height = 0.0f;
            layout.spacing = 0.0f;
            layout.show_labels = false;
            return layout;
        }
        
        const float spacing_mult = ResponsiveLayout::GetSpacingMultiplier(layout.mode);
        layout.spacing = base_spacing * spacing_mult;
        
        // Calculate visible items
        layout.visible_count = ResponsiveLayout::CalculateVisibleItems(
            total_buttons,
            base_button_width,
            available_width,
            layout.spacing
        );
        
        // Calculate button dimensions
        layout.button_width = ResponsiveLayout::CalculateBundledItemWidth(
            layout.visible_count,
            available_width,
            layout.spacing,
            ResponsiveConstants::MIN_BUTTON_WIDTH,
            base_button_width
        );
        
        layout.button_height = ResponsiveLayout::CalculateResponsiveSize(
            base_button_height,
            available_height,
            ResponsiveConstants::MIN_BUTTON_HEIGHT
        );
        
        layout.show_labels = (layout.mode == LayoutMode::Full || layout.mode == LayoutMode::Compact);
        
        return layout;
    }
};

} // namespace RC_UI
