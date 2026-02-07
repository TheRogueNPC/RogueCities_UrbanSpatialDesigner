// FILE: rc_panel_city_spec.cpp
// PURPOSE: CitySpec Generator for AI-driven city design

#include "rc_panel_city_spec.h"
#include "client/CitySpecClient.h"
#include "runtime/AiBridgeRuntime.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include <imgui.h>
#include <algorithm>

namespace RogueCity::UI {

void CitySpecPanel::Render() {
    if (!ImGui::Begin("City Spec Generator", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    
    auto& runtime = AI::AiBridgeRuntime::Instance();
    
    // Check if bridge is online
    if (!runtime.IsOnline()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DesignSystem::ToVec4(DesignTokens::YellowWarning));
        ImGui::Text("AI Bridge offline - start it in AI Console");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    
    DesignSystem::SectionHeader("City Description");
    
    ImGui::Text("Describe your city:");
    ImGui::InputTextMultiline("##desc", m_descBuffer, sizeof(m_descBuffer), ImVec2(-1, 120));
    
    const char* scales[] = { "Hamlet", "Town", "City", "Metro" };
    ImGui::Combo("Scale", &m_scaleIndex, scales, 4);
    
    ImGui::BeginDisabled(m_processing);
    if (DesignSystem::ButtonPrimary("Generate CitySpec", ImVec2(180, 30))) {
        m_processing = true;
        
        std::string scaleStr = scales[m_scaleIndex];
        std::transform(scaleStr.begin(), scaleStr.end(), scaleStr.begin(), ::tolower);
        
        m_currentSpec = AI::CitySpecClient::GenerateSpec(
            std::string(m_descBuffer),
            scaleStr
        );
        
        m_hasSpec = !m_currentSpec.intent.description.empty();
        m_processing = false;
    }
    ImGui::EndDisabled();
    
    if (m_hasSpec) {
        DesignSystem::Separator();
        DesignSystem::SectionHeader("Generated Specification");
        
        ImGui::Text("Scale: %s", m_currentSpec.intent.scale.c_str());
        ImGui::Text("Climate: %s", m_currentSpec.intent.climate.c_str());
        ImGui::Text("Description: %s", m_currentSpec.intent.description.c_str());
        
        if (!m_currentSpec.intent.styleTags.empty()) {
            ImGui::Text("Style Tags:");
            for (const auto& tag : m_currentSpec.intent.styleTags) {
                ImGui::BulletText("%s", tag.c_str());
            }
        }
        
        if (!m_currentSpec.districts.empty()) {
            DesignSystem::Separator();
            ImGui::Text("Districts (%zu):", m_currentSpec.districts.size());
            for (size_t i = 0; i < m_currentSpec.districts.size(); ++i) {
                const auto& d = m_currentSpec.districts[i];
                ImGui::BulletText("%s (density: %.2f)", d.type.c_str(), d.density);
            }
        }
        
        DesignSystem::Separator();
        ImGui::Text("Seed: %u", m_currentSpec.seed);
        ImGui::Text("Road Density: %.2f", m_currentSpec.roadDensity);
        
        if (DesignSystem::ButtonPrimary("Apply to Generator", ImVec2(180, 30))) {
            // TODO: Wire into actual city generator
            ImGui::OpenPopup("Not Implemented");
        }
        
        if (ImGui::BeginPopupModal("Not Implemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("CitySpec ? Generator integration coming in Phase 4");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    ImGui::End();
}

} // namespace RogueCity::UI
