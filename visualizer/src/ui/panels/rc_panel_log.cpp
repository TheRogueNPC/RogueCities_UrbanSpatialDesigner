// FILE: visualizer/src/ui/panels/rc_panel_log.cpp
// (RogueCities_UrbanSpatialDesigner) PURPOSE: Floating contextual log with Y2K
// reveal animations and hover-expansion.
#include "ui/panels/rc_panel_log.h"

#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_tokens.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Infomatrix.hpp>

#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <sstream>
#include <string>
#include <vector>

namespace RC_UI::Panels::Log {

namespace {
struct RuntimeSnapshot {
  uint32_t roads = 0;
  uint32_t districts = 0;
  uint32_t lots = 0;
  uint32_t buildings = 0;
  bool plan_approved = true;
  bool dirty_any = false;
};

static RuntimeSnapshot s_prev{};
static bool s_initialized = false;
static uint32_t s_last_event_count = 0;
static float s_reveal_timer = 0.0f;
static bool s_hover_expanded = false;

void CaptureRuntimeEvents() {
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  using RogueCity::Core::Editor::InfomatrixEvent;

  RuntimeSnapshot now{};
  now.roads = static_cast<uint32_t>(gs.roads.size());
  now.districts = static_cast<uint32_t>(gs.districts.size());
  now.lots = static_cast<uint32_t>(gs.lots.size());
  now.buildings = static_cast<uint32_t>(gs.buildings.size());
  now.plan_approved = gs.plan_approved;
  now.dirty_any = gs.dirty_layers.AnyDirty();

  if (!s_initialized) {
    s_initialized = true;
    s_prev = now;
    gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime,
                            "[BOOT] Event stream online");
    return;
  }

  if (now.roads != s_prev.roads || now.districts != s_prev.districts ||
      now.lots != s_prev.lots || now.buildings != s_prev.buildings) {
    std::ostringstream line;
    line << "[GEN] roads=" << now.roads << " districts=" << now.districts
         << " lots=" << now.lots << " buildings=" << now.buildings;
    gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime, line.str());
  }

  if (now.plan_approved != s_prev.plan_approved) {
    gs.infomatrix.pushEvent(InfomatrixEvent::Category::Validation,
                            now.plan_approved ? "[VALIDATION] plan approved"
                                              : "[VALIDATION] plan rejected");
  }

  if (now.dirty_any != s_prev.dirty_any) {
    gs.infomatrix.pushEvent(InfomatrixEvent::Category::Dirty,
                            now.dirty_any ? "[DIRTY] regeneration pending"
                                          : "[DIRTY] all clean");
  }

  s_prev = now;
}
} // namespace

void DrawContent(float dt) {
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  auto &gs = RogueCity::Core::Editor::GetGlobalState();

  CaptureRuntimeEvents();

  auto es = gs.infomatrix.events();
  if (es.data.size() > s_last_event_count) {
    s_reveal_timer = 0.0f; // Reset reveal animation for new entry
    s_last_event_count = static_cast<uint32_t>(es.data.size());
  }
  s_reveal_timer += dt;

  const bool hovered =
      ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
  static float expand_t = 0.0f;
  expand_t =
      std::clamp(expand_t + (hovered ? dt * 4.0f : -dt * 2.0f), 0.0f, 1.0f);

  const int base_lines = 5;
  const int max_lines =
      std::max(base_lines, gs.config.contextual_log_expanded_lines);
  const int visible_lines =
      static_cast<int>(base_lines + (max_lines - base_lines) * expand_t);
  const float line_height = ImGui::GetTextLineHeightWithSpacing();
  const float panel_h = visible_lines * line_height + 40.0f;

  // Street Sweeper reveal logic
  const float reveal_duration = 0.4f;
  const float reveal_p =
      std::clamp(s_reveal_timer / reveal_duration, 0.0f, 1.0f);

  ImGui::BeginChild("LogStream", ImVec2(0.0f, panel_h - 40.0f), false,
                    ImGuiWindowFlags_NoScrollbar);

  const size_t start_idx = (es.data.size() > static_cast<size_t>(visible_lines))
                               ? (es.data.size() - visible_lines)
                               : 0;

  for (size_t i = start_idx; i < es.data.size(); ++i) {
    const auto &ev = es.data[i];
    ImU32 color = UITokens::TextPrimary;
    using RogueCity::Core::Editor::InfomatrixEvent;
    switch (ev.cat) {
    case InfomatrixEvent::Category::Runtime:
      color = UITokens::CyanAccent;
      break;
    case InfomatrixEvent::Category::Validation:
      color = ev.msg.find("rejected") != std::string::npos ? UITokens::ErrorRed
                                                           : UITokens::GreenHUD;
      break;
    case InfomatrixEvent::Category::Dirty:
      color = UITokens::YellowWarning;
      break;
    default:
      break;
    }

    // Apply reveal only to the LAST entry if it's new
    std::string msg = ev.msg;
    if (i == es.data.size() - 1 && reveal_p < 1.0f) {
      const int len = static_cast<int>(msg.length());
      const int visible_chars = static_cast<int>(reveal_p * len);
      msg = msg.substr(0, visible_chars);
    }

    // Parse [PREFIX] to colorize it
    size_t bracket_start = msg.find('[');
    size_t bracket_end = msg.find(']');
    
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Make sure it uses mono/default
    if (bracket_start == 0 && bracket_end != std::string::npos) {
        std::string prefix = msg.substr(1, bracket_end - 1);
        std::string rest = msg.substr(bracket_end + 1);
        
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "[");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "%s", prefix.c_str());
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "]");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s", rest.c_str());
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary), "%s", msg.c_str());
    }
    ImGui::PopFont();
  }

  ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();

  uiint.RegisterWidget(
      {"log", "ContextualLog", "log.contextual", {"log", "floating"}});
}

void Draw(float dt) {
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  if (!gs.config.show_contextual_log)
    return;

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  const float width = 500.0f;
  const float margin = 20.0f;

  // Position at bottom center
  ImVec2 pos(viewport->Pos.x + (viewport->Size.x - width) * 0.5f,
             viewport->Pos.y + viewport->Size.y - margin);

  // We need to calculate height based on hover state
  const bool hovered = ImGui::IsMouseHoveringRect(
      ImVec2(pos.x, pos.y - 120.0f), ImVec2(pos.x + width, pos.y), false);

  static float h_anim = 80.0f;
  const float h_max =
      static_cast<float>(gs.config.contextual_log_expanded_lines) *
          ImGui::GetTextLineHeightWithSpacing() +
      40.0f;
  h_anim =
      std::clamp(h_anim + (hovered ? dt * 400.0f : -dt * 200.0f), 80.0f, h_max);

  ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y - h_anim), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(width, h_anim), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.70f);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                           ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoSavedSettings;

  // If hovered, allow inputs for scrolling
  if (hovered) {
    flags &= ~ImGuiWindowFlags_NoInputs;
  }

  if (RC_UI::Components::BeginTokenPanel("Contextual Log###FloatingLog",
                                         UITokens::InfoBlue, nullptr, flags)) {
    DrawContent(dt);
  }
  RC_UI::Components::EndTokenPanel();
}

int GetEventCount() {
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
  return static_cast<int>(gs.infomatrix.events().data.size());
}

} // namespace RC_UI::Panels::Log
