// FILE: rc_panel_ai_console.cpp
// PURPOSE: AI Bridge runtime control and status monitoring

#include "rc_panel_ai_console.h"
#include "runtime/AiBridgeRuntime.h"
#include "runtime/AiAvailability.h"
#include "config/AiConfig.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_components.h"
#include <imgui.h>

namespace RogueCity::UI {

// === UNLOCK FEATURES MOCK UI ===
static void DrawUnlockFeaturesMock() {
    auto& gs = Core::Editor::GetGlobalState();
    
    // Y2K styled lock icon area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, DesignSystem::ToVec4(DesignTokens::PanelBackground));
    ImGui::BeginChild("LockArea", ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar);
    
    ImGui::SetCursorPosY(20);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Increase font size if available
    
    const char* lockIcon = "[ LOCKED ]";
    ImVec2 textSize = ImGui::CalcTextSize(lockIcon);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - textSize.x) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
    ImGui::Text("%s", lockIcon);
    ImGui::PopStyleColor();
    ImGui::PopFont();
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    DesignSystem::Separator();
    DesignSystem::SectionHeader("AI Bridge Unavailable");
    
    const char* reason = AI::AiAvailability::GetUnavailableReason();
    if (reason && reason[0]) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::ErrorRed));
        ImGui::TextWrapped("Reason: %s", reason);
        ImGui::PopStyleColor();
        DesignSystem::Separator();
    }
    
    ImGui::TextWrapped("AI features require additional dependencies:");

    ImGui::TextWrapped("AI features require additional dependencies:");
    ImGui::Spacing();
    ImGui::Bullet(); ImGui::Text("Python 3.10+ with FastAPI");
    ImGui::Bullet(); ImGui::Text("WinHTTP.dll (Windows HTTP Services)");
    ImGui::Bullet(); ImGui::Text("AI Bridge toolserver (tools/toolserver.py)");
    ImGui::Spacing();
    
    DesignSystem::Separator();
    
    ImGui::TextWrapped("Enable Dev Mode in the Dev Shell panel to unlock AI features (requires runtime dependencies).");
    ImGui::Spacing();
    
    if (gs.config.dev_mode_enabled) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::SuccessGreen));
        ImGui::Text("Dev Mode: ENABLED");
        ImGui::PopStyleColor();
        ImGui::TextWrapped("AI features unlocked but dependencies are missing. Install Python and run Start_Ai_Bridge_Fixed.ps1");
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::TextSecondary));
        ImGui::Text("Dev Mode: DISABLED");
        ImGui::PopStyleColor();
    }
}

void AiConsolePanel::RenderContent() {
    auto& gs = Core::Editor::GetGlobalState();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    // Check availability first
    bool ai_available = AI::AiAvailability::IsAvailable();
    bool unlocked = gs.config.dev_mode_enabled || ai_available;
    
    if (!unlocked) {
        DrawUnlockFeaturesMock();
        return;
    }
    
    if (!ai_available) {
        // Dev mode enabled but dependencies missing
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
        ImGui::TextWrapped("Dev Mode enabled but AI bridge dependencies not found.");
        ImGui::PopStyleColor();
        DesignSystem::Separator();
        DrawUnlockFeaturesMock();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    auto& config = AI::AiConfigManager::Instance().GetConfig();
    
    // === STATUS DISPLAY ===
    DesignSystem::SectionHeader("Bridge Status");
    
    ImU32 statusColor = runtime.IsOnline() ? RC_UI::UITokens::SuccessGreen : RC_UI::UITokens::YellowWarning;
    if (runtime.GetStatus() == AI::BridgeStatus::Failed) {
        statusColor = RC_UI::UITokens::ErrorRed;
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(statusColor));
    ImGui::Text("Status: %s", runtime.GetStatusString().c_str());
    ImGui::PopStyleColor();
    
    if (runtime.GetStatus() == AI::BridgeStatus::Failed && !runtime.GetLastError().empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(RC_UI::UITokens::ErrorRed));
        ImGui::Text("Error: %s", runtime.GetLastError().c_str());
        ImGui::PopStyleColor();
    }
    
    DesignSystem::Separator();
    
    // === CONTROL BUTTONS ===
    DesignSystem::SectionHeader("Control");
    
    ImGui::BeginDisabled(runtime.IsOnline() || runtime.GetStatus() == AI::BridgeStatus::Starting);
    if (DesignSystem::ButtonPrimary("Start AI Bridge", ImVec2(180, 30))) {
        runtime.StartBridge();
    }
    uiint.RegisterWidget({"button", "Start AI Bridge", "action:ai.bridge.start", {"action", "ai"}});
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    ImGui::BeginDisabled(!runtime.IsOnline());
    if (DesignSystem::ButtonDanger("Stop AI Bridge", ImVec2(180, 30))) {
        runtime.StopBridge();
    }
    uiint.RegisterWidget({"button", "Stop AI Bridge", "action:ai.bridge.stop", {"action", "ai"}});
    ImGui::EndDisabled();
    
    DesignSystem::Separator();
    
    // === MODEL CONFIGURATION ===
    if (ImGui::CollapsingHeader("Model Configuration")) {
        ImGui::Text("UI Agent:      %s", config.uiAgentModel.c_str());
        ImGui::Text("City Spec:     %s", config.citySpecModel.c_str());
        ImGui::Text("Code Assistant: %s", config.codeAssistantModel.c_str());
        ImGui::Text("Naming:        %s", config.namingModel.c_str());
    }
    
    // === CONNECTION INFO ===
    if (ImGui::CollapsingHeader("Connection")) {
        ImGui::Text("Bridge URL: %s", config.bridgeBaseUrl.c_str());
        ImGui::Text("Health Check Timeout: %d sec", config.healthCheckTimeoutSec);
        ImGui::Text("PowerShell Preference: %s", config.preferPwsh ? "pwsh" : "powershell");
    }

}

void AiConsolePanel::Render() {
    RC_UI::ApplyUnifiedWindowSchema();
    const bool open = RC_UI::Components::BeginTokenPanel(
        "AI Console",
        RC_UI::UITokens::InfoBlue,
        &m_showWindow,
        ImGuiWindowFlags_NoCollapse);
    RC_UI::PopUnifiedWindowSchema();
    RC_UI::BeginUnifiedTextWrap();

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "AI Console",
            "AI Console",
            "settings",
            "Floating",
            "visualizer/src/ui/panels/rc_panel_ai_console.cpp",
            {"ai", "bridge", "control"}
        },
        open
    );

    if (!open) {
        RC_UI::EndUnifiedTextWrap();
        uiint.EndPanel();
        RC_UI::Components::EndTokenPanel();
        return;
    }
    
    RenderContent();
    
    uiint.EndPanel();
    RC_UI::EndUnifiedTextWrap();
    RC_UI::Components::EndTokenPanel();
}

} // namespace RogueCity::UI

// === NAMESPACE-LEVEL DRAW FUNCTION FOR DRAWER PATTERN ===
namespace RC_UI::Panels::AiConsole {

void DrawContent(float dt) {
    // Reuse the class method for now
    static RogueCity::UI::AiConsolePanel panel_instance;
    panel_instance.RenderContent();
}

} // namespace RC_UI::Panels::AiConsole
