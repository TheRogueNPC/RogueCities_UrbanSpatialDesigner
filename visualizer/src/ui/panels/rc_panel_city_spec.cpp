// FILE: rc_panel_city_spec.cpp
// PURPOSE: CitySpec Generator for AI-driven city design

#include "rc_panel_city_spec.h"
#include "client/CitySpecClient.h"
#include "runtime/AiBridgeRuntime.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>
#include <algorithm>
#include <thread>
#include <cmath>

namespace RogueCity::UI {

static void RenderBusyIndicator(std::atomic<bool>& busyFlag, float& busyTimeSeconds) {
    if (!busyFlag.load()) return;

    busyTimeSeconds += ImGui::GetIO().DeltaTime;
    float t = (sinf(busyTimeSeconds * 3.14f) * 0.5f) + 0.5f; // 0..1
    float alpha = 0.3f + 0.7f * t;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, alpha));
    ImGui::Text("AI processing...");
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void CitySpecPanel::Render() {
    const bool open = ImGui::Begin("City Spec Generator", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "City Spec Generator",
            "City Spec Generator",
            "toolbox",
            "Floating",
            "visualizer/src/ui/panels/rc_panel_city_spec.cpp",
            {"ai", "city_spec"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        uiint.RegisterWidget({"text", "AI Bridge offline", "ai.bridge.status", {"ai", "status"}});
        uiint.EndPanel();
        ImGui::End();
        return;
    }
    
    DesignSystem::SectionHeader("City Description");
    RenderBusyIndicator(m_processing, m_busyTime);
    
    ImGui::Text("Describe your city:");
    ImGui::InputTextMultiline("##desc", m_descBuffer, sizeof(m_descBuffer), ImVec2(-1, 120));
    uiint.RegisterWidget({"text", "Description", "city_spec.description", {"input"}});
    
    const char* scales[] = { "Hamlet", "Town", "City", "Metro" };
    ImGui::Combo("Scale", &m_scaleIndex, scales, 4);
    uiint.RegisterWidget({"combo", "Scale", "city_spec.scale", {"input"}});
    
    ImGui::BeginDisabled(m_processing.load());
    if (DesignSystem::ButtonPrimary("Generate CitySpec", ImVec2(180, 30))) {
        if (!m_processing.exchange(true)) {
            m_busyTime = 0.0f;

            std::string desc = std::string(m_descBuffer);
            std::string scaleStr = scales[m_scaleIndex];
            std::transform(scaleStr.begin(), scaleStr.end(), scaleStr.begin(), ::tolower);

            std::thread([this, desc, scaleStr]() {
                Core::CitySpec spec = AI::CitySpecClient::GenerateSpec(desc, scaleStr);
                bool hasSpec = !spec.intent.description.empty();
                {
                    std::scoped_lock lock(m_specMutex);
                    m_currentSpec = std::move(spec);
                    m_hasSpec = hasSpec;
                }
                m_processing = false;
            }).detach();
        }
    }
    ImGui::EndDisabled();
    uiint.RegisterWidget({"button", "Generate CitySpec", "action:ai.city_spec.generate", {"action", "ai"}});
    
    Core::CitySpec specCopy;
    bool hasSpecCopy = false;
    {
        std::scoped_lock lock(m_specMutex);
        specCopy = m_currentSpec;
        hasSpecCopy = m_hasSpec;
    }

    if (hasSpecCopy) {
        DesignSystem::Separator();
        DesignSystem::SectionHeader("Generated Specification");
        
        ImGui::Text("Scale: %s", specCopy.intent.scale.c_str());
        ImGui::Text("Climate: %s", specCopy.intent.climate.c_str());
        ImGui::Text("Description: %s", specCopy.intent.description.c_str());
        
        if (!specCopy.intent.styleTags.empty()) {
            ImGui::Text("Style Tags:");
            for (const auto& tag : specCopy.intent.styleTags) {
                ImGui::BulletText("%s", tag.c_str());
            }
        }
        
        if (!specCopy.districts.empty()) {
            DesignSystem::Separator();
            ImGui::Text("Districts (%zu):", specCopy.districts.size());
            for (size_t i = 0; i < specCopy.districts.size(); ++i) {
                const auto& d = specCopy.districts[i];
                ImGui::BulletText("%s (density: %.2f)", d.type.c_str(), d.density);
            }
        }
        
        DesignSystem::Separator();
        ImGui::Text("Seed: %u", specCopy.seed);
        ImGui::Text("Road Density: %.2f", specCopy.roadDensity);
        
        if (DesignSystem::ButtonPrimary("Apply to Generator", ImVec2(180, 30))) {
            // TODO: Wire into actual city generator
            ImGui::OpenPopup("Not Implemented");
        }
        uiint.RegisterWidget({"button", "Apply to Generator", "action:generator.apply_city_spec", {"action", "generator"}});
        
        if (ImGui::BeginPopupModal("Not Implemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("CitySpec ? Generator integration coming in Phase 4");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    uiint.EndPanel();
    ImGui::End();
}

} // namespace RogueCity::UI
