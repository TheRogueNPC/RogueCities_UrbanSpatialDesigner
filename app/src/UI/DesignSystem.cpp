// DesignSystem.cpp - Implementation of Cockpit Doctrine theme & helpers
#include "RogueCity/App/UI/DesignSystem.h"
#include <cmath>
#include <cassert>

namespace RogueCity::UI {

// ============================================================================
// THEME APPLICATION
// ============================================================================

void DesignSystem::ApplyCockpitTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // === COLORS (Y2K High-Contrast Palette) ===
    ImVec4* colors = style.Colors;
    
    // Window backgrounds
    colors[ImGuiCol_WindowBg]       = ToVec4(DesignTokens::PanelBackground);
    colors[ImGuiCol_ChildBg]        = ToVec4(DesignTokens::BackgroundDark);
    colors[ImGuiCol_PopupBg]        = ToVec4(DesignTokens::PanelBackground);
    
    // Borders (Y2K warning stripes)
    colors[ImGuiCol_Border]         = ToVec4(DesignTokens::YellowWarning);
    colors[ImGuiCol_BorderShadow]   = ToVec4(IM_COL32(0, 0, 0, 0));  // No shadow (hard edges)
    
    // Text
    colors[ImGuiCol_Text]           = ToVec4(DesignTokens::TextPrimary);
    colors[ImGuiCol_TextDisabled]   = ToVec4(DesignTokens::TextDisabled);
    
    // Headers (Neon cyan accent)
    colors[ImGuiCol_Header]         = ToVec4(WithAlpha(DesignTokens::CyanAccent, 60));
    colors[ImGuiCol_HeaderHovered]  = ToVec4(WithAlpha(DesignTokens::CyanAccent, 100));
    colors[ImGuiCol_HeaderActive]   = ToVec4(WithAlpha(DesignTokens::CyanAccent, 150));
    
    // Buttons (State-reactive)
    colors[ImGuiCol_Button]         = ToVec4(WithAlpha(DesignTokens::InfoBlue, 80));
    colors[ImGuiCol_ButtonHovered]  = ToVec4(WithAlpha(DesignTokens::InfoBlue, 120));
    colors[ImGuiCol_ButtonActive]   = ToVec4(WithAlpha(DesignTokens::InfoBlue, 180));
    
    // Selection (Magenta highlight)
    colors[ImGuiCol_FrameBg]        = ToVec4(WithAlpha(DesignTokens::BackgroundDark, 150));
    colors[ImGuiCol_FrameBgHovered] = ToVec4(WithAlpha(DesignTokens::MagentaHighlight, 60));
    colors[ImGuiCol_FrameBgActive]  = ToVec4(WithAlpha(DesignTokens::MagentaHighlight, 100));
    
    // Scrollbars (Minimal, subtle)
    colors[ImGuiCol_ScrollbarBg]    = ToVec4(WithAlpha(DesignTokens::BackgroundDark, 50));
    colors[ImGuiCol_ScrollbarGrab]  = ToVec4(WithAlpha(DesignTokens::TextSecondary, 80));
    colors[ImGuiCol_ScrollbarGrabHovered] = ToVec4(WithAlpha(DesignTokens::CyanAccent, 100));
    colors[ImGuiCol_ScrollbarGrabActive]  = ToVec4(WithAlpha(DesignTokens::CyanAccent, 150));
    
    // Tabs (Docking system)
    colors[ImGuiCol_Tab]            = ToVec4(WithAlpha(DesignTokens::PanelBackground, 180));
    colors[ImGuiCol_TabHovered]     = ToVec4(WithAlpha(DesignTokens::CyanAccent, 100));
    colors[ImGuiCol_TabActive]      = ToVec4(WithAlpha(DesignTokens::CyanAccent, 150));
    colors[ImGuiCol_TabUnfocused]   = ToVec4(WithAlpha(DesignTokens::BackgroundDark, 200));
    colors[ImGuiCol_TabUnfocusedActive] = ToVec4(WithAlpha(DesignTokens::InfoBlue, 80));
    
    // === SPACING & LAYOUT ===
    style.WindowPadding     = ImVec2(DesignTokens::SpaceM, DesignTokens::SpaceM);
    style.FramePadding      = ImVec2(DesignTokens::SpaceS, DesignTokens::SpaceXS);
    style.ItemSpacing       = ImVec2(DesignTokens::SpaceS, DesignTokens::SpaceS);
    style.ItemInnerSpacing  = ImVec2(DesignTokens::SpaceXS, DesignTokens::SpaceXS);
    
