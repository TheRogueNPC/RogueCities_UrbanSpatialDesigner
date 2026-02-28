// FILE: rc_ui_components.h
// PURPOSE: Token-driven, reusable UI component templates.
#pragma once

#include "ui/rc_ui_tokens.h"

#include <cstdarg>
#include <imgui.h>

namespace RC_UI::Components {

inline bool
BeginTokenPanel(const char *title, ImU32 border_color = UITokens::YellowWarning,
                bool *p_open = nullptr,
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse,
                ImU32 window_bg = UITokens::PanelBackground) {
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImGui::ColorConvertU32ToFloat4(window_bg));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(
                                             WithAlpha(border_color, 180)));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, UITokens::BorderNormal);
  return ImGui::Begin(title, p_open, flags);
}

inline void EndTokenPanel() {
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
}

inline bool AnimatedActionButton(const char *id, const char *label,
                                 ButtonFeedback &feedback, float dt,
                                 bool active = false,
                                 ImVec2 size = ImVec2(0.0f, 0.0f)) {
  const ImVec2 requested = size;
  const float min_target = 28.0f; // P0 requirement for minimum hit targets
  const ImVec2 scaled_size(
      requested.x > 0.0f ? requested.x * feedback.GetScale() : min_target,
      requested.y > 0.0f ? requested.y * feedback.GetScale() : min_target);
  const ImVec2 final_size(std::max(scaled_size.x, min_target),
                          std::max(scaled_size.y, min_target));

  ImGui::PushID(id);
  const bool clicked = ImGui::Button(label, final_size);
  const bool hovered = ImGui::IsItemHovered();
  const bool held = ImGui::IsItemActive();

  feedback.Update(dt, hovered, active || held, clicked);
  if (hovered || feedback.flash_t > 0.0f) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 pmin = ImGui::GetItemRectMin();
    const ImVec2 pmax = ImGui::GetItemRectMax();
    const float glow = feedback.GetGlowIntensity();
    draw_list->AddRect(pmin, pmax, GetHoverGlow(UITokens::CyanAccent, glow),
                       UITokens::RoundingButton, 0,
                       UITokens::BorderThin + glow * 4.0f);
  }
  ImGui::PopID();
  return clicked;
}

inline void StatusChip(const char *label, ImU32 color,
                       bool emphasized = false) {
  const ImVec2 text_size = ImGui::CalcTextSize(label);
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const float pad_x = UITokens::SpaceS;
  const float pad_y = UITokens::SpaceXS;
  const float h = text_size.y + pad_y * 2.0f;
  const float w = text_size.x + pad_x * 2.0f;
  const float rounding =
      emphasized ? UITokens::RoundingChip : UITokens::RoundingButton;

  ImDrawList *draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(cursor, ImVec2(cursor.x + w, cursor.y + h),
                      WithAlpha(color, emphasized ? 220 : 180), rounding);
  draw->AddRect(cursor, ImVec2(cursor.x + w, cursor.y + h),
                WithAlpha(UITokens::TextPrimary, 110), rounding, 0,
                UITokens::BorderThin);
  ImGui::SetCursorScreenPos(ImVec2(cursor.x + pad_x, cursor.y + pad_y));
  ImGui::TextUnformatted(label);
  ImGui::SetCursorScreenPos(cursor);
  // Submit a layout item so SetCursorScreenPos does not leave the window in an
  // invalid state.
  ImGui::Dummy(ImVec2(w, h + UITokens::SpaceXS));
}

inline void DrawScanlineBackdrop(const ImVec2 &min, const ImVec2 &max,
                                 float time_sec,
                                 ImU32 tint = UITokens::GreenHUD) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const float height = max.y - min.y;
  if (height <= 1.0f) {
    return;
  }

  constexpr int kLines = 9;
  for (int i = 0; i < kLines; ++i) {
    const float phase = time_sec * 0.85f + static_cast<float>(i) * 0.3f;
    const float y = min.y + std::fmod(phase * 30.0f, height);
    const float alpha = 0.04f + 0.08f * GetPulseValue(phase, 0.6f);
    draw->AddLine(ImVec2(min.x, y), ImVec2(max.x, y),
                  WithAlpha(tint, static_cast<uint8_t>(
                                      std::clamp(alpha, 0.0f, 1.0f) * 255.0f)),
                  UITokens::BorderThin);
  }
}

