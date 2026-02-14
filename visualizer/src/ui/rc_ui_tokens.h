// FILE: rc_ui_tokens.h
// PURPOSE: Unified UI Token Schema - Single Source of Truth
// COCKPIT DOCTRINE: All UI code MUST reference these tokens, never hardcode values
#pragma once

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace RC_UI {

// ============================================================================
// UNIFIED UI TOKEN SCHEMA
// ============================================================================
// This file consolidates all design tokens for the RogueCity UI system.
// It enforces Cockpit Doctrine compliance and provides gamified visual feedback.

struct UITokens {
    // ========================================================================
    // Y2K COLOR PALETTE (High contrast, neon accents)
    // ========================================================================
    
    // Primary accent colors
    static constexpr ImU32 CyanAccent       = IM_COL32(0, 255, 255, 255);
    static constexpr ImU32 GreenHUD         = IM_COL32(0, 255, 128, 255);
    static constexpr ImU32 YellowWarning    = IM_COL32(255, 200, 0, 255);
    static constexpr ImU32 MagentaHighlight = IM_COL32(255, 0, 255, 255);
    static constexpr ImU32 AmberGlow        = IM_COL32(255, 180, 0, 255);
    
    // Background colors (Deep, low-luminance)
    static constexpr ImU32 BackgroundDark   = IM_COL32(15, 20, 30, 255);
    static constexpr ImU32 PanelBackground  = IM_COL32(20, 25, 35, 255);
    static constexpr ImU32 GridOverlay      = IM_COL32(40, 50, 70, 100);
    
    // Text colors
    static constexpr ImU32 TextPrimary      = IM_COL32(255, 255, 255, 255);
    static constexpr ImU32 TextSecondary    = IM_COL32(180, 180, 180, 255);
    static constexpr ImU32 TextDisabled     = IM_COL32(100, 100, 100, 255);
    
    // State feedback colors
    static constexpr ImU32 ErrorRed         = IM_COL32(255, 50, 50, 255);
    static constexpr ImU32 SuccessGreen     = IM_COL32(0, 255, 100, 255);
    static constexpr ImU32 InfoBlue         = IM_COL32(100, 150, 255, 255);
    
    // ========================================================================
    // SPACING SYSTEM (8px base unit)
    // ========================================================================
    static constexpr float SpaceXS   = 4.0f;    // 0.5 units
    static constexpr float SpaceS    = 8.0f;    // 1 unit
    static constexpr float SpaceM    = 16.0f;   // 2 units
    static constexpr float SpaceL    = 24.0f;   // 3 units
    static constexpr float SpaceXL   = 32.0f;   // 4 units
    
    // ========================================================================
    // BORDER WIDTHS (Y2K hard-edged)
    // ========================================================================
    static constexpr float BorderThin   = 1.0f;
    static constexpr float BorderNormal = 2.0f;
    static constexpr float BorderThick  = 3.0f;
    
    // ========================================================================
    // CORNER ROUNDING (Minimal for Y2K) 
    // ========================================================================
    static constexpr float RoundingNone    = 0.0f;
    static constexpr float RoundingSubtle  = 2.0f;
    static constexpr float RoundingButton  = 4.0f;
    static constexpr float RoundingPanel   = 8.0f;
    static constexpr float RoundingChip    = 14.0f;
    
    // ========================================================================
    // RESPONSIVE LAYOUT TOKENS (For dynamic resizing and adaptive UI)
    // ========================================================================
    // 75% shrinkage with 25% cull threshold
    static constexpr float ResponsiveMaxShrink   = 0.75f;
    static constexpr float ResponsiveCullAt      = 0.25f;
    static constexpr float ResponsiveMinButton   = 32.0f;
    static constexpr float ResponsiveMinPanel    = 120.0f;
    
    // Breakpoints
    static constexpr float BreakpointCompact     = 400.0f;
    static constexpr float BreakpointIconOnly    = 200.0f;
    static constexpr float BreakpointCollapse    = 120.0f;
    
    // ========================================================================
    // ANIMATION TIMING (Motion as Instruction)
    // ========================================================================
    static constexpr float AnimExpansion = 0.8f;   // Ring expansion
    static constexpr float AnimResize    = 0.3f;   // Knob resize
    static constexpr float AnimPulse     = 2.0f;   // Hz, breathing UI
    static constexpr float AnimFade      = 0.5f;   // Panel fade
    
    // ========================================================================
    // VISUAL FEEDBACK TOKENS (Gamified UX)
    // ========================================================================
    static constexpr float HoverGlowIntensity    = 0.3f;
    static constexpr float ActivePulseHz         = 1.5f;
    static constexpr float TransitionDuration    = 0.15f;
    static constexpr float FeedbackFlashDuration = 0.1f;
    static constexpr float HoverScaleUp          = 1.05f;  // Slight grow on hover
    static constexpr float ClickScaleDown        = 0.95f;  // Slight shrink on click
    
    // ========================================================================
    // TYPOGRAPHY TOKENS (Y2K digital font vibes)
    // ========================================================================
    static constexpr float FontSizeSmall   = 12.0f;
    static constexpr float FontSizeMedium  = 14.0f;
    static constexpr float FontSizeLarge   = 18.0f;
    static constexpr float FontSizeTitle   = 24.0f;
    
    // ========================================================================
    // DECK/CHIP SIZING (For tool decks, status chips, etc.)
    // ========================================================================
    static constexpr float ChipBaseWidth      = 110.0f;
    static constexpr float ChipHoverExpansion = 30.0f;
    static constexpr float ChipMinWidth       = 48.0f;
    static constexpr float ChipHeight         = 28.0f;
    static constexpr float ChipMinHeight      = 20.0f;
    static constexpr float ChipSpacing        = 8.0f;
    
    // ========================================================================
    // PANEL CONSTRAINTS (Enforce minimum sizes for usability)
    // ========================================================================
    static constexpr float PanelMinWidth  = 120.0f;
    static constexpr float PanelMinHeight = 80.0f;
    static constexpr float PanelMaxWidth  = 800.0f;
    static constexpr float PanelMaxHeight = 1200.0f;
};

// ============================================================================
// VISUAL FEEDBACK UTILITIES //todo: expand this section to include more complex feedback patterns (e.g. multi-stage clicks, long-press, toggle states) and integrate with a centralized input handling system for consistent state management across the UI.
// ============================================================================

// Calculate hover glow color with intensity
inline ImU32 GetHoverGlow(ImU32 base_color, float intensity = UITokens::HoverGlowIntensity) {
    ImVec4 col = ImGui::ColorConvertU32ToFloat4(base_color);
    col.x = std::min(1.0f, col.x + intensity);
    col.y = std::min(1.0f, col.y + intensity);
    col.z = std::min(1.0f, col.z + intensity);
    return ImGui::ColorConvertFloat4ToU32(col);
}

// Calculate pulse value for active states (0.0 - 1.0)
inline float GetPulseValue(float time_sec, float frequency_hz = UITokens::ActivePulseHz) {
    return (std::sin(time_sec * frequency_hz * 6.283185f) + 1.0f) * 0.5f;
}

// Apply alpha to color
inline ImU32 WithAlpha(ImU32 color, uint8_t alpha) {
    return (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha) << 24);
}

