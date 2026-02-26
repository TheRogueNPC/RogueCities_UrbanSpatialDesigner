/**
 * @file DesignSystem.h
 * @brief Defines the Cockpit Doctrine design system for RogueCity UI.
 *
 * This header enforces a cohesive, affordance-rich Y2K-inspired design language across the application.
 * All UI code must reference the tokens and helpers defined here—no hardcoded colors, spacing, or timing
 * outside this file. Static analysis and code review guidelines should ensure adherence.
 *
 * @section DesignTokens
 * - Centralized palette for colors, spacing, border widths, corner rounding, animation timing, and typography.
 * - All values are inspired by aviation/spacecraft HUDs: neon accents, hard edges, high contrast, and instructional motion.
 * - Spacing is strictly multiples of 8px (or 4px for tight layouts).
 *
 * @section DesignSystem
 * - Provides theme application, color conversion, validation, motion helpers, layout helpers, and affordance patterns.
 * - Ensures all ImGui style settings are defined in one place.
 * - Includes validation functions to detect and warn about hardcoded values.
 *
 * @section UI
 * - Safe wrapper surface for editor/panel code, abstracting ImGui calls.
 * - Standardizes panel, input, spacing, and text patterns for consistency.
 *
 * @section DebugMacros
 * - Macros for compile-time enforcement of design token usage (debug builds only).
 *
 * @section CockpitDoctrinePrinciples
 * - Y2K Geometry: Hard edges, neon accents, warning stripes, grid overlays.
 * - Motion as Instruction: Animations teach meaning, not decoration.
 * - Affordance-Rich: Visible state changes, responsive elements, no hidden modes.
 * - State-Reactive: Layout adapts to system state, status always visible.
 * - Diegetic UI: Cockpit/instrument panel metaphor, coordinate readouts, control surfaces.
 *
 * @note
 * All UI styling and layout must reference DesignTokens. No hardcoded values allowed.
 * This is essential for maintaining a consistent, easily updatable design system.
 */
 // Enforces Cockpit Doctrine: Y2K aesthetics, motion-as-instruction, affordance-rich
 
#pragma once
#include <imgui.h>
#include <cstdint>
#include <cstddef>
#include <string>
//todo [[MISSION CRITICAL]] ensure that all UI code references DesignTokens for colors, spacing, and timing, and that no hardcoded values are used outside of this file. This is essential for maintaining a consistent design language across the entire application and for enabling easy updates to the design system in the future. Additionally, consider implementing static analysis checks or code review guidelines to enforce this rule and prevent design drift over time. The DesignSystem class should be the only place where ImGui style settings are defined, and all UI components should reference the tokens defined in DesignTokens for their styling needs. This will help ensure that our UI remains cohesive and true to our Cockpit Doctrine design principles as we continue to develop and expand the application.
namespace RogueCity::UI {

// ============================================================================
// COCKPIT DOCTRINE DESIGN TOKENS
// ============================================================================
// These tokens are the ONLY place colors, spacing, and timing should be defined.
// All UI code MUST reference these tokens, never hardcode values.

struct DesignTokens {
    // === Y2K COLOR PALETTE ===
    // Hard-edged, high-contrast, inspired by aviation/spacecraft HUDs
    
    // Primary Colors (Neon accents)
    static constexpr ImU32 CyanAccent       = IM_COL32(0, 255, 255, 255);    // Roads, active elements
    static constexpr ImU32 GreenHUD         = IM_COL32(0, 255, 128, 255);    // Success, navigation labels
    static constexpr ImU32 YellowWarning    = IM_COL32(255, 200, 0, 255);    // Borders, warnings
    static constexpr ImU32 MagentaHighlight = IM_COL32(255, 0, 255, 255);    // Selection, focus
    
    // Background Colors (Deep, low-luminance)
    static constexpr ImU32 BackgroundDark   = IM_COL32(15, 20, 30, 255);     // Main viewport bg
    static constexpr ImU32 PanelBackground  = IM_COL32(20, 25, 35, 255);     // Panel bg
    static constexpr ImU32 GridOverlay      = IM_COL32(40, 50, 70, 100);     // Subtle grid lines
    
