// FILE: rc_ui_components.h
// PURPOSE: Token-driven, reusable UI component templates.
#pragma once

#include "ui/rc_ui_tokens.h"

#include <imgui.h>
#include <cstdarg>

namespace RC_UI::Components {

inline bool BeginTokenPanel(
    const char* title,
    ImU32 border_color = UITokens::YellowWarning,
    bool* p_open = nullptr,
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse,
    ImU32 window_bg = UITokens::PanelBackground) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(window_bg));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(WithAlpha(border_color, 180)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, UITokens::BorderNormal);
    return ImGui::Begin(title, p_open, flags);
}

inline void EndTokenPanel() {
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

inline bool AnimatedActionButton(
    const char* id,
    const char* label,
    ButtonFeedback& feedback,
    float dt,
    bool active = false,
    ImVec2 size = ImVec2(0.0f, 0.0f)) {
    const ImVec2 requested = size;
    const ImVec2 final_size(
        requested.x > 0.0f ? requested.x * feedback.GetScale() : 0.0f,
        requested.y > 0.0f ? requested.y * feedback.GetScale() : 0.0f);

    ImGui::PushID(id);
    const bool clicked = ImGui::Button(label, final_size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    feedback.Update(dt, hovered, active || held, clicked);
    if (hovered || feedback.flash_t > 0.0f) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 pmin = ImGui::GetItemRectMin();
        const ImVec2 pmax = ImGui::GetItemRectMax();
        const float glow = feedback.GetGlowIntensity();
        draw_list->AddRect(
            pmin,
            pmax,
            GetHoverGlow(UITokens::CyanAccent, glow),
            UITokens::RoundingButton,
            0,
            UITokens::BorderThin + glow * 4.0f);
    }
    ImGui::PopID();
    return clicked;
}

inline void StatusChip(const char* label, ImU32 color, bool emphasized = false) {
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const float pad_x = UITokens::SpaceS;
    const float pad_y = UITokens::SpaceXS;
    const float h = text_size.y + pad_y * 2.0f;
    const float w = text_size.x + pad_x * 2.0f;
    const float rounding = emphasized ? UITokens::RoundingChip : UITokens::RoundingButton;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(cursor, ImVec2(cursor.x + w, cursor.y + h), WithAlpha(color, emphasized ? 220 : 180), rounding);
    draw->AddRect(cursor, ImVec2(cursor.x + w, cursor.y + h), WithAlpha(UITokens::TextPrimary, 110), rounding, 0, UITokens::BorderThin);
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + pad_x, cursor.y + pad_y));
    ImGui::TextUnformatted(label);
    ImGui::SetCursorScreenPos(cursor);
    // Submit a layout item so SetCursorScreenPos does not leave the window in an invalid state.
    ImGui::Dummy(ImVec2(w, h + UITokens::SpaceXS));
}

inline void DrawScanlineBackdrop(const ImVec2& min, const ImVec2& max, float time_sec, ImU32 tint = UITokens::GreenHUD) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float height = max.y - min.y;
    if (height <= 1.0f) {
        return;
    }

    constexpr int kLines = 9;
    for (int i = 0; i < kLines; ++i) {
        const float phase = time_sec * 0.85f + static_cast<float>(i) * 0.3f;
        const float y = min.y + std::fmod(phase * 30.0f, height);
        const float alpha = 0.04f + 0.08f * GetPulseValue(phase, 0.6f);
        draw->AddLine(
            ImVec2(min.x, y),
            ImVec2(max.x, y),
            WithAlpha(tint, static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)),
            UITokens::BorderThin);
    }
}

inline void TextToken(ImU32 color, const char* fmt, ...) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Components