// Interpolate between two colors
inline ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
    ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    ImVec4 result(
        ca.x + (cb.x - ca.x) * t,
        ca.y + (cb.y - ca.y) * t,
        ca.z + (cb.z - ca.z) * t,
        ca.w + (cb.w - ca.w) * t
    );
    return ImGui::ColorConvertFloat4ToU32(result);
}

// ============================================================================
// GAMIFIED BUTTON FEEDBACK //todo: expand this to support more complex feedback patterns (e.g. multi-stage clicks, long-press, toggle states) and integrate with a centralized input handling system for consistent state management across the UI.
// ============================================================================
struct ButtonFeedback {
    float hover_t = 0.0f;      // 0.0 = not hovered, 1.0 = fully hovered
    float active_t = 0.0f;     // 0.0 = not active, 1.0 = fully active
    float flash_t = 0.0f;      // Click flash timer
    
    void Update(float dt, bool is_hovered, bool is_active, bool was_clicked) {
        // Hover interpolation
        const float hover_target = is_hovered ? 1.0f : 0.0f;
        hover_t += (hover_target - hover_t) * std::min(1.0f, dt / UITokens::TransitionDuration);
        
        // Active interpolation
        const float active_target = is_active ? 1.0f : 0.0f;
        active_t += (active_target - active_t) * std::min(1.0f, dt / UITokens::TransitionDuration);
        
        // Click flash
        if (was_clicked) {
            flash_t = 1.0f;
        }
        if (flash_t > 0.0f) {
            flash_t = std::max(0.0f, flash_t - dt / UITokens::FeedbackFlashDuration);
        }
    }
    
    // Get current scale multiplier
    [[nodiscard]] float GetScale() const {
        float scale = 1.0f;
        scale += (UITokens::HoverScaleUp - 1.0f) * hover_t;
        scale += (UITokens::ClickScaleDown - 1.0f) * active_t;
        return scale;
    }
    
    // Get current glow intensity
    [[nodiscard]] float GetGlowIntensity() const {
        return hover_t * UITokens::HoverGlowIntensity + flash_t * 0.5f;
    }
};

} // namespace RC_UI