    // Text Colors (High contrast for readability)
    static constexpr ImU32 TextPrimary      = IM_COL32(255, 255, 255, 255);  // Main text
    static constexpr ImU32 TextSecondary    = IM_COL32(180, 180, 180, 255);  // Secondary info
    static constexpr ImU32 TextDisabled     = IM_COL32(100, 100, 100, 255);  // Disabled text
    
    // State Colors (Feedback)
    static constexpr ImU32 ErrorRed         = IM_COL32(255, 50, 50, 255);    // Errors, delete
    static constexpr ImU32 SuccessGreen     = IM_COL32(0, 255, 100, 255);    // Success messages
    static constexpr ImU32 InfoBlue         = IM_COL32(100, 150, 255, 255);  // Info messages
    
    // === SPACING SYSTEM (8px base unit) ===
    // All spacing MUST be multiples of 8px for consistency
    static constexpr float SpaceXS   = 4.0f;    // 0.5 units (tight)
    static constexpr float SpaceS    = 8.0f;    // 1 unit (default)
    static constexpr float SpaceM    = 16.0f;   // 2 units (medium)
    static constexpr float SpaceL    = 24.0f;   // 3 units (large)
    static constexpr float SpaceXL   = 32.0f;   // 4 units (extra large)
    
    // === BORDER WIDTHS (Y2K hard-edged style) ===
    static constexpr float BorderThin   = 1.0f;  // Subtle dividers
    static constexpr float BorderNormal = 2.0f;  // Standard borders
    static constexpr float BorderThick  = 3.0f;  // Warning stripes, emphasis
    
    // === CORNER ROUNDING (Minimal for Y2K aesthetic) ===
    static constexpr float RoundingNone    = 0.0f;  // Hard edges (default)
    static constexpr float RoundingSubtle  = 2.0f;  // Slight softness (rare)
    static constexpr float RoundingButton  = 4.0f;  // Buttons only
    
    // === ANIMATION TIMING (Motion as Instruction) ===
    // All animations teach meaning, not just decoration
    static constexpr float AnimExpansion = 0.8f;  // Ring expansion (teaches radius)
    static constexpr float AnimResize    = 0.3f;  // Knob resize (instant feedback)
    static constexpr float AnimPulse     = 2.0f;  // Hz (breathing UI, shows activity)
    static constexpr float AnimFade      = 0.5f;  // Panel fade in/out
    
    // === TYPOGRAPHY (Monospace for cockpit feel) ===
    static constexpr float FontSizeSmall   = 12.0f;
    static constexpr float FontSizeMedium  = 14.0f;
    static constexpr float FontSizeLarge   = 18.0f;
    static constexpr float FontSizeTitle   = 24.0f;
};

// ============================================================================
// COCKPIT DOCTRINE ENFORCEMENT
// ============================================================================
// These functions apply the design system and prevent drift

class DesignSystem {
public:
    // === THEME APPLICATION ===
    // Call once at startup to set ImGui style
    static void ApplyCockpitTheme();
    
    // === COLOR HELPERS ===
    // Convert token to ImVec4 (for ImGui style settings)
    static ImVec4 ToVec4(ImU32 color);
    
    // Alpha variants (for hover/disabled states)
    static ImU32 WithAlpha(ImU32 color, uint8_t alpha);
    
    // === VALIDATION ===
    // Warn if hardcoded colors/spacing detected (dev builds only)
    static void ValidateNoHardcodedValues();
    
    // === MOTION HELPERS ===
    // Standard easing functions for animations
    static float EaseOutCubic(float t);     // Ring expansion
    static float EaseInOutQuad(float t);    // Smooth transitions
    static float PulseWave(float time_sec, float frequency_hz);
    
    // === LAYOUT HELPERS ===
    // Standard spacing patterns
    static void BeginPanel(const char* title, ImGuiWindowFlags flags = 0);
    static void EndPanel();
    