inline void TextToken(ImU32 color, const char *fmt, ...) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
  va_list args;
  va_start(args, fmt);
  ImGui::TextV(fmt, args);
  va_end(args);
  ImGui::PopStyleColor();
}

inline void BoundedText(const char *text, float padding = 8.0f) {
  ImGui::PushTextWrapPos(ImGui::GetCursorPosX() +
                         ImGui::GetContentRegionAvail().x - padding);
  ImGui::TextUnformatted(text);
  ImGui::PopTextWrapPos();
}

inline void DraggableSectionDivider(const char *label, const char *popup_id) {
  ImGui::Spacing();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 24.0f);

  ImGui::PushID(label);

  // Background and interaction
  ImGui::InvisibleButton("##drag", size);
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();

  ImU32 bg_color =
      GetHoverGlow(UITokens::PanelBackground, hovered ? 0.2f : 0.0f);
  if (active)
    bg_color = GetHoverGlow(UITokens::CyanAccent, 0.4f);

  ImDrawList *draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg_color,
                      UITokens::RoundingSubtle);

  // Draw grab handle and label
  ImGui::SetCursorScreenPos(ImVec2(pos.x + 8.0f, pos.y + 4.0f));
  TextToken(UITokens::TextSecondary, "::");
  ImGui::SameLine();
  TextToken(UITokens::TextPrimary, label);

  // Draw dropdown handle
  ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 24.0f, pos.y + 4.0f));
  TextToken(UITokens::CyanAccent, "...");
  if (ImGui::IsItemClicked()) {
    ImGui::OpenPopup(popup_id);
  }

  // Context menu
  if (ImGui::BeginPopup(popup_id)) {
    ImGui::TextDisabled("Section Actions");
    ImGui::Separator();
    if (ImGui::MenuItem("Move Up")) {
    }
    if (ImGui::MenuItem("Move Down")) {
    }
    if (ImGui::MenuItem("Collapse")) {
    }
    ImGui::EndPopup();
  }

  ImGui::PopID();

  // Submit a dummy to advance cursor past the absolute positioned elements
  ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + 4.0f));
}

/// DrawPanelFrame — overlays the Y2K corner-bracket decoration and top accent
/// bar that the mockup uses (.panel::before + .panel-corner--*).
///
/// Call INSIDE an open ImGui::Begin() / BeginChild() block, before any content.
/// Matches the mockup CSS:
///   .panel::before  → top accent bar spanning 30%-left, gap, 30%-right
///   .panel-corner   → 12px corner brackets at each corner (--secondary color)
///
/// Usage:
///   if (ImGui::Begin("My Panel", ...)) {
///       RC_UI::Components::DrawPanelFrame();
///       // ... panel content ...
///   }
///   ImGui::End();
inline void DrawPanelFrame(
    ImU32 accent_color = UITokens::CyanAccent,
    float corner_size  = 12.0f,
    float bar_alpha    = 0.60f,
    float border_w     = UITokens::BorderNormal)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pmin = ImGui::GetWindowPos();
    const ImVec2 sz   = ImGui::GetWindowSize();
    const ImVec2 pmax = ImVec2(pmin.x + sz.x, pmin.y + sz.y);

    const ImU32 accent_dim = WithAlpha(accent_color, static_cast<uint8_t>(bar_alpha * 255.0f));

    // Top accent bar: 30% left segment, 40% gap, 30% right segment
    // Matches: background: linear-gradient(90deg, --secondary 30%, transparent, --secondary 70%)
    const float seg = sz.x * 0.30f;
    draw->AddRectFilled(
        ImVec2(pmin.x,         pmin.y),
        ImVec2(pmin.x + seg,   pmin.y + border_w),
        accent_dim);
    draw->AddRectFilled(
        ImVec2(pmax.x - seg,   pmin.y),
        ImVec2(pmax.x,         pmin.y + border_w),
        accent_dim);

    // Corner brackets — 2px lines, 12px arms, full accent color
    const float bw = border_w;
    const float cs = corner_size;

    // Top-left
    draw->AddLine(ImVec2(pmin.x,      pmin.y),      ImVec2(pmin.x + cs, pmin.y),      accent_color, bw);
    draw->AddLine(ImVec2(pmin.x,      pmin.y),      ImVec2(pmin.x,      pmin.y + cs), accent_color, bw);
    // Top-right
    draw->AddLine(ImVec2(pmax.x,      pmin.y),      ImVec2(pmax.x - cs, pmin.y),      accent_color, bw);
    draw->AddLine(ImVec2(pmax.x,      pmin.y),      ImVec2(pmax.x,      pmin.y + cs), accent_color, bw);
    // Bottom-left
    draw->AddLine(ImVec2(pmin.x,      pmax.y),      ImVec2(pmin.x + cs, pmax.y),      accent_color, bw);
    draw->AddLine(ImVec2(pmin.x,      pmax.y),      ImVec2(pmin.x,      pmax.y - cs), accent_color, bw);
    // Bottom-right
    draw->AddLine(ImVec2(pmax.x,      pmax.y),      ImVec2(pmax.x - cs, pmax.y),      accent_color, bw);
    draw->AddLine(ImVec2(pmax.x,      pmax.y),      ImVec2(pmax.x,      pmax.y - cs), accent_color, bw);
}

