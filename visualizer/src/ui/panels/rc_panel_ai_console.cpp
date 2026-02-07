// FILE: rc_panel_ai_console.cpp
// PURPOSE: AI Bridge runtime control and status monitoring

#include "rc_panel_ai_console.h"
#include "runtime/AiBridgeRuntime.h"
#include "config/AiConfig.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>

namespace RogueCity::UI {

void AiConsolePanel::Render() {
    const bool open = ImGui::Begin("AI Console", &m_showWindow, ImGuiWindowFlags_NoCollapse);

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
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    auto& config = AI::AiConfigManager::Instance().GetConfig();
    
    // === STATUS DISPLAY ===
    DesignSystem::SectionHeader("Bridge Status");
    
    ImU32 statusColor = runtime.IsOnline() ? DesignTokens::SuccessGreen : DesignTokens::YellowWarning;
    if (runtime.GetStatus() == AI::BridgeStatus::Failed) {
        statusColor = DesignTokens::ErrorRed;
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(statusColor));
    ImGui::Text("Status: %s", runtime.GetStatusString().c_str());
    ImGui::PopStyleColor();
    
    if (runtime.GetStatus() == AI::BridgeStatus::Failed && !runtime.GetLastError().empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::ErrorRed));
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

    uiint.EndPanel();
    ImGui::End();
}

} // namespace RogueCity::UI