    static void Separator();  // With proper spacing
    static void SectionHeader(const char* text);
    
    // === AFFORDANCE PATTERNS ===
    // Standard UI elements that teach interaction
    static bool ButtonPrimary(const char* label, ImVec2 size = ImVec2(0, 0));
    static bool ButtonSecondary(const char* label, ImVec2 size = ImVec2(0, 0));
    static bool ButtonDanger(const char* label, ImVec2 size = ImVec2(0, 0));
    
    static void StatusMessage(const char* text, bool is_error = false);
    static void CoordinateDisplay(const char* label, float x, float y);
    
private:
    static void ApplyButtonStyle(ImU32 base_color, ImU32 hover_color, ImU32 active_color);
};

// ============================================================================
// SAFE UI WRAPPER SURFACE
// ============================================================================
// Prefer this API in editor/panel code instead of raw style/window setup.
class UI {
public:
    static bool BeginPanel(const char* title, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    static void EndPanel();

    static bool InputText(const char* label, std::string& text, size_t max_chars = 512);
    static bool InputFloat(const char* label, float& value, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f");
    static bool Checkbox(const char* label, bool& value);
    static bool ColorEdit(const char* label, ImVec4& color, ImGuiColorEditFlags flags = 0);
    static bool BeginCombo(const char* label, const char* preview, ImGuiComboFlags flags = 0);
    static void EndCombo();

    static void Space();
    static void SpaceM();
    static void SpaceL();

    static void TextPrimary(const char* fmt, ...);
    static void TextSecondary(const char* fmt, ...);
    static void TextSuccess(const char* fmt, ...);
    static void TextError(const char* fmt, ...);
    static void TextWarning(const char* fmt, ...);
};

// ============================================================================
// DESIGN VALIDATION MACROS (Debug builds only)
// ============================================================================
#ifdef _DEBUG
    #define DESIGN_ASSERT_NO_HARDCODED_COLOR(color) \
        static_assert(false, "Use DesignTokens::* instead of hardcoded ImU32/ImVec4")
    
    #define DESIGN_ASSERT_SPACING_MULTIPLE(value) \
        static_assert(((int)(value) % 8) == 0 || ((int)(value) % 4) == 0, \
                      "Spacing must be multiple of 4px or 8px")
#else
    #define DESIGN_ASSERT_NO_HARDCODED_COLOR(color)
    #define DESIGN_ASSERT_SPACING_MULTIPLE(value)
#endif

// ============================================================================
// COCKPIT DOCTRINE PRINCIPLES (Documentation)
// ============================================================================
/*
 * PRINCIPLE 1: Y2K GEOMETRY
 * - Hard-edged UI elements (minimal rounding)
 * - High-contrast neon accents on dark backgrounds
 * - Warning stripe borders (3px yellow) for emphasis
 * - Grid overlays for depth cues
 * 
 * PRINCIPLE 2: MOTION AS INSTRUCTION
 * - Ring expansion (0.8s) teaches axiom radius without tooltips
 * - Knob hover glow shows draggability without cursor change
 * - Pulse animations (2 Hz) indicate active/live state
 * - All motion has instructional purpose, not decoration
 * 
 * PRINCIPLE 3: AFFORDANCE-RICH
 * - UI elements visibly respond to hover/drag/click
 * - State changes are immediate and visible
 * - No hidden modes or invisible interactions
 * - Interface "breathes" and feels alive
 * 
 * PRINCIPLE 4: STATE-REACTIVE
 * - Panel layouts change based on HFSM state
 * - Docking adapts to current tool/mode
 * - Status always visible (no "what's happening?" moments)
 * - System state drives UI, not user memory
 * 
 * PRINCIPLE 5: DIEGETIC UI
 * - Interface as cockpit/instrument panel metaphor
 * - Coordinate readouts, navigation labels (NAV, HUD)
 * - Context popups as control panel readouts
 * - No "forms" or "dialogs" - everything is a control surface
 */

} // namespace RogueCity::UI
