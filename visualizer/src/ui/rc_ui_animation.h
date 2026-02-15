// FILE: rc_ui_animation.h
// PURPOSE: Animation helpers for Cockpit Doctrine (motion as instruction)
// PATTERN: Simple state + helpers for pulse, breathe, glow effects

#pragma once

#include <imgui.h>
#include <cstdint>

namespace RC_UI {

// ============================================================================
// ANIMATION STATE (per-panel or per-widget)
// ============================================================================

struct AnimationState {
    float time = 0.0f;                  // Accumulated time for this animation
    bool is_focused = false;            // Panel/widget has focus
    float pulse_phase = 0.0f;           // Cached pulse value (0..1)
    float breathe_alpha = 1.0f;         // Current breathing alpha
    
    void Update(float dt, bool focused) {
        time += dt;
        is_focused = focused;
        
        // Update pulse (2-4Hz for active elements)
        float pulse_freq = focused ? 3.0f : 2.0f;
        pulse_phase = 0.5f + 0.5f * sinf(time * pulse_freq * 6.28318f); // 6.28 = 2*pi
        
        // Update breathe (0.8 -> 1.0 oscillation)
        if (focused) {
            breathe_alpha = 0.8f + 0.2f * pulse_phase;
        } else {
            breathe_alpha = 1.0f;
        }
    }
    
    // Get pulsing color for borders/accents
    ImU32 GetPulsingColor(ImU32 base_color) const {
        uint8_t r = (base_color >> IM_COL32_R_SHIFT) & 0xFF;
        uint8_t g = (base_color >> IM_COL32_G_SHIFT) & 0xFF;
        uint8_t b = (base_color >> IM_COL32_B_SHIFT) & 0xFF;
        uint8_t a = static_cast<uint8_t>(pulse_phase * 255.0f);
        return IM_COL32(r, g, b, a);
    }
    
    // Get breathing alpha for window backgrounds
    float GetBreathingAlpha() const {
        return breathe_alpha;
    }
};

// ============================================================================
// ANIMATION HELPERS
// ============================================================================

class AnimationHelpers {
public:
    // Apply pulse glow to active panel borders
    static void DrawPulsingBorder(ImU32 color, float pulse_intensity = 1.0f) {
        if (!ImGui::GetCurrentContext()) return;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        const ImVec2 max = ImVec2(min.x + size.x, min.y + size.y);
        
        // Calculate pulse alpha
        float time = static_cast<float>(ImGui::GetTime());
        float pulse = 0.5f + 0.5f * sinf(time * 3.0f * 6.28318f);
        uint8_t alpha = static_cast<uint8_t>(pulse * pulse_intensity * 255.0f);
        
        ImU32 pulse_color = (color & 0x00FFFFFF) | (alpha << IM_COL32_A_SHIFT);
        
        // Draw border with pulse
        draw_list->AddRect(min, max, pulse_color, 0.0f, 0, 3.0f);
    }
    
    // Apply neon glow effect to text
    static void DrawGlowingText(const char* text, ImU32 color, float glow_intensity = 0.5f) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Draw glow shadow (offset slightly)
        uint8_t glow_alpha = static_cast<uint8_t>(glow_intensity * 128.0f);
        ImU32 glow_color = (color & 0x00FFFFFF) | (glow_alpha << IM_COL32_A_SHIFT);
        
        draw_list->AddText(ImVec2(pos.x + 1, pos.y + 1), glow_color, text);
        draw_list->AddText(ImVec2(pos.x - 1, pos.y - 1), glow_color, text);
        
        // Draw main text
        draw_list->AddText(pos, color, text);
        
        // Advance cursor
        ImGui::Dummy(ImGui::CalcTextSize(text));
    }
    
    // Breathing header for active panels
    static void BeginBreathingHeader(const char* label, ImU32 base_color, bool is_active) {
        float time = static_cast<float>(ImGui::GetTime());
        float breathe = is_active ? (0.8f + 0.2f * (0.5f + 0.5f * sinf(time * 2.0f * 6.28318f))) : 1.0f;
        
        uint8_t r = (base_color >> IM_COL32_R_SHIFT) & 0xFF;
        uint8_t g = (base_color >> IM_COL32_G_SHIFT) & 0xFF;
        uint8_t b = (base_color >> IM_COL32_B_SHIFT) & 0xFF;
        uint8_t a = static_cast<uint8_t>(breathe * 255.0f);
        
        ImU32 breathing_color = IM_COL32(r, g, b, a);
        
        ImGui::PushStyleColor(ImGuiCol_Header, breathing_color);
        ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();
    }
};

} // namespace RC_UI