/// DrawSectionHeader — draws a left color-bar LCARS-style section header,
/// matching the mockup's .section-header::before stripe.
/// Call where a CollapsingHeader would go; returns true if the section is open.
inline bool DrawSectionHeader(const char* label, ImU32 bar_color,
                              bool default_open = true)
{
    // Left accent stripe (4px wide, full item height)
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const float  h = text_size.y + 8.0f;    // ~FramePadding.y * 2 + text

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(
        cursor,
        ImVec2(cursor.x + 4.0f, cursor.y + h),
        WithAlpha(bar_color, 220));

    // Indent past the stripe, then draw a tree node
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + 8.0f, cursor.y));
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (default_open) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(bar_color));
    const bool open = ImGui::TreeNodeEx(label, flags);
    ImGui::PopStyleColor();
    if (open) ImGui::TreePop();

    // Restore cursor to below the header row
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + h + 2.0f));
    return open;
}

/// DrawMeter - renders a stylish, thin progress bar mimicking the mockup .meter-bar
inline void DrawMeter(const char* label, float value, ImU32 fill_color, const char* value_text = nullptr) {
    ImGui::PushID(label);
    
    // Label
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s", label);
    ImGui::SameLine(90.0f); // Fixed width for label (min-width: 80px in mockup)
    
    // Meter Bar
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.y += (ImGui::GetTextLineHeight() - 6.0f) * 0.5f; // Center vertically
    float bar_width = ImGui::GetContentRegionAvail().x - 45.0f; // Leave room for value text
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    // Background track
    draw->AddRectFilled(pos, ImVec2(pos.x + bar_width, pos.y + 6.0f), 
                        WithAlpha(UITokens::BackgroundDark, 200), UITokens::RoundingSubtle);
    
    // Fill
    float fill_w = bar_width * std::clamp(value, 0.0f, 1.0f);
    if (fill_w > 0) {
        draw->AddRectFilled(pos, ImVec2(pos.x + fill_w, pos.y + 6.0f), 
                            fill_color, UITokens::RoundingSubtle);
        
        // Add glow effect if it's not a disabled/dim color
        if (fill_color != UITokens::TextDisabled) {
            draw->AddRect(pos, ImVec2(pos.x + fill_w, pos.y + 6.0f), 
                          WithAlpha(fill_color, 100), UITokens::RoundingSubtle, 0, 2.0f);
        }
    }
    
    ImGui::Dummy(ImVec2(bar_width, 6.0f));
    
    // Value text
    ImGui::SameLine();
    char buf[32];
    if (!value_text) {
        snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(value * 100.0f));
        value_text = buf;
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(value_text).x);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s", value_text);
    
    ImGui::PopID();
}

/// DrawDiagRow - renders a simple key-value diagnostic row
inline void DrawDiagRow(const char* label, const char* value, ImU32 value_color = UITokens::TextPrimary) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s", label);
    ImGui::SameLine();
    
    float val_width = ImGui::CalcTextSize(value).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - val_width);
    
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(value_color), "%s", value);
}

} // namespace RC_UI::Components
