// FILE: visualizer/src/ui/panels/rc_panel_log.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Event log panel with runtime generation and editing events.
#include "ui/panels/rc_panel_log.h"

#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>
#include <deque>
#include <sstream>
#include <string>

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

    static std::deque<std::string> s_log_lines{};
    static RuntimeSnapshot s_prev{};
    static bool s_initialized = false;

    void PushLog(const std::string& line) {
        s_log_lines.push_back(line);
        constexpr size_t kMaxLines = 220u;
        while (s_log_lines.size() > kMaxLines) {
            s_log_lines.pop_front();
        }
    }

    void CaptureRuntimeEvents() {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();
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
            PushLog("[BOOT] Event stream online");
            return;
        }

        if (now.roads != s_prev.roads || now.districts != s_prev.districts ||
            now.lots != s_prev.lots || now.buildings != s_prev.buildings) {
            std::ostringstream line;
            line << "[GEN] roads=" << now.roads
                 << " districts=" << now.districts
                 << " lots=" << now.lots
                 << " buildings=" << now.buildings;
            PushLog(line.str());
        }

        if (now.plan_approved != s_prev.plan_approved) {
            PushLog(now.plan_approved ? "[VALIDATION] plan approved" : "[VALIDATION] plan rejected");
        }

        if (now.dirty_any != s_prev.dirty_any) {
            PushLog(now.dirty_any ? "[DIRTY] regeneration pending" : "[DIRTY] all clean");
        }

        s_prev = now;
    }
} // namespace

void DrawContent(float dt)
{
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    CaptureRuntimeEvents();

    static ReactiveF flash;
    constexpr float kGlowBase = 0.2f;
    constexpr float kGlowRange = 0.5f;
    flash.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    flash.Update(dt);

    const ImVec4 glow = ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::CyanAccent, static_cast<uint8_t>((kGlowBase + kGlowRange * flash.v) * 255.0f)));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, glow);
    const bool pin_to_latest = true;
    if (ImGui::Button("Clear")) {
        s_log_lines.clear();
        PushLog("[LOG] cleared");
    }
    ImGui::SameLine();
    Components::StatusChip("LIVE", UITokens::SuccessGreen, true);
    ImGui::Separator();

    ImGui::BeginChild("LogStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (const std::string& line : s_log_lines) {
        ImU32 color = UITokens::TextPrimary;
        if (line.rfind("[VALIDATION]", 0) == 0 && line.find("rejected") != std::string::npos) {
            color = UITokens::ErrorRed;
        } else if (line.rfind("[DIRTY]", 0) == 0) {
            color = UITokens::YellowWarning;
        } else if (line.rfind("[GEN]", 0) == 0) {
            color = UITokens::CyanAccent;
        } else if (line.rfind("[BOOT]", 0) == 0) {
            color = UITokens::GreenHUD;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
        ImGui::TextUnformatted(line.c_str());
        ImGui::PopStyleColor();
    }
    if (pin_to_latest) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    uiint.RegisterWidget({"tree", "LogStream", "log.tail", {"log"}});
}

void Draw(float dt)
{
    const bool open = Components::BeginTokenPanel("Log", UITokens::AmberGlow);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Log",
            "Log",
            "log",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_log.cpp",
            {"events", "telemetry"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }
    
    DrawContent(dt);
    
    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::Log