    // === BORDERS (Y2K Hard-Edged) ===
    style.WindowBorderSize  = DesignTokens::BorderThick;   // Warning stripe borders
    style.ChildBorderSize   = DesignTokens::BorderThin;    // Subtle child borders
    style.PopupBorderSize   = DesignTokens::BorderThick;   // Prominent popups
    style.FrameBorderSize   = DesignTokens::BorderNormal;  // Input frames
    
    // === ROUNDING (Minimal for Y2K) ===
    style.WindowRounding    = DesignTokens::RoundingNone;
    style.ChildRounding     = DesignTokens::RoundingNone;
    style.FrameRounding     = DesignTokens::RoundingButton;  // Slight softness on buttons
    style.PopupRounding     = DesignTokens::RoundingNone;
    style.ScrollbarRounding = DesignTokens::RoundingNone;
    style.GrabRounding      = DesignTokens::RoundingNone;
    style.TabRounding       = DesignTokens::RoundingSubtle;
    
    // === MISC ===
    style.Alpha             = 1.0f;  // No global transparency (hard-edged aesthetic)
    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);  // Left-aligned titles
    style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);  // Centered button text
}

// ============================================================================
// COLOR HELPERS
// ============================================================================

ImVec4 DesignSystem::ToVec4(ImU32 color) {
    return ImGui::ColorConvertU32ToFloat4(color);
}

ImU32 DesignSystem::WithAlpha(ImU32 color, uint8_t alpha) {
    return (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha) << 24);
}

// ============================================================================
// MOTION HELPERS
// ============================================================================

float DesignSystem::EaseOutCubic(float t) {
    return 1.0f - powf(1.0f - t, 3.0f);
}

float DesignSystem::EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float DesignSystem::PulseWave(float time_sec, float frequency_hz) {
    return 1.0f + 0.2f * sinf(time_sec * frequency_hz * 2.0f * 3.14159f);
}

// ============================================================================
// LAYOUT HELPERS
// ============================================================================

void DesignSystem::BeginPanel(const char* title, ImGuiWindowFlags flags) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ToVec4(DesignTokens::PanelBackground));
    ImGui::Begin(title, nullptr, flags);
}

void DesignSystem::EndPanel() {
    ImGui::End();
    ImGui::PopStyleColor();
}

void DesignSystem::Separator() {
    ImGui::Dummy(ImVec2(0.0f, DesignTokens::SpaceS));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, DesignTokens::SpaceS));
}

void DesignSystem::SectionHeader(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ToVec4(DesignTokens::GreenHUD));
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

// ============================================================================
// AFFORDANCE PATTERNS (State-Reactive Buttons)
// ============================================================================

void DesignSystem::ApplyButtonStyle(ImU32 base_color, ImU32 hover_color, ImU32 active_color) {
    ImGui::PushStyleColor(ImGuiCol_Button, ToVec4(base_color));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ToVec4(hover_color));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ToVec4(active_color));
}

bool DesignSystem::ButtonPrimary(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        WithAlpha(DesignTokens::InfoBlue, 120),
        WithAlpha(DesignTokens::InfoBlue, 180),
        WithAlpha(DesignTokens::InfoBlue, 220)
    );
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

bool DesignSystem::ButtonSecondary(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        WithAlpha(DesignTokens::TextSecondary, 80),
        WithAlpha(DesignTokens::TextSecondary, 120),
        WithAlpha(DesignTokens::TextSecondary, 160)
    );
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

bool DesignSystem::ButtonDanger(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        WithAlpha(DesignTokens::ErrorRed, 120),
        WithAlpha(DesignTokens::ErrorRed, 180),
        WithAlpha(DesignTokens::ErrorRed, 220)
    );
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

// ============================================================================
// STATUS DISPLAY PATTERNS (Cockpit Readouts)
// ============================================================================

void DesignSystem::StatusMessage(const char* text, bool is_error) {
    ImU32 color = is_error ? DesignTokens::ErrorRed : DesignTokens::SuccessGreen;
    ImGui::PushStyleColor(ImGuiCol_Text, ToVec4(color));
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

void DesignSystem::CoordinateDisplay(const char* label, float x, float y) {
    ImGui::PushStyleColor(ImGuiCol_Text, ToVec4(DesignTokens::GreenHUD));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("X: %.1f  Y: %.1f", x, y);
}

// ============================================================================
// VALIDATION (Debug builds)
// ============================================================================

void DesignSystem::ValidateNoHardcodedValues() {
#ifdef _DEBUG
    // This is called in dev builds to warn about hardcoded colors/spacing
    // Implementation: scan ImGui draw list for non-token colors (future)
    // For now, this is a reminder to use tokens
#endif
}

} // namespace RogueCity::UI
